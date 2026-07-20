// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatAs_fwd.hpp"

#include <cstdint>

namespace erbsland::text {

/// Base type for adapting a user value to a supported format argument type.
/// @seedoc{/reference/text/string_formatter}
/// @tested{FormatAsTest}
template <typename tValue, typename tArgument>
struct FormatAs {
    /// The user value type to adapt.
    using Value = tValue;
    /// The supported format argument type.
    using Argument = tArgument;
};

/// Adapt a user value to a signed integer format argument.
/// @tested{FormatAsTest}
template <typename T>
struct FormatAsInt64;

/// Adapt a user value to an unsigned integer format argument.
/// @tested{FormatAsTest}
template <typename T>
struct FormatAsUInt64;

/// Adapt a user value to a floating point format argument.
/// @tested{FormatAsTest}
template <typename T>
struct FormatAsDouble;

/// Adapt a user value to a boolean format argument.
/// @tested{FormatAsTest}
template <typename T>
struct FormatAsBool;

/// Adapt a user value to a character format argument.
/// @tested{FormatAsTest}
template <typename T>
struct FormatAsChar;

/// Adapt a user value to a UTF-8 text format argument.
template <typename T>
struct FormatAsText;

/// Adapt a user value to a UTF-8 text format argument.
/// @tested{FormatAsTest}
template <typename T>
struct FormatAsU8Text;

/// Adapt a user value to a UTF-16 text format argument.
/// @tested{FormatAsTest}
template <typename T>
struct FormatAsU16Text;

/// Adapt a user value to a UTF-32 text format argument.
/// @tested{FormatAsTest}
template <typename T>
struct FormatAsU32Text;

/// Adapt a value to a byte-block format argument.
/// @tested{FormatAsTest}
template <typename T>
struct FormatAsBytes;

}
