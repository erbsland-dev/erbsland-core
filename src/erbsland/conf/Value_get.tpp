// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::conf {

template <typename tExpectedType>
    requires(
        std::is_same_v<tExpectedType, bool> ||
        (!std::is_integral_v<tExpectedType> && !std::is_floating_point_v<tExpectedType>))
auto Value::get(
    [[maybe_unused]] const NamePathLike &namePath,
    impl::value_get_default_param_t<tExpectedType> defaultValue) const noexcept -> tExpectedType {

    return defaultValue;
}

template <typename tExpectedType>
    requires(std::is_integral_v<tExpectedType> && !std::is_same_v<tExpectedType, bool>)
auto Value::get(const NamePathLike &namePath, tExpectedType defaultValue) const noexcept -> tExpectedType {
    const auto resolvedValue = value(namePath);
    if (resolvedValue == nullptr || resolvedValue->type() != ValueType::Integer) {
        return defaultValue;
    }
    return resolvedValue->template asType<tExpectedType>();
}

template <typename tExpectedType>
    requires(
        std::is_floating_point_v<tExpectedType> && !std::is_integral_v<tExpectedType> &&
        !std::is_same_v<tExpectedType, bool>)
auto Value::get(const NamePathLike &namePath, tExpectedType defaultValue) const noexcept -> tExpectedType {
    return static_cast<tExpectedType>(getFloat(namePath, static_cast<Float>(defaultValue)));
}

template <>
[[nodiscard]] inline auto Value::get<bool>(const NamePathLike &namePath, bool defaultValue) const noexcept -> bool {
    return getBoolean(namePath, defaultValue);
}
template <>
[[nodiscard]] inline auto Value::get<text::String>(
    const NamePathLike &namePath, const text::String &defaultValue) const noexcept -> text::String {
    return getText(namePath, defaultValue);
}
template <>
[[nodiscard]] inline auto Value::get<time::Date>(
    const NamePathLike &namePath, const time::Date &defaultValue) const noexcept -> time::Date {
    return getDate(namePath, defaultValue);
}
template <>
[[nodiscard]] inline auto Value::get<time::Time>(
    const NamePathLike &namePath, const time::Time &defaultValue) const noexcept -> time::Time {
    return getTime(namePath, defaultValue);
}
template <>
[[nodiscard]] inline auto Value::get<time::TimeWithZone>(
    const NamePathLike &namePath, const time::TimeWithZone &defaultValue) const noexcept -> time::TimeWithZone {
    return getTimeWithZone(namePath, defaultValue);
}
template <>
[[nodiscard]] inline auto Value::get<time::DateTime>(
    const NamePathLike &namePath, const time::DateTime &defaultValue) const noexcept -> time::DateTime {
    return getDateTime(namePath, defaultValue);
}
template <>
[[nodiscard]] inline auto Value::get<time::CalendarDelta>(
    const NamePathLike &namePath, const time::CalendarDelta &defaultValue) const noexcept -> time::CalendarDelta {
    return getCalendarDelta(namePath, defaultValue);
}
template <>
[[nodiscard]] inline auto Value::get<mem::ByteBlock>(
    const NamePathLike &namePath, const mem::ByteBlock &defaultValue) const noexcept -> mem::ByteBlock {
    return getBytes(namePath, defaultValue);
}
template <>
[[nodiscard]] inline auto Value::get<re::RegExPtr>(
    const NamePathLike &namePath, const re::RegExPtr &defaultValue) const noexcept -> re::RegExPtr {
    return getRegEx(namePath, defaultValue);
}

template <>
[[nodiscard]] inline auto Value::getOrThrow<bool>(const NamePathLike &namePath) const -> bool {
    return getBooleanOrThrow(namePath);
}
template <>
[[nodiscard]] inline auto Value::getOrThrow<text::String>(const NamePathLike &namePath) const -> text::String {
    return getTextOrThrow(namePath);
}
template <>
[[nodiscard]] inline auto Value::getOrThrow<time::Date>(const NamePathLike &namePath) const -> time::Date {
    return getDateOrThrow(namePath);
}
template <>
[[nodiscard]] inline auto Value::getOrThrow<time::Time>(const NamePathLike &namePath) const -> time::Time {
    return getTimeOrThrow(namePath);
}
template <>
[[nodiscard]] inline auto Value::getOrThrow<time::TimeWithZone>(const NamePathLike &namePath) const
    -> time::TimeWithZone {
    return getTimeWithZoneOrThrow(namePath);
}
template <>
[[nodiscard]] inline auto Value::getOrThrow<time::DateTime>(const NamePathLike &namePath) const -> time::DateTime {
    return getDateTimeOrThrow(namePath);
}
template <>
[[nodiscard]] inline auto Value::getOrThrow<time::CalendarDelta>(const NamePathLike &namePath) const
    -> time::CalendarDelta {
    return getCalendarDeltaOrThrow(namePath);
}
template <>
[[nodiscard]] inline auto Value::getOrThrow<mem::ByteBlock>(const NamePathLike &namePath) const -> mem::ByteBlock {
    return getBytesOrThrow(namePath);
}
template <>
[[nodiscard]] inline auto Value::getOrThrow<re::RegExPtr>(const NamePathLike &namePath) const -> re::RegExPtr {
    return getRegExOrThrow(namePath);
}

}
