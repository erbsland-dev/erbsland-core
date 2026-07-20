// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/Character.hpp"

#include "../../../text/String.hpp"
#include "../../../text/StringEditor.hpp"

#include <cstdint>
#include <variant>
#include <vector>

namespace erbsland::re::impl {

/// The type of argument.
enum class ArgumentKind : uint8_t {
    Unknown,        ///< An unknown argument kind, used when parsing code.
    ProgramCounter, ///< A program counter as an integer or label.
    Char,           ///< A character as UTF-8 code point.
    CaptureGroup,   ///< The index of a capture group (zero-based!).
    Anchor,         ///< An anchor type. Specified as text, returned as integer.
    Category,       ///< A character category. Specified as text, returned as integer.
    SequenceIndex,  ///< The start index of a character sequence.
    SequenceLength, ///< The length of a character sequence.
    CharClassIndex, ///< The index of a predefined character class
    CounterIndex,   ///< A counter index.
    CounterValue,   ///< A counter value.
    AtomicGroupId,  ///< An atomic group ID.
};

/// The native type of the argument.
enum class ArgumentType : uint8_t {
    Text,    ///< A text.
    Integer, ///< An integer.
    Boolean, ///< A boolean.
};

/// A single argument.
using ArgumentValue = std::variant<text::StringEditor, uint32_t, bool>;

/// An index of an argument in an operation.
using ArgumentIndex = uint8_t;

/// A list of arguments.
using Arguments = std::vector<ArgumentValue>;

/// Get a string for the argument kind.
[[nodiscard]] auto toString(ArgumentKind argumentKind) noexcept -> text::String;

/// Get a string for the argument type.
[[nodiscard]] auto toString(ArgumentType argumentType) noexcept -> text::String;

/// Get the type from a value.
[[nodiscard]] auto argumentTypeFromValue(const ArgumentValue &value) noexcept -> ArgumentType;

}
