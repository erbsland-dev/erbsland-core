// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PemCodec.hpp"

#include "DerParser.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/AsciiCategory.hpp"
#include "../../text/base_n/BaseNDecoder.hpp"
#include "../../text/base_n/BaseNEncoder.hpp"
#include "../../text/base_n/BaseNFormat.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"
#include "../../unit/ByteLength.hpp"

#include <utility>

namespace erbsland::cryptology::impl {

using namespace text::literals;

PemCodec::PemCodec(text::String text) noexcept : _text{std::move(text)} {
}

PemCodec::PemCodec(mem::ByteBlock certificate) {
    _certificates.append(std::move(certificate));
}

PemCodec::PemCodec(util::List<mem::ByteBlock> certificates) noexcept : _certificates{std::move(certificates)} {
}

auto PemCodec::decode() const -> util::List<mem::ByteBlock> {
    // RFC 7468 sections 3 and 5.1: parse exact CERTIFICATE boundaries with whitespace-tolerant Base64 content.
    if (_text.length().toSizeTOrThrow() > cMaximumBundleTextLength) {
        throw err::OutOfRangeError{"PEM bundle exceeds the fixed source-text limit."_el};
    }
    auto reader = text::StringCharReader{_text};
    auto result = util::List<mem::ByteBlock>{};
    auto totalDerLength = std::size_t{};
    static const auto cWhitespace = text::CharSet::from(text::AsciiCategory::Whitespace);
    reader.advanceWhile(cWhitespace);
    while (!reader.isAtEnd()) {
        if (result.count().toSizeT() >= cMaximumCertificates) {
            throw err::OutOfRangeError{"PEM bundle exceeds the fixed certificate-count limit."_el};
        }
        if (!reader.advanceIf("-----BEGIN CERTIFICATE-----"_el) || reader.advanceWhile(cWhitespace).isZero()) {
            throw err::ParseError{
                "Expected an exact CERTIFICATE PEM pre-encapsulation boundary."_el, reader.position()};
        }
        auto base64 = text::StringEditor{};
        while (true) {
            if (reader.isAtEnd()) {
                throw err::ParseError{
                    "PEM certificate is missing its post-encapsulation boundary."_el, reader.position()};
            }
            if (reader.peek() == U'-') {
                if (!reader.advanceIf("-----END CERTIFICATE-----"_el)) {
                    throw err::ParseError{
                        "Malformed CERTIFICATE PEM post-encapsulation boundary."_el, reader.position()};
                }
                break;
            }
            const auto character = reader.read();
            if (character.isAsciiWhitespace()) {
                continue;
            }
            const auto isBase64 =
                character.isAsciiAlphanumeric() || character == U'+' || character == U'/' || character == U'=';
            if (!isBase64) {
                throw err::ParseError{"PEM certificate contains a non-Base64 character."_el, reader.position()};
            }
            base64.append(character);
            if (base64.length().toSizeTOrThrow() > cMaximumCertificateTextLength) {
                throw err::OutOfRangeError{"PEM certificate exceeds the fixed text limit."_el};
            }
        }
        if (base64.isEmpty()) {
            throw err::ParseError{"PEM certificate contains no Base64 data."_el, reader.position()};
        }
        auto format = text::base_n::BaseNFormat::base64();
        format.setWhitespace({});
        auto der = text::base_n::BaseNDecoder{text::String{base64}, format}.toDataOrThrow(DerParser::cMaximumLength);
        const auto derLength = der.length().toSizeTOrThrow();
        if (derLength > cMaximumBundleDerLength - totalDerLength) {
            throw err::OutOfRangeError{"PEM bundle exceeds the fixed decoded-DER limit."_el};
        }
        totalDerLength += derLength;
        result.append(std::move(der));
        reader.advanceWhile(cWhitespace);
    }
    if (result.isEmpty()) {
        throw err::ParseError{"PEM input contains no CERTIFICATE block."_el, unit::CpIndex::zero()};
    }
    return result;
}

auto PemCodec::encode() const -> text::String {
    auto result = text::StringEditor{};
    for (const auto &certificate : _certificates) {
        appendEncoded(result, certificate);
    }
    return text::String{result};
}

void PemCodec::appendEncoded(text::StringEditor &result, const mem::ByteBlock &der) {
    // RFC 7468 sections 3 and 5.1: canonical CERTIFICATE boundaries and 64-character Base64 lines.
    result.append("-----BEGIN CERTIFICATE-----\n"_el);
    const auto encoded = text::base_n::BaseNEncoder{der, text::base_n::BaseNFormat::base64Pem()}.toString();
    result.append(encoded);
    if (!encoded.endsWith("\n"_el)) {
        result.append(U'\n');
    }
    result.append("-----END CERTIFICATE-----\n"_el);
}

}
