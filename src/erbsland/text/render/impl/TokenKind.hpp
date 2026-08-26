// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::render::impl {

/// The kind of one lexical layout token.
enum class TokenKind : uint8_t {
    End = 0U,             ///< The end of the layout source.
    Text = 1U,            ///< Raw layout text.
    ExpressionBegin = 2U, ///< The beginning of an output expression tag.
    StatementBegin = 3U,  ///< The beginning of a statement tag.
    TagEnd = 4U,          ///< The end of an expression or statement tag.
    Identifier = 5U,      ///< An ASCII identifier.
    String = 6U,          ///< A decoded text literal.
    Integer = 7U,         ///< A signed decimal integer literal.
    Float = 8U,           ///< A decimal floating-point literal.
    True = 9U,            ///< The `true` literal.
    False = 10U,          ///< The `false` literal.
    Null = 11U,           ///< The `none` or `null` keyword.
    LeftParen = 12U,      ///< A left parenthesis.
    RightParen = 13U,     ///< A right parenthesis.
    Dot = 14U,            ///< A member-access dot.
    Comma = 15U,          ///< A comma separating loop targets.
    Pipe = 16U,           ///< A filter-chain pipe.
    Assign = 17U,         ///< A single assignment character.
    Equal = 18U,          ///< The equality operator.
    NotEqual = 19U,       ///< The inequality operator.
    Greater = 20U,        ///< The greater-than operator.
    GreaterEqual = 21U,   ///< The greater-than-or-equal operator.
    Less = 22U,           ///< The less-than operator.
    LessEqual = 23U,      ///< The less-than-or-equal operator.
    And = 24U,            ///< The `and` operator.
    Or = 25U,             ///< The `or` operator.
    Not = 26U,            ///< The `not` operator.
    Unsupported = 27U,    ///< A character unsupported by the expression grammar.
    LeftBracket = 28U,    ///< A left square bracket.
    RightBracket = 29U,   ///< A right square bracket.
    LeftBrace = 30U,      ///< A left curly brace.
    RightBrace = 31U,     ///< A right curly brace.
    Colon = 32U,          ///< A map-key separator.
    Plus = 33U,           ///< The addition or unary-plus operator.
    Minus = 34U,          ///< The subtraction or unary-minus operator.
    Multiply = 35U,       ///< The multiplication operator.
    Divide = 36U,         ///< The division operator.
    Concatenate = 37U,    ///< The string-concatenation operator.
    In = 38U,             ///< The membership operator.
    Is = 39U,             ///< The value-test operator.
};

}
