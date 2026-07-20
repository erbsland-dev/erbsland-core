// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::conf {

using namespace text::literals;

template <typename T>
auto Value::asType() const noexcept -> T {
    static_assert(
        ValueType::from<T>().raw() != ValueType::Undefined,
        "no type support available for the specified template argument.");
    return {};
}

template <typename T>
auto Value::asTypeOrThrow() const -> T {
    static_assert(
        ValueType::from<T>().raw() != ValueType::Undefined,
        "no type support available for the specified template argument.");
    throw ConfError(ConfErrorCategory::TypeMismatch, "asTypeOrThrow() does not support the given type."_el);
}

template <typename T>
    requires std::is_integral_v<T>
auto Value::asType() const noexcept -> T {
    return math::saturatingCast<T, Integer>(asInteger());
}

template <typename T>
    requires std::is_integral_v<T>
auto Value::asTypeOrThrow() const -> T {
    const auto value = asIntegerOrThrow();
    if (math::willCastOverflow<T, Integer>(value)) {
        throw ConfError(
            ConfErrorCategory::TypeMismatch, "The value exceeds the expected range."_el, namePath(), location());
    }
    return static_cast<T>(value);
}

template <typename T>
    requires std::is_floating_point_v<T>
auto Value::asType() const noexcept -> T {
    const auto value = asFloat();
    if constexpr (sizeof(T) < sizeof(Float)) {
        if (!std::isfinite(value)) {
            return static_cast<T>(value);
        }
        const auto max = static_cast<Float>(std::numeric_limits<T>::max());
        const auto lowest = static_cast<Float>(std::numeric_limits<T>::lowest());
        if (value > max) {
            return std::numeric_limits<T>::max();
        }
        if (value < lowest) {
            return std::numeric_limits<T>::lowest();
        }
    }
    return static_cast<T>(value);
}

template <typename T>
    requires std::is_floating_point_v<T>
auto Value::asTypeOrThrow() const -> T {
    const auto value = asFloatOrThrow();
    if constexpr (sizeof(T) < sizeof(Float)) {
        if (!std::isfinite(value)) {
            return static_cast<T>(value);
        }
        const auto max = static_cast<Float>(std::numeric_limits<T>::max());
        const auto lowest = static_cast<Float>(std::numeric_limits<T>::lowest());
        if (value > max || value < lowest) {
            throw ConfError(
                ConfErrorCategory::TypeMismatch, "The value exceeds the expected range."_el, namePath(), location());
        }
    }
    return static_cast<T>(value);
}

template <>
[[nodiscard]] inline auto Value::asType<bool>() const noexcept -> bool {
    return asBoolean();
}

template <>
[[nodiscard]] inline auto Value::asType<text::String>() const noexcept -> text::String {
    return asText();
}

template <>
[[nodiscard]] inline auto Value::asType<time::Date>() const noexcept -> time::Date {
    return asDate();
}
template <>
[[nodiscard]] inline auto Value::asType<time::Time>() const noexcept -> time::Time {
    return asTime();
}
template <>
[[nodiscard]] inline auto Value::asType<time::TimeWithZone>() const noexcept -> time::TimeWithZone {
    return asTimeWithZone();
}
template <>
[[nodiscard]] inline auto Value::asType<time::DateTime>() const noexcept -> time::DateTime {
    return asDateTime();
}
template <>
[[nodiscard]] inline auto Value::asType<time::CalendarDelta>() const noexcept -> time::CalendarDelta {
    return asCalendarDelta();
}
template <>
[[nodiscard]] inline auto Value::asType<mem::ByteBlock>() const noexcept -> mem::ByteBlock {
    return asBytes();
}
template <>
[[nodiscard]] inline auto Value::asType<re::RegExPtr>() const noexcept -> re::RegExPtr {
    return asRegEx();
}
template <>
[[nodiscard]] inline auto Value::asType<ValueList>() const noexcept -> ValueList {
    return asValueList();
}

template <>
[[nodiscard]] inline auto Value::asTypeOrThrow<bool>() const -> bool {
    return asBooleanOrThrow();
}
template <>
[[nodiscard]] inline auto Value::asTypeOrThrow<text::String>() const -> text::String {
    return asTextOrThrow();
}
template <>
[[nodiscard]] inline auto Value::asTypeOrThrow<time::Date>() const -> time::Date {
    return asDateOrThrow();
}
template <>
[[nodiscard]] inline auto Value::asTypeOrThrow<time::Time>() const -> time::Time {
    return asTimeOrThrow();
}
template <>
[[nodiscard]] inline auto Value::asTypeOrThrow<time::TimeWithZone>() const -> time::TimeWithZone {
    return asTimeWithZoneOrThrow();
}
template <>
[[nodiscard]] inline auto Value::asTypeOrThrow<time::DateTime>() const -> time::DateTime {
    return asDateTimeOrThrow();
}
template <>
[[nodiscard]] inline auto Value::asTypeOrThrow<time::CalendarDelta>() const -> time::CalendarDelta {
    return asCalendarDeltaOrThrow();
}
template <>
[[nodiscard]] inline auto Value::asTypeOrThrow<mem::ByteBlock>() const -> mem::ByteBlock {
    return asBytesOrThrow();
}
template <>
[[nodiscard]] inline auto Value::asTypeOrThrow<re::RegExPtr>() const -> re::RegExPtr {
    return asRegExOrThrow();
}
template <>
[[nodiscard]] inline auto Value::asTypeOrThrow<ValueList>() const -> ValueList {
    return asValueListOrThrow();
}

}
