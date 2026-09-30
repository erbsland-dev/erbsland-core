// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AuthenticationCodec.hpp"

#include "../../err/ParameterError.hpp"
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
#include "../../unit/ByteLength.hpp"

namespace erbsland::cryptology::impl::authentication_codec {

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

auto encodeBase64Url(const mem::ConstByteSpan data, const bool sensitive) -> String {
    auto format = base_n::BaseNFormat::base64Url();
    format.clearFlags(base_n::BaseNFormatFlag::All);
    format.setPadding(std::nullopt);
    format.setWhitespace(CharSet{});
    auto bytes = mem::ByteBlock::fromSpan(data);
    if (sensitive) {
        bytes.markAsSensitive();
    }
    return base_n::BaseNEncoder{std::move(bytes), std::move(format)}.toString();
}

auto decodeBase64Url(const String &encoded, const std::size_t expectedLength, const bool sensitive) -> mem::ByteBlock {
    auto format = base_n::BaseNFormat::base64Url();
    format.clearFlags(base_n::BaseNFormatFlag::All);
    format.setPadding(std::nullopt);
    format.setWhitespace(CharSet{});
    if (encoded.length().toSizeT() != (expectedLength * 8U + 5U) / 6U) {
        throw err::ParseError{"Authentication data has an invalid encoded length."_el};
    }
    auto result = base_n::BaseNDecoder{encoded, format}.toDataOrThrow(ByteLength{expectedLength});
    if (sensitive) {
        result.markAsSensitive();
    }
    if (result.length().toSizeT() != expectedLength || encodeBase64Url(result.span(), sensitive) != encoded) {
        throw err::ParseError{"Authentication data is not canonical Base64url."_el};
    }
    return result;
}

auto encodeToken(const String &identifier, const char role, const mem::ConstByteSpan keyData) -> String {
    if (!isValidIdentifier(identifier) || (role != 'c' && role != 's') || keyData.size() != 32U) {
        throw err::ParameterError{"Invalid authentication-token identifier, role, or key data."_el, "identifier"_el};
    }
    auto editor = mem::ByteBlockEditor{ByteLength{34U}};
    if (role == 'c') {
        editor.markAsSensitive();
    }
    editor.set(ByteIndex{0U}, mem::Byte{1U});
    editor.set(ByteIndex{1U}, mem::Byte{1U});
    for (auto index = 0U; index < 32U; ++index) {
        editor.set(ByteIndex{index + 2U}, keyData[index]);
    }
    auto result = StringEditor{};
    if (role == 'c') {
        result.reserve(ByteLength{6U + identifier.length().toSizeT() + 3U + 46U});
        result.markAsSensitive();
    }
    result.append("elctk-"_el).append(identifier).append(U'-').append(role == 'c' ? U'c' : U's').append(U'-');
    auto encoded = encodeBase64Url(mem::ByteBlock{editor}.span(), role == 'c');
    result.append(encoded);
    auto token = String{result};
    if (role == 'c') {
        token.markAsSensitive();
    }
    return token;
}

auto decodeToken(const String &token, const char expectedRole, String &identifier) -> mem::ByteBlock {
    const auto bytes = text::impl::UnsafeU8StringAccess{token}.dataSpan();
    if (bytes.size() < 6U + 1U + 3U + 46U || bytes.size() > 6U + 64U + 3U + 46U || !token.startsWith("elctk-"_el)) {
        throw err::ParseError{"Invalid authentication-token text."_el};
    }
    auto separator = std::size_t{6U};
    while (separator < bytes.size() && bytes[separator] != '-') {
        ++separator;
    }
    identifier = token.slice({ByteIndex{6U}, ByteLength{separator - 6U}});
    if (!isValidIdentifier(identifier) || separator + 2U >= bytes.size() || bytes[separator + 1U] != expectedRole ||
        bytes[separator + 2U] != '-') {
        throw err::ParseError{"Authentication-token identifier or role is invalid."_el};
    }
    const auto encoded = token.slice({ByteIndex{separator + 3U}, ByteLength::infinite()});
    auto data = decodeBase64Url(encoded, 34U, expectedRole == 'c');
    if (data.span()[0U] != mem::Byte{1U} || data.span()[1U] != mem::Byte{1U}) {
        throw err::ParseError{"Unsupported authentication-token version or algorithm."_el};
    }
    return data.slice(ByteIndex{2U}, ByteLength{32U});
}

auto encodeChallenge(const String &identifier, const mem::ConstByteSpan nonce) -> mem::ByteBlock {
    if (!isValidIdentifier(identifier) || nonce.size() != 32U) {
        throw err::ParameterError{"Invalid challenge identifier or nonce."_el, "identifier"_el};
    }
    const auto identifierBytes = StringEncoder{identifier}.encode(StringEncoding::Utf8, StringBomMode::Reject);
    auto writer = mem::ByteWriter{};
    writer.writeByte(mem::Byte{'E'}).writeByte(mem::Byte{'L'}).writeByte(mem::Byte{'C'}).writeByte(mem::Byte{'A'});
    writer.writeUInt8(1U).writeUInt8(static_cast<uint8_t>(identifierBytes.length().toSizeT()));
    writer.writeBytes(identifierBytes).writeBytes(nonce);
    return writer.toByteBlock();
}

auto decodeChallengeIdentifier(const mem::ConstByteSpan challenge) -> String {
    if (challenge.size() < 39U || challenge.size() > 102U || challenge[0U].toUInt8() != 'E' ||
        challenge[1U].toUInt8() != 'L' || challenge[2U].toUInt8() != 'C' || challenge[3U].toUInt8() != 'A' ||
        challenge[4U].toUInt8() != 1U) {
        throw err::ParseError{"Unsupported authentication challenge."_el};
    }
    const auto idLength = challenge[5U].toUInt8();
    if (idLength == 0U || idLength > 64U || challenge.size() != 6U + idLength + 32U) {
        throw err::ParseError{"Authentication challenge has invalid field lengths."_el};
    }
    const auto idBytes = mem::ByteBlock::fromSpan(challenge.subspan(6U, idLength));
    const auto identifier =
        StringDecoder{idBytes}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
    if (!isValidIdentifier(identifier)) {
        throw err::ParseError{"Authentication challenge has an invalid identifier."_el};
    }
    return identifier;
}

}
