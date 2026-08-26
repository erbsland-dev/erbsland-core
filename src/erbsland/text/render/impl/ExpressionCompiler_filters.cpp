// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ExpressionCompiler.hpp"

#include "BuiltInFilters.hpp"

#include "../RenderError.hpp"

#include "../../../err/Exception.hpp"
#include "../../../text/Literals.hpp"

#include <cmath>
#include <limits>

namespace erbsland::text::render::impl {

using namespace text::literals;

void ExpressionCompiler::parseFiltered() {
    parsePrimary();
    while (token().kind == TokenKind::Pipe) {
        nextToken();
        if (!token().isIdentifier()) {
            throwSyntax("A filter pipe must be followed by a filter name."_el, token().location);
        }
        const auto name = token().text;
        const auto location = token().location;
        nextToken();
        if (name == "escape"_el || name == "e"_el || name == "safe"_el) {
            parseOutputModifier(name, location);
            continue;
        }
        const auto isApplicationFilter = _applicationFilters.contains(name);
        const auto builtInFilter = built_in_filters::find(name);
        if (!isApplicationFilter && !builtInFilter.has_value()) {
            throwSyntax("The expression references an unknown filter."_el, location);
        }
        const auto argumentCount = parseFilterArguments();
        if (isApplicationFilter) {
            _writer.writeApplicationFilter(addConstant(name), argumentCount, location);
        } else {
            _writer.writeBuiltInFilter(*builtInFilter, argumentCount, location);
        }
        if (_containsSuper) {
            _superTransformed = true;
        }
    }
}

auto ExpressionCompiler::parseFilterArguments() -> std::size_t {
    if (token().kind != TokenKind::LeftParen) {
        return 0U;
    }
    nextToken();
    if (token().kind == TokenKind::RightParen) {
        nextToken();
        return 0U;
    }
    auto count = std::size_t{};
    while (true) {
        parseOr();
        ++count;
        if (count > 2U) {
            throwSyntax("A filter accepts at most two arguments."_el, token().location);
        }
        if (token().kind != TokenKind::Comma) {
            break;
        }
        nextToken();
        if (token().kind == TokenKind::RightParen) {
            throwSyntax("A filter argument list must not end with a comma."_el, token().location);
        }
    }
    if (token().kind != TokenKind::RightParen) {
        throwSyntax("A filter argument list requires a closing parenthesis."_el, token().location);
    }
    nextToken();
    return count;
}

void ExpressionCompiler::parseOutputModifier(const String &name, const unit::CodeLocation location) {
    if (!_allowOutputModifiers) {
        throwSyntax(
            "The 'escape', 'e', and 'safe' modifiers are allowed only on direct output expressions."_el, location);
    }
    if (_containsSuper) {
        throwSyntax("Output modifiers cannot be applied to a super block result."_el, location);
    }
    if (name == "safe"_el) {
        if (token().kind == TokenKind::LeftParen) {
            nextToken();
            if (token().kind != TokenKind::RightParen) {
                throwSyntax("The 'safe' output modifier does not accept arguments."_el, token().location);
            }
            nextToken();
        }
        _outputFormat = EscapeFormat::None;
    } else {
        auto format = _outputFormat == EscapeFormat::None ? EscapeFormat{EscapeFormat::Html} : _outputFormat;
        if (token().kind == TokenKind::LeftParen) {
            nextToken();
            if (token().kind != TokenKind::RightParen) {
                if (token().kind != TokenKind::String && !token().isIdentifier()) {
                    throwSyntax("The 'escape' modifier requires a canonical format name."_el, token().location);
                }
                const auto parsed = EscapeFormat::fromString(token().text);
                if (!parsed.has_value()) {
                    throwSyntax("The 'escape' modifier references an unknown format."_el, token().location);
                }
                format = *parsed;
                nextToken();
            }
            if (token().kind != TokenKind::RightParen) {
                throwSyntax("The 'escape' modifier accepts at most one argument."_el, token().location);
            }
            nextToken();
        }
        _outputFormat = format;
    }
    if (token().kind != TokenKind::TagEnd) {
        throwSyntax("An output modifier must be the final filter of a direct output expression."_el, token().location);
    }
}

}
