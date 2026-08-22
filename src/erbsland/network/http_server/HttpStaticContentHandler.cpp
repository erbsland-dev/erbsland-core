// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticContentHandler.hpp"

#include "HttpMediaTypeMapping.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../text/CaseSensitivity.hpp"
#include "../../text/Literals.hpp"
#include "../../text/NormalizationForm.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"

namespace erbsland::network {

using namespace text::literals;

HttpStaticContentHandler::HttpStaticContentHandler(text::String urlPrefix) :
    _urlPrefix{canonicalUrlPrefix(std::move(urlPrefix))},
    _indexFileNames{{"index.html"_el, "index.htm"_el}},
    _mediaTypeMap{HttpMediaTypeMapping::defaultMapping()} {
}

auto HttpStaticContentHandler::urlPrefix() const -> text::String {
    const auto lock = std::scoped_lock{_mutex};
    return _urlPrefix;
}

auto HttpStaticContentHandler::setUrlPrefix(text::String value) -> HttpStaticContentHandler & {
    value = canonicalUrlPrefix(std::move(value));
    const auto lock = std::scoped_lock{_mutex};
    verifyConfigurationMutable();
    _urlPrefix = std::move(value);
    return *this;
}

auto HttpStaticContentHandler::priority() const noexcept -> std::int32_t {
    const auto lock = std::scoped_lock{_mutex};
    return _priority;
}

auto HttpStaticContentHandler::setPriority(const std::int32_t value) -> HttpStaticContentHandler & {
    const auto lock = std::scoped_lock{_mutex};
    verifyConfigurationMutable();
    _priority = value;
    return *this;
}

auto HttpStaticContentHandler::indexFileNames() const -> text::StringList {
    const auto lock = std::scoped_lock{_mutex};
    return _indexFileNames;
}

auto HttpStaticContentHandler::setIndexFileNames(text::StringList value) -> HttpStaticContentHandler & {
    if (value.count() > unit::ItemCount{16U}) {
        throw err::ParameterError{
            "A static-content handler accepts at most 16 index filenames."_el, "indexFileNames"_el};
    }
    for (const auto &name : value) {
        verifyIndexFileName(name);
    }
    const auto lock = std::scoped_lock{_mutex};
    verifyConfigurationMutable();
    _indexFileNames = std::move(value);
    return *this;
}

auto HttpStaticContentHandler::mediaTypeMapping() const -> HttpMediaTypeMappingConstPtr {
    const auto lock = std::scoped_lock{_mutex};
    return _mediaTypeMap;
}

auto HttpStaticContentHandler::setMediaTypeMapping(HttpMediaTypeMappingConstPtr value) -> HttpStaticContentHandler & {
    if (value == nullptr) {
        throw err::ParameterError{"A static-content handler requires a media-type mapping."_el, "mediaTypeMapping"_el};
    }
    const auto lock = std::scoped_lock{_mutex};
    verifyConfigurationMutable();
    _mediaTypeMap = std::move(value);
    return *this;
}

auto HttpStaticContentHandler::lockConfiguration() const -> std::unique_lock<std::mutex> {
    return std::unique_lock{_mutex};
}

void HttpStaticContentHandler::verifyConfigurationMutable() const {
    if (_activeUseCount != 0U) {
        throw err::LogicError{
            "Static-content handler configuration cannot change while a server is starting or active."_el};
    }
}

auto HttpStaticContentHandler::canonicalUrlPrefix(text::String value) -> text::String {
    if (!value.isValidUtf8() || !value.startsWith("/"_el) || value.contains("?"_el) || value.contains("#"_el)) {
        throw err::ParameterError{"A static-content URL prefix must be an absolute UTF-8 path."_el, "urlPrefix"_el};
    }
    value = value.normalized(text::NormalizationForm::Nfc);
    while (value.length() > unit::ByteLength::one() && value.endsWith("/"_el)) {
        value = value.slice(text::StringSide::Front, value.length() - unit::ByteLength::one());
    }
    return value;
}

void HttpStaticContentHandler::beginUse() {
    const auto lock = std::scoped_lock{_mutex};
    _mediaTypeMap->beginUse();
    ++_activeUseCount;
}

void HttpStaticContentHandler::endUse() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    if (_activeUseCount == 0U) {
        return;
    }
    --_activeUseCount;
    _mediaTypeMap->endUse();
}

void HttpStaticContentHandler::verifyIndexFileName(const text::String &name) {
    if (!name.isValidUtf8() || name.isEmpty() || name == "."_el || name == ".."_el || name.contains("/"_el) ||
        name.contains("\\"_el) || name.contains(":"_el) || name.endsWith("."_el) || name.endsWith(" "_el) ||
        name.length() > unit::ByteLength{255U}) {
        throw err::ParameterError{
            "A static-content index filename must be one safe path segment."_el, "indexFileNames"_el};
    }
    auto reader = text::StringCharReader{name};
    auto baseName = text::StringEditor{};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == U'\0') {
            throw err::ParameterError{
                "A static-content index filename must be one safe path segment."_el, "indexFileNames"_el};
        }
        if (character == U'.') {
            break;
        }
        baseName.append(character);
    }
    const auto base = text::String{baseName};
    const auto compare = text::cCaseInsensitive.asciiComparisonFn();
    const auto isDevicePrefix = [&](const text::StringLiteral &prefix) -> bool {
        return base.length() == unit::ByteLength{4U} && base.startsWith(prefix, compare) &&
            base.charAt(text::StringSide::Back).isAsciiDigit() && base.charAt(text::StringSide::Back) != U'0';
    };
    if (base.compare("con"_el, compare) == std::strong_ordering::equal ||
        base.compare("prn"_el, compare) == std::strong_ordering::equal ||
        base.compare("aux"_el, compare) == std::strong_ordering::equal ||
        base.compare("nul"_el, compare) == std::strong_ordering::equal || isDevicePrefix("com"_el) ||
        isDevicePrefix("lpt"_el)) {
        throw err::ParameterError{
            "A static-content index filename must be one safe path segment."_el, "indexFileNames"_el};
    }
}

}
