// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Divider.hpp"
#include "OneChar.hpp"
#include "PatternSet.hpp"
#include "Range.hpp"
#include "Text.hpp"

#include <type_traits>

namespace erbsland::text::pattern {

template <typename T>
concept AnyElement = std::is_same_v<std::remove_cvref_t<T>, Text> || std::is_same_v<std::remove_cvref_t<T>, OneChar> ||
    std::is_same_v<std::remove_cvref_t<T>, Range> || std::is_same_v<std::remove_cvref_t<T>, Set> ||
    std::is_same_v<std::remove_cvref_t<T>, Divider>;

}
