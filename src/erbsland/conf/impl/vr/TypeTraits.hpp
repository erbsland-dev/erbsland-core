// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/String.hpp"
#include "../../../time/Date.hpp"
#include "../../../time/DateTime.hpp"
#include "../../Float.hpp"
#include "../../Integer.hpp"

#include <type_traits>

namespace erbsland::conf::impl {

template <typename tValueType>
auto constexpr IsInteger = std::is_integral_v<tValueType> && !std::is_same_v<tValueType, bool>;

template <typename tValueType>
auto constexpr IsFloat = std::is_floating_point_v<tValueType>;

template <typename tValueType>
auto constexpr IsBoolean = std::is_same_v<tValueType, bool>;

template <typename tValueType>
auto constexpr IsDate = std::is_same_v<tValueType, time::Date>;

template <typename tValueType>
auto constexpr IsDateTime = std::is_same_v<tValueType, time::DateTime>;

template <typename tValueType>
auto constexpr IsString = std::is_same_v<tValueType, text::String>;

template <typename tValueType>
auto constexpr IsStringLike = IsString<tValueType>;

template <typename tValueType>
auto constexpr IsIntegerPair = std::is_same_v<tValueType, std::pair<Integer, Integer>>;

}
