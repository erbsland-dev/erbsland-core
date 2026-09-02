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
#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"
#include "../../unit/ByteLength.hpp"

#include <utility>

namespace erbsland::cryptology::impl {

using namespace text::literals;

PemCodec::PemCodec(text::String text, const PemLabel label) noexcept : _text{std::move(text)}, _label{label} {
}

PemCodec::PemCodec(mem::ByteBlock certificate, const PemLabel label) : _label{label} {
    _certificates.append(std::move(certificate));
}

PemCodec::PemCodec(util::List<mem::ByteBlock> certificates, const PemLabel label) noexcept :
    _certificates{std::move(certificates)}, _label{label} {
}

auto PemCodec::decode() const -> util::List<mem::ByteBlock> {
    // RFC 7468 sections 3 and 5.1: parse exact CERTIFICATE boundaries with whitespace-tolerant Base64 content.
    if (_text.length().toSizeTOrThrow() > cMaximumBundleTextLength) {
        throw err::OutOfRangeError{"PEM bundle exceeds the fixed source-text limit."_el};
    }
    auto reader = text::StringCharReader{_text};
    auto result = util::List<mem::ByteBlock>{};
    auto totalDerLength = std::size_t{};
    reader.advanceWhile(text::AsciiCategory::Whitespace);
    while (!reader.isAtEnd()) {
        const auto maximumBlocks = _label == PemLabel::Certificate ? cMaximumCertificates : std::size_t{1U};
        if (result.count().toSizeT() >= maximumBlocks) {
            if (_label != PemLabel::Certificate) {
                throw err::ParseError{"This PEM artifact must contain exactly one block."_el, reader.position()};
            }
            throw err::OutOfRangeError{"PEM bundle exceeds the fixed certificate-count limit."_el};
        }
        if (!reader.advanceIf(beginBoundary()) || reader.advanceWhile(text::AsciiCategory::Whitespace).isZero()) {
            throw err::ParseError{
                "Expected the exact PEM pre-encapsulation boundary for this artifact."_el, reader.position()};
        }
        auto base64 = text::StringEditor{};
        while (true) {
            if (reader.isAtEnd()) {
                throw err::ParseError{"PEM artifact is missing its post-encapsulation boundary."_el, reader.position()};
            }
            if (reader.peek() == U'-') {
                if (!reader.advanceIf(endBoundary())) {
                    throw err::ParseError{"Malformed PEM post-encapsulation boundary."_el, reader.position()};
                }
                break;
            }
            const auto character = reader.read();
            if (character.isAsciiWhitespace()) {
                continue;
            }
            if (!character.isAsciiCategory(text::AsciiCategory::Base64Text)) {
                throw err::ParseError{"PEM artifact contains a non-Base64 character."_el, reader.position()};
            }
            base64.append(character);
            if (base64.length().toSizeTOrThrow() > cMaximumCertificateTextLength) {
                throw err::OutOfRangeError{"PEM artifact exceeds the fixed text limit."_el};
            }
        }
        if (base64.isEmpty()) {
            throw err::ParseError{"PEM artifact contains no Base64 data."_el, reader.position()};
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
        reader.advanceWhile(text::AsciiCategory::Whitespace);
    }
    if (result.isEmpty()) {
        throw err::ParseError{"PEM input contains no block with the required label."_el, unit::CpIndex::zero()};
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

void PemCodec::appendEncoded(text::StringEditor &result, const mem::ByteBlock &der) const {
    // RFC 7468 section 3: exact boundaries and 64-character Base64 lines.
    result.append(beginBoundary());
    result.append(U'\n');
    const auto encoded = text::base_n::BaseNEncoder{der, text::base_n::BaseNFormat::base64Pem()}.toString();
    result.append(encoded);
    if (!encoded.endsWith("\n"_el)) {
        result.append(U'\n');
    }
    result.append(endBoundary());
    result.append(U'\n');
}

auto PemCodec::labelText() const -> text::String {
    switch (_label) {
    case PemLabel::Certificate:
        return "CERTIFICATE"_el;
    case PemLabel::PrivateKey:
        return "PRIVATE KEY"_el;
    case PemLabel::EncryptedPrivateKey:
        return "ENCRYPTED PRIVATE KEY"_el;
    case PemLabel::PublicKey:
        return "PUBLIC KEY"_el;
    case PemLabel::CertificateRequest:
        return "CERTIFICATE REQUEST"_el;
    }
    return {};
}

auto PemCodec::beginBoundary() const -> text::String {
    auto result = text::StringEditor{"-----BEGIN "_el};
    result.append(labelText());
    result.append("-----"_el);
    return text::String{result};
}

auto PemCodec::endBoundary() const -> text::String {
    auto result = text::StringEditor{"-----END "_el};
    result.append(labelText());
    result.append("-----"_el);
    return text::String{result};
}

}
