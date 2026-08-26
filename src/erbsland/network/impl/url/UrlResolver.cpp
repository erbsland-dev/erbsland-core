// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UrlResolver.hpp"

#include "UrlParser.hpp"

#include "../../../err/ParseError.hpp"
#include "../../../text/AnyString.hpp"
#include "../../../text/AsciiCategory.hpp"
#include "../../../text/Char.hpp"
#include "../../../text/CharSet.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringSide.hpp"
#include "../../../unit/ByteIndex.hpp"

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

auto UrlResolver::resolve() const -> std::shared_ptr<UrlData> {
    if (_reference.length() > _options.maximumLength()) {
        throw err::ParseError{"The URI reference exceeds the configured length limit."_el};
    }
    if (!_reference.isValidUtf8()) {
        throw err::ParseError{"The URI reference is not valid UTF-8."_el};
    }
    if (!hasHierarchicalBase()) {
        throw err::ParseError{"This base URL does not support hierarchical reference resolution."_el};
    }
    if (hasScheme()) {
        return parseAbsolute(_reference);
    }
    if (_reference.startsWith("//"_el)) {
        return parseAbsolute(join(join(_base.schemeText, ":"_el), _reference));
    }
    return resolveRelative(parseRelative());
}

auto UrlResolver::hasScheme() const noexcept -> bool {
    auto reader = StringCharReader{_reference};
    if (reader.isAtEnd() || !reader.read().isAsciiLetter()) {
        return false;
    }
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == U':') {
            return true;
        }
        if (character == U'/' || character == U'?' || character == U'#') {
            return false;
        }
        if (!character.isAsciiCategory(AsciiCategory::UrlScheme)) {
            return false;
        }
    }
    return false;
}

auto UrlResolver::hasHierarchicalBase() const noexcept -> bool {
    if (_base.scheme == UrlScheme::Invalid || _base.scheme == UrlScheme::Mailto) {
        return false;
    }
    return _base.scheme != UrlScheme::Custom || _base.hasAuthority;
}

auto UrlResolver::parseRelative() const -> Reference {
    auto result = Reference{};
    auto reader = StringCharReader{_reference};
    static const auto cPathEnd = CharSet{U'?', U'#'};
    static const auto cQueryEnd = CharSet{U'#'};

    reader.startCapture();
    reader.advanceUntil(cPathEnd);
    result.path = reader.takeCapture().toString();
    if (reader.advanceIf(U'?')) {
        result.hasQuery = true;
        reader.startCapture();
        reader.advanceUntil(cQueryEnd);
        result.query = reader.takeCapture().toString();
    }
    if (reader.advanceIf(U'#')) {
        result.hasFragment = true;
        reader.startCapture();
        reader.advanceUntil(CharSet{});
        result.fragment = reader.takeCapture().toString();
    }
    return result;
}

auto UrlResolver::resolveRelative(const Reference &reference) const -> std::shared_ptr<UrlData> {
    auto result = std::make_shared<UrlData>(_base);
    const auto decodedPath = UrlParser::decodeComponent(reference.path);
    if (decodedPath.isEmpty()) {
        if (reference.hasQuery) {
            result->query = UrlParser::decodeComponent(reference.query);
            result->hasQuery = true;
        }
    } else {
        result->path = removeDotSegments(decodedPath.startsWith("/"_el) ? decodedPath : mergePath(decodedPath));
        result->query = reference.hasQuery ? UrlParser::decodeComponent(reference.query) : String{};
        result->hasQuery = reference.hasQuery;
    }
    result->fragment = reference.hasFragment ? UrlParser::decodeComponent(reference.fragment) : String{};
    result->hasFragment = reference.hasFragment;
    return result;
}

auto UrlResolver::parseAbsolute(const String &text) const -> std::shared_ptr<UrlData> {
    auto result = UrlParser{text, _options}.parse();
    result->path = removeDotSegments(result->path);
    return result;
}

auto UrlResolver::mergePath(const String &path) const -> String {
    if (_base.hasAuthority && _base.path.isEmpty()) {
        return join("/"_el, path);
    }
    const auto separator = _base.path.findLastOf(CharSet{U'/'});
    if (separator.isNoIndex()) {
        return path;
    }
    return join(_base.path.slice(StringSide::Front, separator.advanced(unit::ByteLength::one())), path);
}

auto UrlResolver::removeDotSegments(String input) -> String {
    auto output = String{};
    while (!input.isEmpty()) {
        if (input.startsWith("../"_el)) {
            input = removePrefix(input, unit::ByteLength{3U});
            continue;
        }
        if (input.startsWith("./"_el)) {
            input = removePrefix(input, unit::ByteLength{2U});
            continue;
        }
        if (input.startsWith("/./"_el)) {
            input = join("/"_el, removePrefix(input, unit::ByteLength{3U}));
            continue;
        }
        if (input == "/."_el) {
            input = "/"_el;
            continue;
        }
        if (input.startsWith("/../"_el)) {
            input = join("/"_el, removePrefix(input, unit::ByteLength{4U}));
            output = removeLastSegment(output);
            continue;
        }
        if (input == "/.."_el) {
            input = "/"_el;
            output = removeLastSegment(output);
            continue;
        }
        if (input == "."_el || input == ".."_el) {
            input = {};
            continue;
        }
        const auto searchStart = input.startsWith("/"_el) ? unit::ByteIndex{1U} : unit::ByteIndex::zero();
        const auto separator = input.find("/"_el, searchStart);
        if (separator.isNoIndex()) {
            output = join(output, input);
            input = {};
        } else {
            output = join(output, input.slice(StringSide::Front, separator));
            input = input.slice(StringSide::Back, separator);
        }
    }
    return output;
}

auto UrlResolver::removeLastSegment(const String &path) -> String {
    const auto separator = path.findLastOf(CharSet{U'/'});
    return separator.isNoIndex() ? String{} : path.slice(StringSide::Front, separator);
}

auto UrlResolver::removePrefix(const String &text, const unit::ByteLength length) -> String {
    return text.slice(StringSide::Back, unit::ByteIndex::end(length));
}

auto UrlResolver::join(const String &first, const String &second) -> String {
    return String{StringEditor{first}.append(second)};
}

}
