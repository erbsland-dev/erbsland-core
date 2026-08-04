// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BooleanValue.hpp"
#include "BytesValue.hpp"
#include "CalendarDeltaValue.hpp"
#include "DateTimeValue.hpp"
#include "DateValue.hpp"
#include "FloatValue.hpp"
#include "IntegerValue.hpp"
#include "RegExValue.hpp"
#include "TextValue.hpp"
#include "TimeValue.hpp"
#include "TimeWithZoneValue.hpp"

#include "../utilities/TypeTraits.hpp"

namespace erbsland::conf::impl {

/// Access the matching scalar value's storage without copying it.
/// @param value The scalar configuration value.
/// @return A reference to the matching stored value.
template <typename T>
[[nodiscard]] auto directStorageAccess(const conf::ValuePtr &value) noexcept -> const T & {
    if constexpr (std::is_same_v<T, Integer>) {
        return std::dynamic_pointer_cast<IntegerValue>(value)->rawStorage();
    } else if constexpr (std::is_same_v<T, bool>) {
        return std::dynamic_pointer_cast<BooleanValue>(value)->rawStorage();
    } else if constexpr (std::is_same_v<T, Float>) {
        return std::dynamic_pointer_cast<FloatValue>(value)->rawStorage();
    } else if constexpr (std::is_same_v<T, text::String>) {
        return std::dynamic_pointer_cast<TextValue>(value)->rawStorage();
    } else if constexpr (std::is_same_v<T, time::Date>) {
        return std::dynamic_pointer_cast<DateValue>(value)->rawStorage();
    } else if constexpr (std::is_same_v<T, time::Time>) {
        return std::dynamic_pointer_cast<TimeValue>(value)->rawStorage();
    } else if constexpr (std::is_same_v<T, time::TimeWithZone>) {
        return std::dynamic_pointer_cast<TimeWithZoneValue>(value)->rawStorage();
    } else if constexpr (std::is_same_v<T, time::DateTime>) {
        return std::dynamic_pointer_cast<DateTimeValue>(value)->rawStorage();
    } else if constexpr (std::is_same_v<T, mem::ByteBlock>) {
        return std::dynamic_pointer_cast<BytesValue>(value)->rawStorage();
    } else if constexpr (std::is_same_v<T, time::CalendarDelta>) {
        return std::dynamic_pointer_cast<CalendarDeltaValue>(value)->rawStorage();
    } else if constexpr (std::is_same_v<T, re::RegExPtr>) {
        return std::dynamic_pointer_cast<RegExValue>(value)->rawStorage();
    } else {
        static_assert(always_false_v<T>, "Unsupported type");
        std::terminate();
    }
}

}
