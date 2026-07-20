// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <type_traits>

namespace erbsland::conf::impl {

template <class>
inline constexpr bool always_false_v = false;

template <class T>
inline constexpr bool value_get_pass_default_by_ref_v =
    std::is_same_v<T, text::String> || std::is_same_v<T, mem::ByteBlock> || std::is_same_v<T, re::RegExPtr> ||
    std::is_same_v<T, time::TimeWithZone> || std::is_same_v<T, time::Date> || std::is_same_v<T, time::Time> ||
    std::is_same_v<T, time::DateTime> || std::is_same_v<T, time::CalendarDelta>;

template <class T>
using value_get_default_param_t = std::conditional_t<value_get_pass_default_by_ref_v<T>, const T &, T>;

}
