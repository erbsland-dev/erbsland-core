// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../math/AnyIntegerTypes.hpp"
#include "../../text/String_fwd.hpp"

#include <concepts>
#include <type_traits>

namespace erbsland::stream::impl {

template <typename T>
concept PrintStringResult = std::same_as<std::remove_cvref_t<T>, text::String>;

template <typename T>
concept PrintCharacterArgument =
    std::same_as<T, char> || std::same_as<T, char8_t> || std::same_as<T, char16_t> || std::same_as<T, char32_t>;

template <typename T>
concept PrintRawIntegerResult =
    math::AnyIntegerType<std::remove_cvref_t<T>> && !PrintCharacterArgument<std::remove_cvref_t<T>>;

template <typename T>
concept PrintObjectWithToString = !math::AnyIntegerType<std::remove_cvref_t<T>> && requires(const T &value) {
    { value.toString() } -> PrintStringResult;
};

template <typename T>
concept PrintObjectWithRawInteger = !PrintObjectWithToString<T> && requires(const T &value) {
    { value.toRawValue() } -> PrintRawIntegerResult;
};

}
