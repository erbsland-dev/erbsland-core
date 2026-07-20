// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/String.hpp"
#include "../../../time/Date.hpp"
#include "../../../time/DateTime.hpp"
#include "../../Float.hpp"
#include "../../Integer.hpp"

#include <type_traits>

namespace erbsland::conf::vr::builder {

template <typename ValueType>
auto constexpr IsInteger = std::is_integral_v<ValueType> && !std::is_same_v<ValueType, bool>;

template <typename ValueType>
auto constexpr IsFloat = std::is_floating_point_v<ValueType>;

template <typename ValueType>
auto constexpr IsBoolean = std::is_same_v<ValueType, bool>;

template <typename ValueType>
auto constexpr IsDate = std::is_same_v<ValueType, time::Date>;

template <typename ValueType>
auto constexpr IsDateTime = std::is_same_v<ValueType, time::DateTime>;

template <typename ValueType>
auto constexpr IsString = std::is_same_v<ValueType, text::String>;

template <typename ValueType>
auto constexpr IsStringLike = IsString<ValueType>;

template <typename ValueType>
auto constexpr IsIntegerPair = std::is_same_v<ValueType, std::pair<Integer, Integer>>;

}
