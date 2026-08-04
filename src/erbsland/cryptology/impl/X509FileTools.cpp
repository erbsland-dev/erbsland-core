// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509FileTools.hpp"

#include "PemCodec.hpp"

#include "../../err/ParameterError.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../path/Path.hpp"
#include "../../path/PathContent.hpp"
#include "../../path/PathReadDataOptions.hpp"
#include "../../path/PathWriteDataOptions.hpp"
#include "../../path/PathWriteTextOptions.hpp"
#include "../../text/Char.hpp"
#include "../../text/EncodingError.hpp"
#include "../../text/EncodingMode.hpp"
#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../text/StringBomMode.hpp"
#include "../../text/StringDecoder.hpp"
#include "../../text/StringEncoding.hpp"
#include "../../unit/ByteLength.hpp"

#include <array>
#include <utility>

namespace erbsland::cryptology::impl {

using namespace text::literals;

X509FileTools::X509FileTools(path::Path path) noexcept : _path{std::move(path)} {
}

auto X509FileTools::read() const -> mem::ByteBlock {
    auto options = path::PathReadDataOptions{};
    options.setMaximumByteLength(unit::ByteLength{PemCodec::cMaximumBundleTextLength});
    return path::PathContent{_path}.readDataOrThrow(options);
}

auto X509FileTools::inputFormat(const mem::ByteBlock &data, const X509CertificateFormat requested) const
    -> X509CertificateFormat {
    if (requested != X509CertificateFormat::Automatic) {
        return requested;
    }
    if (suffixIs(".pem"_el)) {
        if (!looksLikePem(data)) {
            throw err::ParseError{"A .pem certificate file does not contain PEM text."_el};
        }
        return X509CertificateFormat::Pem;
    }
    if (suffixIs(".der"_el)) {
        if (looksLikePem(data)) {
            throw err::ParseError{"A .der certificate file contains PEM text."_el};
        }
        return X509CertificateFormat::Der;
    }
    return looksLikePem(data) ? X509CertificateFormat::Pem : X509CertificateFormat::Der;
}

auto X509FileTools::outputFormat(const X509CertificateFormat requested) const -> X509CertificateFormat {
    if (requested != X509CertificateFormat::Automatic) {
        return requested;
    }
    if (suffixIs(".pem"_el) || suffixIs(".crt"_el)) {
        return X509CertificateFormat::Pem;
    }
    if (suffixIs(".der"_el) || suffixIs(".cer"_el)) {
        return X509CertificateFormat::Der;
    }
    throw err::ParameterError{
        "Automatic certificate output requires a .pem, .crt, .der, or .cer suffix."_el, "path"_el};
}

auto X509FileTools::toPemText(const mem::ByteBlock &data) -> text::String {
    // RFC 7468 section 3 requires textual encodings without a UTF-8 byte-order mark.
    if (!data.isEmpty() && data.span().size() >= 3U && data.span()[0U].toUInt8() == 0xEFU &&
        data.span()[1U].toUInt8() == 0xBBU && data.span()[2U].toUInt8() == 0xBFU) {
        throw err::ParseError{"PEM source must be BOM-free valid UTF-8."_el};
    }
    try {
        return text::StringDecoder{data}.decode(
            text::StringEncoding::Utf8, text::StringBomMode::Reject, text::EncodingMode::Strict);
    } catch (const text::EncodingError &) {
        throw err::ParseError{"PEM source must be BOM-free valid UTF-8."_el};
    }
}

void X509FileTools::writePem(const text::String &text) const {
    auto options = path::PathWriteTextOptions{text::StringEncoding::Utf8};
    options.setBomMode(text::StringBomMode::Reject);
    path::PathContent{_path}.writeTextOrThrow(text, options);
}

void X509FileTools::writeDer(const mem::ByteBlock &data) const {
    path::PathContent{_path}.writeDataOrThrow(data);
}

auto X509FileTools::suffixIs(const text::String &suffix) const noexcept -> bool {
    return _path.suffix().compare(suffix, text::Char::compareAsciiFolded) == std::strong_ordering::equal;
}

auto X509FileTools::looksLikePem(const mem::ByteBlock &data) noexcept -> bool {
    // RFC 7468 section 5.1 defines this exact CERTIFICATE pre-encapsulation boundary.
    constexpr auto cBoundary = std::array<uint8_t, 27U>{
        '-',
        '-',
        '-',
        '-',
        '-',
        'B',
        'E',
        'G',
        'I',
        'N',
        ' ',
        'C',
        'E',
        'R',
        'T',
        'I',
        'F',
        'I',
        'C',
        'A',
        'T',
        'E',
        '-',
        '-',
        '-',
        '-',
        '-'};
    auto offset = std::size_t{};
    while (offset < data.span().size()) {
        const auto value = data.span()[offset].toUInt8();
        if (value == ' ' || value == '\t' || value == '\r' || value == '\n' || value == '\v' || value == '\f') {
            ++offset;
            continue;
        }
        break;
    }
    if (data.span().size() - offset < cBoundary.size()) {
        return false;
    }
    for (auto index = std::size_t{}; index < cBoundary.size(); ++index) {
        if (data.span()[offset + index].toUInt8() != cBoundary[index]) {
            return false;
        }
    }
    return true;
}

}
