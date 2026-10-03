// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SignedByteBlock.hpp"

#include "impl/AuthenticationCodec.hpp"
#include "impl/CryptologyOids.hpp"
#include "keys/PublicKey.hpp"
#include "keys/SigningPrivateKey.hpp"
#include "x509/X509AlgorithmIdentifier.hpp"

#include "../err/LogicFailure.hpp"
#include "../err/ParameterFailure.hpp"
#include "../err/ParseError.hpp"
#include "../err/RuntimeError.hpp"
#include "../mem/ByteReader.hpp"
#include "../mem/ByteWriter.hpp"
#include "../text/EncodingMode.hpp"
#include "../text/Literals.hpp"
#include "../text/StringDecoder.hpp"
#include "../text/StringEncoder.hpp"
#include "../text/StringEncoding.hpp"
#include "../unit/ByteCount.hpp"

#include <limits>

namespace erbsland::cryptology {

using namespace err;
using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

SignedByteBlock::SignedByteBlock(
    ByteBlock encoded,
    std::optional<String> keyId,
    String purpose,
    const ByteIndex payloadOffset,
    const ByteCount payloadLength) noexcept :
    _encoded{std::move(encoded)},
    _keyId{std::move(keyId)},
    _purpose{std::move(purpose)},
    _payloadOffset{payloadOffset},
    _payloadLength{payloadLength} {
}

auto SignedByteBlock::verify(const PublicKey &key, const String &expectedPurpose) const -> std::optional<ByteBlock> {
    if (isEmpty()) {
        throw LogicFailure{"Cannot verify an empty signed byte block."_el};
    }
    if (_purpose != expectedPurpose || key.isEmpty() ||
        key.algorithm().oid().toString() != impl::cryptology_oids::ed25519) {
        return std::nullopt;
    }
    static const auto cEd25519Algorithm = X509AlgorithmIdentifier::fromDerOrThrow(
        ByteBlock{Byte{0x30U}, Byte{0x05U}, Byte{0x06U}, Byte{0x03U}, Byte{0x2bU}, Byte{0x65U}, Byte{0x70U}});
    const auto signatureOffset = _encoded.length() - ByteCount{64U};
    try {
        if (!key.verifySignature(
                cEd25519Algorithm,
                _encoded.slice(ByteIndex{0U}, signatureOffset).span(),
                _encoded.slice(ByteIndex::end(signatureOffset), ByteCount{64U}).span())) {
            return std::nullopt;
        }
    } catch (const ParseError &) {
        return std::nullopt;
    }
    return _encoded.slice(_payloadOffset, _payloadLength);
}

auto SignedByteBlock::fromByteBlock(const ConstByteSpan encoded, const ByteCount maximumPayload) noexcept
    -> SignedByteBlock {
    if (!maximumPayload.isFinite()) {
        return {};
    }
    try {
        return fromByteBlockOrThrow(encoded, maximumPayload);
    } catch (const RuntimeError &) {
        return {};
    }
}

auto SignedByteBlock::fromByteBlockOrThrow(const ConstByteSpan encoded, const ByteCount maximumPayload)
    -> SignedByteBlock {
    if (encoded.size() < 76U) {
        throw ParseError{"Signed byte block is too short."_el};
    }
    if (!maximumPayload.isFinite()) {
        throw ParameterFailure{"A finite maximum payload length is required."_el, "maximumPayload"_el};
    }
    const auto maximum = maximumPayload.toSizeT();
    if (encoded.size() > maximum && encoded.size() - maximum > 395U) {
        throw ParseError{"Signed byte block exceeds the caller's payload limit."_el};
    }
    auto reader = ByteReader{ByteBlock::fromSpan(encoded)};
    reader.setEndianness(Endianness::Big);
    if (reader.readByte().toUInt8() != 'E' || reader.readByte().toUInt8() != 'L' ||
        reader.readByte().toUInt8() != 'S' || reader.readByte().toUInt8() != 'B' || reader.readByte().toUInt8() != 1U ||
        reader.readByte().toUInt8() != 1U) {
        throw ParseError{"Unsupported signed byte block format."_el};
    }
    const auto keyIdLength = reader.readByte().toUInt8();
    const auto purposeLength = reader.readByte().toUInt8();
    const auto payloadLength = reader.readUInt32OrThrow();
    const auto fixedLength = std::size_t{76U} + keyIdLength + purposeLength;
    if (keyIdLength > 64U || purposeLength == 0U || encoded.size() < fixedLength ||
        encoded.size() - fixedLength != payloadLength) {
        throw ParseError{"Signed byte block has invalid field lengths."_el};
    }
    if (payloadLength > maximum) {
        throw ParseError{"Signed byte block payload exceeds the caller's limit."_el};
    }
    auto keyId = std::optional<String>{};
    if (keyIdLength != 0U) {
        const auto bytes = reader.readBytesOrThrow(ByteCount{keyIdLength, ByteCount::Unchecked{}});
        keyId = StringDecoder{bytes}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
        if (!impl::authentication_codec::isValidIdentifier(*keyId)) {
            throw ParseError{"Signed byte block has an invalid key identifier."_el};
        }
    }
    const auto purposeBytes = reader.readBytesOrThrow(ByteCount{purposeLength, ByteCount::Unchecked{}});
    auto purpose =
        StringDecoder{purposeBytes}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
    if (purpose.isEmpty() || !purpose.isEncodingValid()) {
        throw ParseError{"Signed byte block has an invalid purpose."_el};
    }
    const auto payloadOffset = reader.position();
    return SignedByteBlock{
        ByteBlock::fromSpan(encoded),
        std::move(keyId),
        std::move(purpose),
        payloadOffset,
        ByteCount{payloadLength, ByteCount::Unchecked{}}};
}

auto SignedByteBlock::sign(
    const SigningPrivateKey &key, const ConstByteSpan payload, const String &purpose, std::optional<String> keyId)
    -> SignedByteBlock {
    if (purpose.isEmpty() || !purpose.isEncodingValid() || purpose.length() > ByteCount{255U} ||
        (keyId.has_value() && !impl::authentication_codec::isValidIdentifier(*keyId)) ||
        payload.size() > std::numeric_limits<uint32_t>::max()) {
        throw ParameterFailure{"Invalid signed byte block purpose, identifier, or payload."_el, "payload"_el};
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
    const auto payloadOffset = ByteIndex::end(writer.length());
    writer.writeBytes(payload);
    const auto signature = key.signEd25519Framed(writer.toByteBlock().span());
    writer.writeBytes(signature);
    return SignedByteBlock{
        writer.toByteBlock(), std::move(keyId), purpose, payloadOffset, ByteCount::fromSizeT(payload.size())};
}

}
