// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Program.hpp"

#include <cstdint>

namespace erbsland::re::impl {

/// An operation in the program
class Operation final {
public:
    using ValueNative = uint8_t;

public:
    enum Value : ValueNative {
        None = 0x00U,    ///< No operation.
        Unknown = 0xffU, ///< Unknown operation.

        // Masks:
        //   program:
        FlowMask = 0x80U,  ///< Mask for flow operations.
        Size2Mask = 0x40U, ///< Mask for operations that require two code units.
        //   behaviour:
        NotMask = 0x20U, ///< Mask for negated operations.
        AltMask = 0x10U, ///< Mask for alternative function (assert, skip-if, increment)

        // Flow Operations:
        Split = 0x01U | FlowMask,                     ///< Split into two threads.
        Jump = 0x02U | FlowMask | Size2Mask,          ///< Unconditional jump to another location.
        Match = 0x03U | FlowMask,                     ///< End the *thread* with a successful match.
        NotMatch = Match | NotMask,                   ///< End the *thread* with no match.
        Anchor = 0x04U | FlowMask,                    ///< Match an anchor
        AssertCategory = 0x05U | FlowMask,            ///< Zero-width match of a category.
        NotAssertCategory = AssertCategory | NotMask, ///< Zero-width *Not* match a category.
        StartCapture = 0x06U | FlowMask,              ///< Start capturing.
        StopCapture = StartCapture | NotMask,         ///< Stop capturing.
        Counter = 0x07U | FlowMask,                   ///< Sets a counter to a given value.
        AddCounter = Counter | AltMask,               ///< Saturation add to a counter.
        Maximum = 0x08U | FlowMask,                   ///< Conditional end thread and increase counter.
        SkipIfMaximum = Maximum | AltMask,            ///< Conditional skip (pc + 1) if the counter reached maximum.
        Minimum = 0x09U | FlowMask,                   ///< Conditional end thread depending on counter.
        Success = 0x0aU | FlowMask,                   ///< End of the *program* with a successful match.
        Failure = Success | NotMask,                  ///< End of the *program* with no match.
        StartAtomic = 0x0bU | FlowMask,               ///< Start an atomic group.
        StopAtomic = StartAtomic | NotMask,           ///< End an atomic group.

        // Char Operations:
        //   masks:
        CaseInsensitiveMask = 0x08U, ///< Mask for case-insensitive char operations.
        //   operations:
        Char = 0x01U,                                     ///< Match a single character.
        CiChar = Char | CaseInsensitiveMask,              ///< Match a single case-insensitive character.
        NotChar = Char | NotMask,                         ///< Negated match of a single character.
        NotCiChar = Char | NotMask | CaseInsensitiveMask, ///< Negated match of a single case-insensitive character.
        Sequence = 0x02U,        ///< Match a character sequence (always uses the last counter).
        CiSequence = Sequence |
            CaseInsensitiveMask, ///< Match a case-insensitive character sequence (always uses the last counter).
        Category = 0x03U,        ///< Match a category.
        NotCategory = Category | NotMask,      ///< *Not* match a category.
        Class = 0x04U,                         ///< Match a character to a custom character class.
        CiClass = Class | CaseInsensitiveMask, ///< Match a character to a custom character class case-insensitive.
        NotClass = Class | NotMask,            ///< *Not* match a character to a custom character class.
        NotCiClass = Class | NotMask |
            CaseInsensitiveMask, ///< *Not* match a character to a custom character class case-insensitive.
        Any = 0x05U,             ///< Match any character.
    };

public:
    /// Create an empty operation.
    Operation() = default;
    /// Create an operation with `value`.
    constexpr Operation(const Value value) : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~Operation() = default;
    Operation(const Operation &) = default;
    auto operator=(const Operation &) -> Operation & = default;

public: // operators
    /// Compare operations for equality.
    constexpr auto operator==(const Operation &other) const noexcept -> bool { return _value == other._value; }
    /// Compare operations for inequality.
    constexpr auto operator!=(const Operation &other) const noexcept -> bool { return _value != other._value; }

public: // tests
    /// Test if this is a flow operation.
    /// Flow operations do not consume characters.
    [[nodiscard]] auto isFlow() const noexcept -> bool {
        return (static_cast<ValueNative>(_value) & static_cast<ValueNative>(FlowMask)) != 0;
    }
    /// Test if this operation requires two code units.
    [[nodiscard]] auto isSize2() const noexcept -> bool {
        return (static_cast<ValueNative>(_value) & static_cast<ValueNative>(Size2Mask)) != 0;
    }
    /// Test if this is a case-insensitive operation.
    [[nodiscard]] auto isCaseInsensitive() const noexcept -> bool {
        return (static_cast<ValueNative>(_value) & static_cast<ValueNative>(CaseInsensitiveMask)) != 0;
    }
    /// Access the raw enum value.
    [[nodiscard]] auto raw() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert the operation from program code.
    [[nodiscard]] constexpr static auto fromCode(const Program::Code code) noexcept -> Operation {
        return Operation{static_cast<Value>(code >> 24)};
    }
    /// Convert the operation to program code.
    [[nodiscard]] auto toCode() const noexcept -> Program::Code { return static_cast<Program::Code>(_value << 24); }

private:
    Value _value{None};
};

}
