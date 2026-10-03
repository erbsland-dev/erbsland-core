// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathGlobCompiler.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../err/ParseError.hpp"
#include "../../text/impl/DynamicStringPatternData.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringJoins.hpp"

#include <cstddef>
#include <utility>

namespace erbsland::path::impl {

using namespace text;
using namespace text::literals;
using Flag = PathGlobPatternFlag;
using ElementKind = text::impl::StringPatternElementKind;

PathGlobCompiler::PathGlobCompiler(const AnyString &source, const PathGlobPatternFlags flags) :
    _reader{source}, _flags{flags} {
    if (flags.isSet(Flag::SemicolonSeparator) && !flags.isSet(Flag::MultiplePatterns)) {
        throw err::ParameterFailure{"SemicolonSeparator requires MultiplePatterns"_el, "flags"_el};
    }
}

void PathGlobCompiler::fail(String reason) const {
    throw err::ParseError{std::move(reason), _reader.position()};
}

void PathGlobCompiler::consumeNewline() {
    if (_reader.readIf(U'\r') && _reader.peek() != U'\n') {
        fail("A carriage return must be followed by a newline"_el);
    }
    _reader.readIf(U'\n');
    _lineStart = true;
}

auto PathGlobCompiler::parse() -> std::unique_ptr<PathGlobData> {
    auto data = std::make_unique<PathGlobData>();
    while (!_reader.isAtEnd()) {
        if (_flags.isSet(Flag::TrimWhitespace)) {
            while (_reader.peek() == U' ' || _reader.peek() == U'\t') {
                _reader.advance();
            }
        }
        if (_reader.isAtEnd()) {
            break;
        }
        if (_lineStart && _flags.isSet(Flag::IgnoreComments) && _reader.peek() == U'#') {
            while (!_reader.isAtEnd() && _reader.peek() != U'\n' && _reader.peek() != U'\r') {
                _reader.advance();
            }
            if (!_reader.isAtEnd()) {
                consumeNewline();
            }
            continue;
        }
        if (_reader.peek() == U'\n' || _reader.peek() == U'\r') {
            if (!_flags.isSet(Flag::MultiplePatterns)) {
                fail("Multiple path patterns are disabled"_el);
            }
            consumeNewline();
            continue;
        }
        if (_reader.peek() == U';') {
            if (!_flags.isSet(Flag::SemicolonSeparator)) {
                fail("Semicolon pattern separators are disabled"_el);
            }
            _reader.advance();
            _lineStart = false;
            continue;
        }
        data->alternatives.push_back(parseAlternative());
        _lineStart = false;
    }
    if (data->alternatives.empty()) {
        fail("The path glob contains no patterns"_el);
    }
    return data;
}

auto PathGlobCompiler::parseComponent() -> PathGlobData::Component {
    auto component = PathGlobData::Component{};
    component.position = _reader.position();
    auto data = text::impl::DynamicStringPatternData::parse(
        _reader, text::impl::StringPatternParseMode::PartialPathGlob, _flags.isSet(Flag::TrimWhitespace));
    const auto view = data->view();
    auto isLiteral = true;
    for (const auto &element : view.elements) {
        if (element.kind != ElementKind::Character) {
            isLiteral = false;
        } else if (element.character.isNull() || element.character.isReplacement()) {
            fail("A path pattern contains an invalid character"_el);
        }
    }
    if (view.elements.size() == 1U && view.elements.front().kind == ElementKind::RecursiveStar) {
        if (!_flags.isSet(Flag::RecursiveWildcard)) {
            fail("Recursive wildcards are disabled"_el);
        }
        component.recursive = true;
    } else if (isLiteral) {
        auto literal = StringEditor{};
        for (const auto &element : view.elements) {
            literal.append(element.character);
        }
        component.literal = literal;
    } else {
        component.pattern = std::move(data);
    }
    return component;
}

auto PathGlobCompiler::parseAlternative() -> PathGlobData::Alternative {
    auto alternative = PathGlobData::Alternative{};
    auto unc = false;
    if (_reader.readIf(U'/')) {
        alternative.anchored = true;
        alternative.root = "/"_el;
        unc = _reader.readIf(U'/');
        if (unc && _reader.peek() == U'/') {
            fail("A UNC root must have a server and share"_el);
        }
    }
    auto trailingSlash = false;
    auto firstHadSlash = false;
    while (!_reader.isAtEnd() && _reader.peek() != U';' && _reader.peek() != U'\n' && _reader.peek() != U'\r') {
        auto component = parseComponent();
        const auto empty = !component.recursive && component.pattern == nullptr && component.literal.isEmpty();
        if (!empty) {
            alternative.hasRecursiveWildcard = alternative.hasRecursiveWildcard || component.recursive;
            alternative.components.push_back(std::move(component));
            trailingSlash = false;
        }
        if (!_reader.readIf(U'/')) {
            break;
        }
        trailingSlash = true;
        if (alternative.components.size() == 1U) {
            firstHadSlash = true;
        }
    }
    if (unc || firstHadSlash) {
        extractRoot(alternative, unc);
    }
    if (trailingSlash) {
        fail("A path glob must not end with a slash"_el);
    }
    return alternative;
}

void PathGlobCompiler::extractRoot(PathGlobData::Alternative &alternative, const bool unc) {
    auto &components = alternative.components;
    if (unc) {
        if (components.size() < 2U || components[0].literal.isEmpty() || components[1].literal.isEmpty()) {
            fail("A UNC root requires a literal server and share"_el);
        }
        const auto root = Path{text::join("//"_el, components[0].literal, "/"_el, components[1].literal)};
        if (root.isEmpty() || !root.isRoot()) {
            fail("The UNC root is invalid"_el);
        }
        alternative.root = root.root();
        components.erase(components.begin(), components.begin() + 2);
        return;
    }
    if (!alternative.root.isEmpty() || components.empty()) {
        return;
    }
    if (components.front().pattern != nullptr) {
        const auto tokens = components.front().pattern->view().elements;
        if (!tokens.empty() && tokens.back().kind == ElementKind::Character && tokens.back().character == U':') {
            fail("A drive root must be literal"_el);
        }
    }
    const auto &drive = components.front().literal;
    if (drive.characterLength() == unit::CpCount{2U} && drive[unit::CpIndex{0U}].isAsciiLetter() &&
        drive[unit::CpIndex{1U}] == U':') {
        alternative.root = Path{text::join(drive, "/"_el)}.root();
        alternative.anchored = true;
        components.erase(components.begin());
    }
}

auto PathGlobCompiler::compile() -> PathGlobPatternPtr {
    return PathGlobPatternPtr{new PathGlobPattern{parse()}};
}

auto PathGlobCompiler::compileForWalk(const bool allowWildcards) -> WalkPattern {
    auto data = parse();
    if (data->alternatives.size() != 1U) {
        fail("A file include requires exactly one pattern"_el);
    }
    auto &alternative = data->alternatives.front();
    auto elements = StringList{};
    if (!alternative.root.isEmpty()) {
        elements.append(alternative.root);
    }
    auto prefixCount = std::size_t{};
    for (const auto &component : alternative.components) {
        if (component.recursive || component.pattern != nullptr) {
            break;
        }
        elements.append(component.literal);
        ++prefixCount;
    }
    auto result = WalkPattern{};
    if (!elements.isEmpty()) {
        result.prefix = Path::fromElements(elements);
        if (result.prefix.isEmpty()) {
            fail("The literal include path is invalid"_el);
        }
    }
    if (prefixCount == alternative.components.size()) {
        return result;
    }
    if (!allowWildcards) {
        fail("Wildcards are disabled for file includes"_el);
    }
    for (auto i = prefixCount; i < alternative.components.size(); ++i) {
        const auto &component = alternative.components[i];
        if (component.literal == "."_el || component.literal == ".."_el) {
            throw err::ParseError{"Path navigation must precede wildcard components"_el, component.position};
        }
    }
    alternative.components.erase(
        alternative.components.begin(), alternative.components.begin() + static_cast<std::ptrdiff_t>(prefixCount));
    alternative.root = {};
    alternative.anchored = false;
    result.pattern = PathGlobPatternPtr{new PathGlobPattern{std::move(data)}};
    return result;
}

}
