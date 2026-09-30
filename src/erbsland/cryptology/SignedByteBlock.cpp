// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SignedByteBlock.hpp"

#include "impl/AuthenticationCodec.hpp"
#include "impl/CryptologyOids.hpp"
#include "keys/PublicKey.hpp"
#include "keys/SigningPrivateKey.hpp"
#include "x509/X509AlgorithmIdentifier.hpp"

#include "../err/Exception.hpp"
#include "../err/LogicError.hpp"
#include "../err/OutOfRangeError.hpp"
#include "../err/ParameterError.hpp"
#include "../err/ParseError.hpp"
#include "../mem/ByteReader.hpp"
#include "../mem/ByteWriter.hpp"
#include "../text/EncodingMode.hpp"
#include "../text/Literals.hpp"
#include "../text/StringDecoder.hpp"
#include "../text/StringEncoder.hpp"
#include "../text/StringEncoding.hpp"

#include <limits>

namespace erbsland::cryptology {

using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

SignedByteBlock::SignedByteBlock(
    ByteBlock encoded,
    std::optional<String> keyId,
    String purpose,
    const std::size_t payloadOffset,
    const std::size_t payloadLength) noexcept :
    _encoded{std::move(encoded)},
    _keyId{std::move(keyId)},
    _purpose{std::move(purpose)},
    _payloadOffset{payloadOffset},
    _payloadLength{payloadLength} {
}

auto SignedByteBlock::verify(const PublicKey &key, const String &expectedPurpose) const -> std::optional<ByteBlock> {
    if (isEmpty()) {
        throw err::LogicError{"Cannot verify an empty signed byte block."_el};
    }
    if (_purpose != expectedPurpose || key.isEmpty() ||
        key.algorithm().oid().toString() != impl::cryptology_oids::ed25519) {
        return std::nullopt;
    }
    static const auto cEd25519Algorithm = X509AlgorithmIdentifier::fromDerOrThrow(
        ByteBlock{Byte{0x30U}, Byte{0x05U}, Byte{0x06U}, Byte{0x03U}, Byte{0x2bU}, Byte{0x65U}, Byte{0x70U}});
    const auto signatureOffset = _encoded.length().toSizeT() - 64U;
    try {
        if (!key.verifySignature(
                cEd25519Algorithm,
                _encoded.slice(ByteIndex{0U}, ByteLength{signatureOffset}).span(),
                _encoded.slice(ByteIndex{signatureOffset}, ByteLength{64U}).span())) {
            return std::nullopt;
        }
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
    return _encoded.slice(ByteIndex{_payloadOffset}, ByteLength{_payloadLength});
}

auto SignedByteBlock::fromByteBlock(const ConstByteSpan encoded, const ByteLength maximumPayload) noexcept
    -> SignedByteBlock {
    try {
        return fromByteBlockOrThrow(encoded, maximumPayload);
    } catch (const err::Exception &) {
        return {};
    }
}

auto SignedByteBlock::fromByteBlockOrThrow(const ConstByteSpan encoded, const ByteLength maximumPayload)
    -> SignedByteBlock {
    if (encoded.size() < 76U) {
        throw err::ParseError{"Signed byte block is too short."_el};
    }
    if (!maximumPayload.isFinite()) {
        throw err::ParameterError{"A finite maximum payload length is required."_el, "maximumPayload"_el};
    }
    const auto maximum = maximumPayload.toSizeT();
    if (encoded.size() > maximum && encoded.size() - maximum > 395U) {
        throw err::OutOfRangeError{"Signed byte block exceeds the caller's payload limit."_el};
    }
    auto reader = ByteReader{ByteBlock::fromSpan(encoded)};
    reader.setEndianness(Endianness::Big);
    if (reader.readByte().toUInt8() != 'E' || reader.readByte().toUInt8() != 'L' ||
        reader.readByte().toUInt8() != 'S' || reader.readByte().toUInt8() != 'B' || reader.readByte().toUInt8() != 1U ||
        reader.readByte().toUInt8() != 1U) {
        throw err::ParseError{"Unsupported signed byte block format."_el};
    }
    const auto keyIdLength = reader.readByte().toUInt8();
    const auto purposeLength = reader.readByte().toUInt8();
    const auto payloadLength = reader.readUInt32OrThrow();
    const auto fixedLength = std::size_t{76U} + keyIdLength + purposeLength;
    if (keyIdLength > 64U || purposeLength == 0U || encoded.size() < fixedLength ||
        encoded.size() - fixedLength != payloadLength) {
        throw err::ParseError{"Signed byte block has invalid field lengths."_el};
    }
    if (payloadLength > maximum) {
        throw err::OutOfRangeError{"Signed byte block payload exceeds the caller's limit."_el};
    }
    auto keyId = std::optional<String>{};
    if (keyIdLength != 0U) {
        const auto bytes = reader.readBytesOrThrow(ByteLength{keyIdLength});
        keyId = StringDecoder{bytes}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
        if (!impl::authentication_codec::isValidIdentifier(*keyId)) {
            throw err::ParseError{"Signed byte block has an invalid key identifier."_el};
        }
    }
    const auto purposeBytes = reader.readBytesOrThrow(ByteLength{purposeLength});
    auto purpose =
        StringDecoder{purposeBytes}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
    if (purpose.isEmpty() || !purpose.isValidUtf8()) {
        throw err::ParseError{"Signed byte block has an invalid purpose."_el};
    }
    const auto payloadOffset = reader.position().toSizeT();
    return SignedByteBlock{
        ByteBlock::fromSpan(encoded), std::move(keyId), std::move(purpose), payloadOffset, payloadLength};
}

auto SignedByteBlock::sign(
    const SigningPrivateKey &key, const ConstByteSpan payload, const String &purpose, std::optional<String> keyId)
    -> SignedByteBlock {
    if (purpose.isEmpty() || !purpose.isValidUtf8() || purpose.length().toSizeT() > 255U ||
        (keyId.has_value() && !impl::authentication_codec::isValidIdentifier(*keyId)) ||
        payload.size() > std::numeric_limits<uint32_t>::max()) {
        throw err::ParameterError{"Invalid signed byte block purpose, identifier, or payload."_el, "payload"_el};
    }
    const auto purposeBytes = StringEncoder{purpose}.encode(StringEncoding::Utf8, StringBomMode::Reject);
    const auto keyIdBytes =
        keyId.has_value() ? StringEncoder{*keyId}.encode(StringEncoding::Utf8, StringBomMode::Reject) : ByteBlock{};
    auto writer = ByteWriter{};
    writer.setEndianness(Endianness::Big);
    writer.writeByte(Byte{'E'}).writeByte(Byte{'L'}).writeByte(Byte{'S'}).writeByte(Byte{'B'});
    writer.writeUInt8(1U).writeUInt8(1U);
    writer.writeUInt8(static_cast<uint8_t>(keyIdBytes.length().toSizeT()));
    writer.writeUInt8(static_cast<uint8_t>(purposeBytes.length().toSizeT()));
    writer.writeUInt32(static_cast<uint32_t>(payload.size()));
    writer.writeBytes(keyIdBytes).writeBytes(purposeBytes);
    const auto payloadOffset = writer.length().toSizeT();
    writer.writeBytes(payload);
    const auto signature = key.signEd25519Framed(writer.toByteBlock().span());
    writer.writeBytes(signature);
    return SignedByteBlock{writer.toByteBlock(), std::move(keyId), purpose, payloadOffset, payload.size()};
}

}
