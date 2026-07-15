// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BooleanFormat.hpp"
#include "FloatFormat.hpp"
#include "IntegerFormat.hpp"
#include "String_fwd.hpp"
#include "StringView_fwd.hpp"

#include <compare>

namespace erbsland::text {

/// Convert a string to the common string type.
[[nodiscard]] auto toString(const String &value) -> String;

/// Convert a string view to the common string type.
[[nodiscard]] auto toString(const StringView &value) -> String;

/// Convert a boolean value to a string.
[[nodiscard]] auto toString(bool value, BooleanFormat format = BooleanFormat::defaultFormat()) -> String;

/// Convert a strong ordering value to a string.
[[nodiscard]] auto toString(std::strong_ordering value) -> String;

/// Convert an integer/float value to a string.
[[nodiscard]] auto toString(int8_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String;
/// @overload
[[nodiscard]] auto toString(int16_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String;
/// @overload
[[nodiscard]] auto toString(int32_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String;
/// @overload
[[nodiscard]] auto toString(int64_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String;
/// @overload
[[nodiscard]] auto toString(uint8_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String;
/// @overload
[[nodiscard]] auto toString(uint16_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String;
/// @overload
[[nodiscard]] auto toString(uint32_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String;
/// @overload
[[nodiscard]] auto toString(uint64_t value, IntegerFormat format = IntegerFormat::defaultFormat()) -> String;
/// @overload
[[nodiscard]] auto toString(float value, FloatFormat format = FloatFormat::defaultFormat()) -> String;
/// @overload
[[nodiscard]] auto toString(double value, FloatFormat format = FloatFormat::defaultFormat()) -> String;

}
