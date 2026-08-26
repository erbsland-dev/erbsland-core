// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::render::impl {

/// An instruction opcode in a compiled layout program.
enum class Opcode : uint8_t {
    EmitText = 0x01U,               ///< Append one text constant; operand: constant index.
    LoadName = 0x02U,               ///< Push one named value; operand: name constant index.
    GetMember = 0x03U,              ///< Replace the top value with a map member; operand: name constant index.
    EmitValue = 0x04U,              ///< Pop and append one printable value.
    End = 0x05U,                    ///< End the current program.
    PushTrue = 0x06U,               ///< Push boolean true.
    PushFalse = 0x07U,              ///< Push boolean false.
    PushInteger = 0x08U,            ///< Push a signed integer; operand: fixed-width bit representation.
    PushFloat = 0x09U,              ///< Push a float; operand: fixed-width bit representation.
    PushText = 0x0aU,               ///< Push one text constant; operand: constant index.
    LogicalNot = 0x0bU,             ///< Replace the top value with its logical negation.
    Equal = 0x0cU,                  ///< Compare two values for equality.
    NotEqual = 0x0dU,               ///< Compare two values for inequality.
    Greater = 0x0eU,                ///< Compare two values using greater-than.
    GreaterEqual = 0x0fU,           ///< Compare two values using greater-than-or-equal.
    Less = 0x10U,                   ///< Compare two values using less-than.
    LessEqual = 0x11U,              ///< Compare two values using less-than-or-equal.
    ApplyApplicationFilter = 0x12U, ///< Apply an application filter; packed name/count operand.
    StoreName = 0x13U,              ///< Store a render-local value; operand: name constant index.
    Jump = 0x14U,                   ///< Jump unconditionally; operand: absolute bytecode position.
    JumpIfFalse = 0x15U,            ///< Pop and jump if the value is false.
    JumpIfFalseOrPop = 0x16U,       ///< Jump with a false value, otherwise pop it.
    JumpIfTrueOrPop = 0x17U,        ///< Jump with a true value, otherwise pop it.
    BeginListIteration = 0x18U,     ///< Pop a list and open an iteration frame.
    BeginMapIteration = 0x19U,      ///< Pop a map and open an iteration frame.
    NextIteration = 0x1aU,          ///< Advance an iteration or jump to its end.
    StoreScopedName = 0x1bU,        ///< Store a value in the innermost iteration scope.
    EndIteration = 0x1cU,           ///< Close an exhausted iteration frame.
    Include = 0x1dU,                ///< Execute one static include; operand: include descriptor index.
    RenderBlock = 0x1eU,            ///< Render a resolved named block; operand: name constant index.
    LoadSuper = 0x1fU,              ///< Push a lazy super request; operand: number of implementations to skip.
    PushNull = 0x20U,               ///< Push a null value.
    UnaryPlus = 0x21U,              ///< Apply numeric unary plus.
    UnaryMinus = 0x22U,             ///< Apply saturating numeric unary minus.
    Add = 0x23U,                    ///< Add two numbers.
    Subtract = 0x24U,               ///< Subtract two numbers.
    Multiply = 0x25U,               ///< Multiply two numbers.
    Divide = 0x26U,                 ///< Divide two numbers, producing null for a zero divisor.
    Concatenate = 0x27U,            ///< Concatenate two renderer scalar strings.
    BuildList = 0x28U,              ///< Build a list; operand: number of stack values.
    BuildMap = 0x29U,               ///< Build a map; operand: number of key/value pairs.
    In = 0x2aU,                     ///< Test supported collection membership.
    NotIn = 0x2bU,                  ///< Negate supported collection membership.
    Is = 0x2cU,                     ///< Apply a value test; operand: ValueTest raw value.
    IsNot = 0x2dU,                  ///< Negate a value test; operand: ValueTest raw value.
    EndIterationIfNotEmpty = 0x2eU, ///< Close an iteration and skip its else body if it produced items.
    EmitEscapedValue = 0x2fU,       ///< Pop and append one printable value using an escape-format operand.
    ApplyBuiltInFilter = 0x30U,     ///< Apply a built-in filter; packed identifier/count operand.
};

}
