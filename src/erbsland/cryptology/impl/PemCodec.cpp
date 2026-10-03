// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PemCodec.hpp"

#include "DerParser.hpp"

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
#include "../../unit/ByteCount.hpp"

#include <utility>

namespace erbsland::cryptology::impl {

using namespace err;
using namespace mem;
using namespace text;
using namespace text::literals;

PemCodec::PemCodec(String text, const PemLabel label) noexcept : _text{std::move(text)}, _label{label} {
}

PemCodec::PemCodec(ByteBlock certificate, const PemLabel label) : _label{label} {
    _certificates.append(std::move(certificate));
}

PemCodec::PemCodec(util::List<ByteBlock> certificates, const PemLabel label) noexcept :
    _certificates{std::move(certificates)}, _label{label} {
}

auto PemCodec::decode() const -> util::List<ByteBlock> {
    // RFC 7468 sections 3 and 5.1: parse exact CERTIFICATE boundaries with whitespace-tolerant Base64 content.
    if (_text.length().toSizeTOrThrow() > cMaximumBundleTextLength) {
        throw ParseError{"PEM bundle exceeds the fixed source-text limit."_el};
    }
    auto reader = StringCharReader{_text};
    auto result = util::List<ByteBlock>{};
    auto totalDerLength = std::size_t{};
    reader.advanceWhile(AsciiCategory::Whitespace);
    while (!reader.isAtEnd()) {
        const auto maximumBlocks = _label == PemLabel::Certificate ? cMaximumCertificates : std::size_t{1U};
        if (result.count().toSizeT() >= maximumBlocks) {
            if (_label != PemLabel::Certificate) {
                throw ParseError{"This PEM artifact must contain exactly one block."_el, reader.position()};
            }
            throw ParseError{"PEM bundle exceeds the fixed certificate-count limit."_el};
        }
        if (!reader.advanceIf(beginBoundary()) || reader.advanceWhile(AsciiCategory::Whitespace).isZero()) {
            throw ParseError{
                "Expected the exact PEM pre-encapsulation boundary for this artifact."_el, reader.position()};
        }
        auto base64 = StringEditor{};
        while (true) {
            if (reader.isAtEnd()) {
                throw ParseError{"PEM artifact is missing its post-encapsulation boundary."_el, reader.position()};
            }
            if (reader.peek() == U'-') {
                if (!reader.advanceIf(endBoundary())) {
                    throw ParseError{"Malformed PEM post-encapsulation boundary."_el, reader.position()};
                }
                break;
            }
            const auto character = reader.read();
            if (character.isAsciiWhitespace()) {
                continue;
            }
            if (!character.isAsciiCategory(AsciiCategory::Base64Text)) {
                throw ParseError{"PEM artifact contains a non-Base64 character."_el, reader.position()};
            }
            base64.append(character);
            if (base64.length().toSizeTOrThrow() > cMaximumCertificateTextLength) {
                throw ParseError{"PEM artifact exceeds the fixed text limit."_el};
            }
        }
        if (base64.isEmpty()) {
            throw ParseError{"PEM artifact contains no Base64 data."_el, reader.position()};
        }
        auto format = base_n::BaseNFormat::base64();
        format.setWhitespace({});
        auto der = base_n::BaseNDecoder{base64, format}.toDataOrThrow(DerParser::cMaximumLength);
        const auto derLength = der.length().toSizeTOrThrow();
        if (derLength > cMaximumBundleDerLength - totalDerLength) {
            throw ParseError{"PEM bundle exceeds the fixed decoded-DER limit."_el};
        }
        totalDerLength += derLength;
        result.append(std::move(der));
        reader.advanceWhile(AsciiCategory::Whitespace);
    }
    if (result.isEmpty()) {
        throw ParseError{"PEM input contains no block with the required label."_el, unit::CpIndex::zero()};
    }
    return result;
}

auto PemCodec::encode() const -> String {
    auto result = StringEditor{};
    for (const auto &certificate : _certificates) {
        appendEncoded(result, certificate);
    }
    return result;
}

void PemCodec::appendEncoded(StringEditor &result, const ByteBlock &der) const {
    // RFC 7468 section 3: exact boundaries and 64-character Base64 lines.
    result.append(beginBoundary());
    result.append(U'\n');
    const auto encoded = base_n::BaseNEncoder{der, base_n::BaseNFormat::base64Pem()}.toString();
    result.append(encoded);
    if (!encoded.endsWith("\n"_el)) {
        result.append(U'\n');
    }
    result.append(endBoundary());
    result.append(U'\n');
}

auto PemCodec::labelText() const -> String {
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

auto PemCodec::beginBoundary() const -> String {
    auto result = StringEditor{"-----BEGIN "_el};
    result.append(labelText());
    result.append("-----"_el);
    return result;
}

auto PemCodec::endBoundary() const -> String {
    auto result = StringEditor{"-----END "_el};
    result.append(labelText());
    result.append("-----"_el);
    return result;
}

}
