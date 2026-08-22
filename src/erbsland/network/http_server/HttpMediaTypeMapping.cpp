// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpMediaTypeMapping.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../text/CaseSensitivity.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"

#include <algorithm>

namespace erbsland::network {

using namespace text;
using namespace text::literals;

HttpMediaTypeMapping::HttpMediaTypeMapping() :
    _fallback{HttpMediaType::fromStringOrThrow("application/octet-stream"_el)} {
}

auto HttpMediaTypeMapping::create() -> HttpMediaTypeMappingPtr {
    return HttpMediaTypeMappingPtr{new HttpMediaTypeMapping{}};
}

auto HttpMediaTypeMapping::defaultMapping() -> HttpMediaTypeMappingConstPtr {
    static const auto instance = []() -> HttpMediaTypeMappingConstPtr {
        auto result = create();
        result->setSuffix(".html"_el, "text/html"_el)
            .setSuffix(".htm"_el, "text/html"_el)
            .setSuffix(".css"_el, "text/css"_el)
            .setSuffix(".js"_el, "text/javascript"_el)
            .setSuffix(".mjs"_el, "text/javascript"_el)
            .setSuffix(".json"_el, "application/json"_el)
            .setSuffix(".map"_el, "application/json"_el)
            .setSuffix(".xml"_el, "application/xml"_el)
            .setSuffix(".txt"_el, "text/plain"_el)
            .setSuffix(".svg"_el, "image/svg+xml"_el)
            .setSuffix(".png"_el, "image/png"_el)
            .setSuffix(".jpg"_el, "image/jpeg"_el)
            .setSuffix(".jpeg"_el, "image/jpeg"_el)
            .setSuffix(".gif"_el, "image/gif"_el)
            .setSuffix(".webp"_el, "image/webp"_el)
            .setSuffix(".avif"_el, "image/avif"_el)
            .setSuffix(".ico"_el, "image/x-icon"_el)
            .setSuffix(".pdf"_el, "application/pdf"_el)
            .setSuffix(".wasm"_el, "application/wasm"_el)
            .setSuffix(".woff"_el, "font/woff"_el)
            .setSuffix(".woff2"_el, "font/woff2"_el)
            .setSuffix(".ttf"_el, "font/ttf"_el)
            .setSuffix(".otf"_el, "font/otf"_el);
        return result;
    }();
    return instance;
}

auto HttpMediaTypeMapping::fallbackMediaType() const -> HttpMediaType {
    const auto lock = std::scoped_lock{_mutex};
    return _fallback;
}

auto HttpMediaTypeMapping::setFallbackMediaType(HttpMediaType mediaType) -> HttpMediaTypeMapping & {
    verifyMediaType(mediaType);
    const auto lock = std::scoped_lock{_mutex};
    verifyMutable();
    _fallback = std::move(mediaType);
    return *this;
}

auto HttpMediaTypeMapping::setFallbackMediaType(String mediaType) -> HttpMediaTypeMapping & {
    return setFallbackMediaType(HttpMediaType::fromStringOrThrow(mediaType));
}

auto HttpMediaTypeMapping::setSuffix(String suffix, HttpMediaType mediaType) -> HttpMediaTypeMapping & {
    suffix = normalizedSuffix(std::move(suffix));
    verifyMediaType(mediaType);
    const auto lock = std::scoped_lock{_mutex};
    verifyMutable();
    const auto compare = cCaseInsensitive.asciiComparisonFn();
    for (auto &[existingSuffix, existingMediaType] : _entries) {
        if (existingSuffix.compare(suffix, compare) == std::strong_ordering::equal) {
            existingMediaType = std::move(mediaType);
            return *this;
        }
    }
    if (_entries.size() >= cMaximumSuffixCount.toSizeT()) {
        throw err::ParameterError{"The HTTP media-type mapping contains too many suffixes."_el, "suffix"_el};
    }
    _entries.emplace_back(std::move(suffix), std::move(mediaType));
    return *this;
}

auto HttpMediaTypeMapping::setSuffix(String suffix, String mediaType) -> HttpMediaTypeMapping & {
    return setSuffix(std::move(suffix), HttpMediaType::fromStringOrThrow(mediaType));
}

auto HttpMediaTypeMapping::removeSuffix(const String &suffix) -> HttpMediaTypeMapping & {
    const auto normalized = normalizedSuffix(suffix);
    const auto compare = cCaseInsensitive.asciiComparisonFn();
    const auto lock = std::scoped_lock{_mutex};
    verifyMutable();
    std::erase_if(_entries, [&](const Entry &entry) -> bool {
        return entry.first.compare(normalized, compare) == std::strong_ordering::equal;
    });
    return *this;
}

auto HttpMediaTypeMapping::clear() -> HttpMediaTypeMapping & {
    const auto lock = std::scoped_lock{_mutex};
    verifyMutable();
    _entries.clear();
    return *this;
}

auto HttpMediaTypeMapping::mediaType(const String &path) const -> HttpMediaType {
    const auto compare = cCaseInsensitive.asciiComparisonFn();
    const auto lock = std::scoped_lock{_mutex};
    const Entry *best = nullptr;
    for (const auto &entry : _entries) {
        if (path.endsWith(entry.first, compare) && (best == nullptr || entry.first.length() > best->first.length())) {
            best = &entry;
        }
    }
    return best == nullptr ? _fallback : best->second;
}

auto HttpMediaTypeMapping::copy() const -> HttpMediaTypeMappingPtr {
    auto result = create();
    const auto lock = std::scoped_lock{_mutex};
    result->_entries = _entries;
    result->_fallback = _fallback;
    return result;
}

auto HttpMediaTypeMapping::normalizedSuffix(String suffix) -> String {
    if (!suffix.isValidUtf8() || suffix.isEmpty() || suffix.length() > cMaximumSuffixLength ||
        !suffix.startsWith("."_el)) {
        throw err::ParameterError{
            "An HTTP media-type suffix must start with a dot and remain within limits."_el, "suffix"_el};
    }
    auto reader = StringCharReader{suffix};
    reader.advance();
    if (reader.isAtEnd()) {
        throw err::ParameterError{"An HTTP media-type suffix requires text after its dot."_el, "suffix"_el};
    }
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!character.isAsciiAlphanumeric() && character != U'.' && character != U'+' && character != U'_' &&
            character != U'-') {
            throw err::ParameterError{"An HTTP media-type suffix contains an invalid character."_el, "suffix"_el};
        }
    }
    return suffix;
}

void HttpMediaTypeMapping::verifyMediaType(const HttpMediaType &mediaType) {
    if (!mediaType.isValid()) {
        throw err::ParameterError{"An HTTP media-type mapping requires a valid media type."_el, "mediaType"_el};
    }
}

void HttpMediaTypeMapping::verifyMutable() const {
    if (_activeUseCount != 0U) {
        throw err::LogicError{"An HTTP media-type mapping cannot change while a static-content handler is active."_el};
    }
}

void HttpMediaTypeMapping::beginUse() const {
    const auto lock = std::scoped_lock{_mutex};
    ++_activeUseCount;
}

void HttpMediaTypeMapping::endUse() const noexcept {
    const auto lock = std::scoped_lock{_mutex};
    if (_activeUseCount != 0U) {
        --_activeUseCount;
    }
}

}
