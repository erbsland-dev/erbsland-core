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

ExpressionCompiler::ExpressionCompiler(
    String layout,
    String origin,
    Tokenizer &tokenizer,
    StringList &constants,
    ProgramWriter &writer,
    const StringMap<FilterFn> &filters,
    const bool allowSuper,
    const bool allowOutputModifiers,
    const EscapeFormat outputFormat) :
    _layout{std::move(layout)},
    _origin{std::move(origin)},
    _tokenizer{tokenizer},
    _constants{constants},
    _writer{writer},
    _applicationFilters{filters},
    _allowSuper{allowSuper},
    _allowOutputModifiers{allowOutputModifiers},
    _outputFormat{outputFormat} {
}

void ExpressionCompiler::compile() {
    requireSupportedToken();
    if (token().kind == TokenKind::TagEnd) {
        throwSyntax("An expression must not be empty."_el, token().location);
    }
    parseOr();
    if (token().kind != TokenKind::TagEnd) {
        throwSyntax("Unexpected token after the expression."_el, token().location);
    }
}

auto ExpressionCompiler::outputFormat() const noexcept -> EscapeFormat {
    return _containsSuper && !_superTransformed ? EscapeFormat{EscapeFormat::None} : _outputFormat;
}

void ExpressionCompiler::parseOr() {
    parseAnd();
    while (token().kind == TokenKind::Or) {
        const auto location = token().location;
        nextToken();
        const auto jump = _writer.writeJumpIfTrueOrPop(location);
        parseAnd();
        _writer.patchJump(jump);
        if (_containsSuper) {
            _superTransformed = true;
        }
    }
}

void ExpressionCompiler::parseAnd() {
    parseNot();
    while (token().kind == TokenKind::And) {
        const auto location = token().location;
        nextToken();
        const auto jump = _writer.writeJumpIfFalseOrPop(location);
        parseNot();
        _writer.patchJump(jump);
        if (_containsSuper) {
            _superTransformed = true;
        }
    }
}

void ExpressionCompiler::parseNot() {
    if (token().kind == TokenKind::Not) {
        const auto location = token().location;
        nextToken();
        parseNot();
        _writer.writeLogicalNot(location);
        if (_containsSuper) {
            _superTransformed = true;
        }
        return;
    }
    parseComparison();
}

void ExpressionCompiler::parseComparison() {
    parseAdditive();
    if (token().kind == TokenKind::Is) {
        const auto location = token().location;
        nextToken();
        auto negate = false;
        if (token().kind == TokenKind::Not) {
            negate = true;
            nextToken();
        }
        if (!token().isIdentifier() && token().kind != TokenKind::Null && token().kind != TokenKind::True &&
            token().kind != TokenKind::False) {
            throwSyntax("The 'is' operator requires an argument-free built-in test name."_el, token().location);
        }
        const auto test = valueTest(token().text);
        if (!test.has_value()) {
            throwSyntax("The expression references an unknown value test."_el, token().location);
        }
        nextToken();
        _writer.writeTest(negate ? Opcode::IsNot : Opcode::Is, *test, location);
    } else if (token().kind == TokenKind::In || token().kind == TokenKind::Not) {
        const auto location = token().location;
        auto negate = false;
        if (token().kind == TokenKind::Not) {
            negate = true;
            nextToken();
            if (token().kind != TokenKind::In) {
                throwSyntax("The comparison keyword 'not' must be followed by 'in'."_el, token().location);
            }
        }
        nextToken();
        parseAdditive();
        _writer.writeBinary(negate ? Opcode::NotIn : Opcode::In, location);
    } else if (isComparison(token().kind)) {
        const auto opcode = comparisonOpcode(token().kind);
        const auto location = token().location;
        nextToken();
        parseAdditive();
        _writer.writeComparison(opcode, location);
    } else {
        return;
    }
    if (_containsSuper) {
        _superTransformed = true;
    }
    if (isComparison(token().kind) || token().kind == TokenKind::In || token().kind == TokenKind::Is ||
        token().kind == TokenKind::Not) {
        throwSyntax("Chained comparisons are not supported; combine comparisons with 'and'."_el, token().location);
    }
}

void ExpressionCompiler::parseAdditive() {
    parseMultiplicative();
    while (
        token().kind == TokenKind::Plus || token().kind == TokenKind::Minus || token().kind == TokenKind::Concatenate) {
        const auto kind = token().kind;
        const auto location = token().location;
        nextToken();
        parseMultiplicative();
        const auto opcode =
            kind == TokenKind::Plus ? Opcode::Add : (kind == TokenKind::Minus ? Opcode::Subtract : Opcode::Concatenate);
        _writer.writeBinary(opcode, location);
        if (_containsSuper) {
            _superTransformed = true;
        }
    }
}

void ExpressionCompiler::parseMultiplicative() {
    parseUnary();
    while (token().kind == TokenKind::Multiply || token().kind == TokenKind::Divide) {
        const auto kind = token().kind;
        const auto location = token().location;
        nextToken();
        parseUnary();
        _writer.writeBinary(kind == TokenKind::Multiply ? Opcode::Multiply : Opcode::Divide, location);
        if (_containsSuper) {
            _superTransformed = true;
        }
    }
}

void ExpressionCompiler::parseUnary() {
    if (token().kind == TokenKind::Plus || token().kind == TokenKind::Minus) {
        const auto kind = token().kind;
        const auto location = token().location;
        nextToken();
        if (kind == TokenKind::Minus && token().kind == TokenKind::Integer &&
            token().text == "9223372036854775808"_el) {
            _writer.writePushInteger(std::numeric_limits<int64_t>::min(), location);
            nextToken();
            return;
        }
        parseUnary();
        _writer.writeUnary(kind == TokenKind::Plus ? Opcode::UnaryPlus : Opcode::UnaryMinus, location);
        if (_containsSuper) {
            _superTransformed = true;
        }
        return;
    }
    parseFiltered();
}
void ExpressionCompiler::parsePrimary() {
    const auto currentToken = token();
    switch (currentToken.kind) {
    case TokenKind::Identifier:
        parseNameOrSuper();
        return;
    case TokenKind::String:
        _writer.writePushText(addConstant(currentToken.text), currentToken.location);
        nextToken();
        return;
    case TokenKind::Integer:
        try {
            _writer.writePushInteger(currentToken.text.toIntegerOrThrow<int64_t>(), currentToken.location);
        } catch (const err::Exception &) {
            throwSyntax("The integer literal is outside the signed 64-bit range."_el, currentToken.location);
        }
        nextToken();
        return;
    case TokenKind::Float:
        try {
            const auto value = currentToken.text.toFloatOrThrow<double>();
            if (!std::isfinite(value)) {
                throwSyntax("The floating-point literal must be finite."_el, currentToken.location);
            }
            _writer.writePushFloat(value, currentToken.location);
        } catch (const RenderError &) {
            throw;
        } catch (const err::Exception &) {
            throwSyntax("The floating-point literal is outside the supported range."_el, currentToken.location);
        }
        nextToken();
        return;
    case TokenKind::True:
        _writer.writePushTrue(currentToken.location);
        nextToken();
        return;
    case TokenKind::False:
        _writer.writePushFalse(currentToken.location);
        nextToken();
        return;
    case TokenKind::Null:
        _writer.writePushNull(currentToken.location);
        nextToken();
        return;
    case TokenKind::LeftParen:
        nextToken();
        parseOr();
        if (token().kind != TokenKind::RightParen) {
            throwSyntax("A grouped expression requires a closing parenthesis."_el, token().location);
        }
        nextToken();
        return;
    case TokenKind::LeftBracket:
        parseList();
        return;
    case TokenKind::LeftBrace:
        parseMap();
        return;
    default:
        throwSyntax("Expected a literal, name, collection, or grouped expression."_el, currentToken.location);
    }
}

void ExpressionCompiler::parseNameOrSuper() {
    const auto first = token();
    nextToken();
    if (first.text != "super"_el) {
        _writer.writeLoadName(addConstant(first.text), first.location);
        while (token().kind == TokenKind::Dot) {
            nextToken();
            if (!token().isIdentifier()) {
                throwSyntax("A member-access dot must be followed by an identifier."_el, token().location);
            }
            _writer.writeGetMember(addConstant(token().text), token().location);
            nextToken();
        }
        return;
    }

    auto depth = std::size_t{1U};
    if (token().kind == TokenKind::Dot) {
        nextToken();
        if (!token().isIdentifier()) {
            throwSyntax("A member-access dot must be followed by an identifier."_el, token().location);
        }
        const auto member = token();
        nextToken();
        if (member.text != "super"_el || token().kind != TokenKind::LeftParen) {
            _writer.writeLoadName(addConstant(first.text), first.location);
            _writer.writeGetMember(addConstant(member.text), member.location);
            while (token().kind == TokenKind::Dot) {
                nextToken();
                if (!token().isIdentifier()) {
                    throwSyntax("A member-access dot must be followed by an identifier."_el, token().location);
                }
                _writer.writeGetMember(addConstant(token().text), token().location);
                nextToken();
            }
            return;
        }
        depth = 2U;
    } else if (token().kind != TokenKind::LeftParen) {
        _writer.writeLoadName(addConstant(first.text), first.location);
        return;
    }

    if (!_allowSuper) {
        throwSyntax("A 'super' call is allowed only inside a block body."_el, first.location);
    }
    nextToken();
    if (token().kind != TokenKind::RightParen) {
        throwSyntax("A 'super' call does not accept arguments."_el, token().location);
    }
    nextToken();
    _containsSuper = true;
    _writer.writeLoadSuper(depth, first.location);
}

void ExpressionCompiler::parseList() {
    const auto location = token().location;
    nextToken();
    auto count = std::size_t{};
    if (token().kind != TokenKind::RightBracket) {
        while (true) {
            parseOr();
            ++count;
            if (token().kind != TokenKind::Comma) {
                break;
            }
            nextToken();
            if (token().kind == TokenKind::RightBracket) {
                break;
            }
        }
    }
    if (token().kind != TokenKind::RightBracket) {
        throwSyntax("A list literal requires a closing bracket."_el, token().location);
    }
    nextToken();
    _writer.writeBuildList(count, location);
}

void ExpressionCompiler::parseMap() {
    const auto location = token().location;
    nextToken();
    auto count = std::size_t{};
    auto keys = StringMap<bool>{};
    if (token().kind != TokenKind::RightBrace) {
        while (true) {
            if (token().kind != TokenKind::String) {
                throwSyntax("A map literal key must be a string literal."_el, token().location);
            }
            const auto key = token().text;
            if (keys.contains(key)) {
                throwSyntax("A map literal must not contain duplicate keys."_el, token().location);
            }
            keys.set(key, true);
            _writer.writePushText(addConstant(key), token().location);
            nextToken();
            if (token().kind != TokenKind::Colon) {
                throwSyntax("A map literal key must be followed by a colon."_el, token().location);
            }
            nextToken();
            parseOr();
            ++count;
            if (token().kind != TokenKind::Comma) {
                break;
            }
            nextToken();
            if (token().kind == TokenKind::RightBrace) {
                break;
            }
        }
    }
    if (token().kind != TokenKind::RightBrace) {
        throwSyntax("A map literal requires a closing brace."_el, token().location);
    }
    nextToken();
    _writer.writeBuildMap(count, location);
}
void ExpressionCompiler::nextToken() {
    _tokenizer.advance();
    requireSupportedToken();
}

void ExpressionCompiler::requireSupportedToken() const {
    if (token().kind == TokenKind::Unsupported) {
        throwSyntax("The expression contains an unsupported character."_el, token().location);
    }
}

auto ExpressionCompiler::isComparison(const TokenKind kind) noexcept -> bool {
    return kind == TokenKind::Equal || kind == TokenKind::NotEqual || kind == TokenKind::Greater ||
        kind == TokenKind::GreaterEqual || kind == TokenKind::Less || kind == TokenKind::LessEqual;
}

auto ExpressionCompiler::comparisonOpcode(const TokenKind kind) const -> Opcode {
    switch (kind) {
    case TokenKind::Equal:
        return Opcode::Equal;
    case TokenKind::NotEqual:
        return Opcode::NotEqual;
    case TokenKind::Greater:
        return Opcode::Greater;
    case TokenKind::GreaterEqual:
        return Opcode::GreaterEqual;
    case TokenKind::Less:
        return Opcode::Less;
    case TokenKind::LessEqual:
        return Opcode::LessEqual;
    default:
        throwSyntax("Expected a comparison operator."_el, token().location);
    }
}

auto ExpressionCompiler::valueTest(const String &name) noexcept -> std::optional<ValueTest> {
    if (name == "none"_el || name == "null"_el)
        return ValueTest::Null;
    if (name == "true"_el)
        return ValueTest::True;
    if (name == "false"_el)
        return ValueTest::False;
    if (name == "boolean"_el)
        return ValueTest::Boolean;
    if (name == "integer"_el)
        return ValueTest::Integer;
    if (name == "float"_el)
        return ValueTest::Float;
    if (name == "number"_el)
        return ValueTest::Number;
    if (name == "text"_el || name == "string"_el)
        return ValueTest::Text;
    if (name == "list"_el || name == "sequence"_el)
        return ValueTest::List;
    if (name == "map"_el || name == "mapping"_el)
        return ValueTest::Map;
    if (name == "iterable"_el)
        return ValueTest::Iterable;
    if (name == "scalar"_el)
        return ValueTest::Scalar;
    if (name == "even"_el)
        return ValueTest::Even;
    if (name == "odd"_el)
        return ValueTest::Odd;
    return std::nullopt;
}

auto ExpressionCompiler::addConstant(String value) -> std::size_t {
    const auto result = _constants.count().toSizeT();
    _constants.append(std::move(value));
    return result;
}

void ExpressionCompiler::throwSyntax(String description, const unit::CodeLocation location) const {
    throw RenderError{
        RenderErrorContext{RenderErrorCategory::Syntax, "Invalid layout expression"_el, std::move(description)}
            .setLayout(_layout)
            .setOrigin(_origin)
            .setLocation(location)};
}

}
