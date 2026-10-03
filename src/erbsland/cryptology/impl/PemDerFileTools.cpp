// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PemDerFileTools.hpp"

#include "PemCodec.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../path/PathAccessProfile.hpp"
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
#include "../../unit/ByteCount.hpp"

#include <array>
#include <utility>

namespace erbsland::cryptology::impl {

using namespace err;
using namespace mem;
using namespace path;
using namespace text;
using namespace text::literals;

PemDerFileTools::PemDerFileTools(Path path, const Artifact artifact, const bool sensitive) noexcept :
    _path{std::move(path)}, _artifact{artifact}, _sensitive{sensitive} {
}

auto PemDerFileTools::read() const -> ByteBlock {
    auto options = PathReadDataOptions{};
    options.setMaximumByteLength(unit::ByteCount{PemCodec::cMaximumBundleTextLength});
    return PathContent{_path}.readDataOrThrow(options);
}

auto PemDerFileTools::inputFormat(const ByteBlock &data, const PemDerFormat requested) const -> PemDerFormat {
    if (requested != PemDerFormat::Automatic) {
        return requested;
    }
    const auto suffix = suffixFormat();
    if (suffix != PemDerFormat::Automatic) {
        if ((suffix == PemDerFormat::Pem) != looksLikePem(data)) {
            throw ParseError{"The artifact content does not match the format selected by its suffix."_el};
        }
        return suffix;
    }
    return looksLikePem(data) ? PemDerFormat::Pem : PemDerFormat::Der;
}

auto PemDerFileTools::outputFormat(const PemDerFormat requested) const -> PemDerFormat {
    if (requested != PemDerFormat::Automatic) {
        return requested;
    }
    const auto result = suffixFormat();
    if (result == PemDerFormat::Automatic) {
        throw ParameterFailure{"Automatic output requires a recognized artifact suffix."_el, "path"_el};
    }
    return result;
}

auto PemDerFileTools::toPemText(const ByteBlock &data) -> String {
    if (!data.isEmpty() && data.span().size() >= 3U && data.span()[0U].toUInt8() == 0xefU &&
        data.span()[1U].toUInt8() == 0xbbU && data.span()[2U].toUInt8() == 0xbfU) {
        throw ParseError{"PEM source must be BOM-free valid UTF-8."_el};
    }
    try {
        return StringDecoder{data}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
    } catch (const EncodingError &) {
        throw ParseError{"PEM source must be BOM-free valid UTF-8."_el};
    }
}

void PemDerFileTools::writePem(const String &text) const {
    auto options = PathWriteTextOptions{StringEncoding::Utf8};
    options.setBomMode(StringBomMode::Reject);
    if (_sensitive) {
        options.setAccessProfile(PathAccessProfile::UserOnly);
    }
    PathContent{_path}.writeTextOrThrow(text, options);
}

void PemDerFileTools::writeDer(const ByteBlock &data) const {
    auto options = PathWriteDataOptions{};
    if (_sensitive) {
        options.setAccessProfile(PathAccessProfile::UserOnly);
    }
    PathContent{_path}.writeDataOrThrow(data, options);
}

auto PemDerFileTools::suffixIs(const String &suffix) const noexcept -> bool {
    return _path.suffix().compare(suffix, Char::compareAsciiFolded) == std::strong_ordering::equal;
}

auto PemDerFileTools::suffixFormat() const noexcept -> PemDerFormat {
    if (suffixIs(".pem"_el)) {
        return PemDerFormat::Pem;
    }
    if (suffixIs(".der"_el)) {
        return PemDerFormat::Der;
    }
    switch (_artifact) {
    case Artifact::Certificate:
        if (suffixIs(".crt"_el)) {
            return PemDerFormat::Pem;
        }
        if (suffixIs(".cer"_el)) {
            return PemDerFormat::Der;
        }
        break;
    case Artifact::PrivateKey:
        if (suffixIs(".key"_el)) {
            return PemDerFormat::Pem;
        }
        if (suffixIs(".p8"_el)) {
            return PemDerFormat::Der;
        }
        break;
    case Artifact::PublicKey:
        if (suffixIs(".pub"_el)) {
            return PemDerFormat::Pem;
        }
        break;
    case Artifact::CertificateRequest:
        if (suffixIs(".csr"_el)) {
            return PemDerFormat::Pem;
        }
        if (suffixIs(".p10"_el)) {
            return PemDerFormat::Der;
        }
        break;
    }
    return PemDerFormat::Automatic;
}

auto PemDerFileTools::looksLikePem(const ByteBlock &data) noexcept -> bool {
    constexpr auto cBoundary = std::array<uint8_t, 11U>{'-', '-', '-', '-', '-', 'B', 'E', 'G', 'I', 'N', ' '};
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
