// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AuthenticationCodec.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../mem/ByteWriter.hpp"
#include "../../text/base_n/BaseNDecoder.hpp"
#include "../../text/base_n/BaseNEncoder.hpp"
#include "../../text/base_n/BaseNFormat.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/EncodingMode.hpp"
#include "../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringDecoder.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringEncoder.hpp"
#include "../../text/StringEncoding.hpp"
#include "../../unit/ByteCount.hpp"

namespace erbsland::cryptology::impl::authentication_codec {

using namespace err;
using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

auto isValidIdentifier(const String &identifier) noexcept -> bool {
    const auto bytes = text::impl::UnsafeU8StringAccess{identifier}.dataSpan();
    if (bytes.empty() || bytes.size() > 64U) {
        return false;
    }
    for (const auto character : bytes) {
        if (character != '_' && (character < 'a' || character > 'z') && (character < '0' || character > '9')) {
            return false;
        }
    }
    return true;
}

auto encodeBase64Url(const ConstByteSpan data, const bool sensitive) -> String {
    auto format = base_n::BaseNFormat::base64Url();
    format.clearFlags(base_n::BaseNFormatFlag::All);
    format.setPadding(std::nullopt);
    format.setWhitespace(CharSet{});
    auto bytes = ByteBlock::fromSpan(data);
    if (sensitive) {
        bytes.markAsSensitive();
    }
    return base_n::BaseNEncoder{std::move(bytes), std::move(format)}.toString();
}

auto decodeBase64Url(const String &encoded, const unit::ByteCount expectedLength, const bool sensitive) -> ByteBlock {
    auto format = base_n::BaseNFormat::base64Url();
    format.clearFlags(base_n::BaseNFormatFlag::All);
    format.setPadding(std::nullopt);
    format.setWhitespace(CharSet{});
    if (encoded.length() != (expectedLength * 8U + ByteCount{5U}) / 6U) {
        throw ParseError{"Authentication data has an invalid encoded length."_el};
    }
    auto result = base_n::BaseNDecoder{encoded, format}.toDataOrThrow(expectedLength);
    if (sensitive) {
        result.markAsSensitive();
    }
    if (result.length() != expectedLength || encodeBase64Url(result.span(), sensitive) != encoded) {
        throw ParseError{"Authentication data is not canonical Base64url."_el};
    }
    return result;
}

auto encodeToken(const String &identifier, const char role, const ConstByteSpan keyData) -> String {
    if (!isValidIdentifier(identifier) || (role != 'c' && role != 's') || keyData.size() != 32U) {
        throw ParameterFailure{"Invalid authentication-token identifier, role, or key data."_el, "identifier"_el};
    }
    auto editor = ByteBlockEditor{ByteCount{34U}};
    if (role == 'c') {
        editor.markAsSensitive();
    }
    editor.set(ByteIndex{0U}, Byte{1U});
    editor.set(ByteIndex{1U}, Byte{1U});
    for (auto index = 0U; index < 32U; ++index) {
        editor.set(ByteIndex{index + 2U, ByteIndex::Unchecked{}}, keyData[index]);
    }
    auto result = StringEditor{};
    if (role == 'c') {
        result.reserve(ByteCount{6U} + identifier.length() + ByteCount{3U} + ByteCount{46U});
        result.markAsSensitive();
    }
    result.append("elctk-"_el).append(identifier).append(U'-').append(role == 'c' ? U'c' : U's').append(U'-');
    auto encoded = encodeBase64Url(ByteBlock{editor}.span(), role == 'c');
    result.append(encoded);
    auto token = String{result};
    if (role == 'c') {
        token.markAsSensitive();
    }
    return token;
}

auto decodeToken(const String &token, const char expectedRole, String &identifier) -> ByteBlock {
    const auto bytes = text::impl::UnsafeU8StringAccess{token}.dataSpan();
    if (bytes.size() < 6U + 1U + 3U + 46U || bytes.size() > 6U + 64U + 3U + 46U || !token.startsWith("elctk-"_el)) {
        throw ParseError{"Invalid authentication-token text."_el};
    }
    auto separator = ByteIndex{6U};
    while (separator.isWithin(token.length()) && bytes[separator.toSizeT()] != '-') {
        separator.uncheckedIncrement();
    }
    identifier = token.slice({ByteIndex{6U}, separator.distanceFromZero() - ByteCount{6U}});
    if (!isValidIdentifier(identifier) || separator + ByteCount{2U} >= ByteIndex::end(token.length()) ||
        bytes[(separator + ByteCount{1U}).toSizeT()] != expectedRole ||
        bytes[(separator + ByteCount{2U}).toSizeT()] != '-') {
        throw ParseError{"Authentication-token identifier or role is invalid."_el};
    }
    const auto encoded = token.slice({separator + ByteCount{3U}, ByteCount::infinite()});
    auto data = decodeBase64Url(encoded, ByteCount{34U}, expectedRole == 'c');
    if (data.span()[0U] != Byte{1U} || data.span()[1U] != Byte{1U}) {
        throw ParseError{"Unsupported authentication-token version or algorithm."_el};
    }
    return data.slice(ByteIndex{2U}, ByteCount{32U});
}

auto encodeChallenge(const String &identifier, const ConstByteSpan nonce) -> ByteBlock {
    if (!isValidIdentifier(identifier) || nonce.size() != 32U) {
        throw ParameterFailure{"Invalid challenge identifier or nonce."_el, "identifier"_el};
    }
    const auto identifierBytes = StringEncoder{identifier}.encode(StringEncoding::Utf8, StringBomMode::Reject);
    auto writer = ByteWriter{};
    writer.writeByte(Byte{'E'}).writeByte(Byte{'L'}).writeByte(Byte{'C'}).writeByte(Byte{'A'});
    writer.writeUInt8(1U).writeUInt8(static_cast<uint8_t>(identifierBytes.length().toSizeT()));
    writer.writeBytes(identifierBytes).writeBytes(nonce);
    return writer.toByteBlock();
}

auto decodeChallengeIdentifier(const ConstByteSpan challenge) -> String {
    if (challenge.size() < 39U || challenge.size() > 102U || challenge[0U].toUInt8() != 'E' ||
        challenge[1U].toUInt8() != 'L' || challenge[2U].toUInt8() != 'C' || challenge[3U].toUInt8() != 'A' ||
        challenge[4U].toUInt8() != 1U) {
        throw ParseError{"Unsupported authentication challenge."_el};
    }
    const auto idLength = challenge[5U].toUInt8();
    if (idLength == 0U || idLength > 64U || challenge.size() != 6U + idLength + 32U) {
        throw ParseError{"Authentication challenge has invalid field lengths."_el};
    }
    const auto idBytes = ByteBlock::fromSpan(challenge.subspan(6U, idLength));
    const auto identifier =
        StringDecoder{idBytes}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
    if (!isValidIdentifier(identifier)) {
        throw ParseError{"Authentication challenge has an invalid identifier."_el};
    }
    return identifier;
}

}
