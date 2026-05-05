// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../math/IntegerConversion.hpp"
#include "../../math/IntegerTraits.hpp"

#include <concepts>
#include <type_traits>

namespace erbsland::text::impl {

template <typename T>
concept FormatCharacterArgument =
    std::same_as<T, char> || std::same_as<T, char8_t> || std::same_as<T, char16_t> || std::same_as<T, char32_t>;

template <typename T>
concept FormatIntegerArgument = math::AnyIntegerType<T> && !FormatCharacterArgument<T>;

template <typename T>
concept FormatSignedIntegerArgument = FormatIntegerArgument<T> && std::signed_integral<math::NativeIntegerOfT<T>>;

template <typename T>
concept FormatUnsignedIntegerArgument = FormatIntegerArgument<T> && std::unsigned_integral<math::NativeIntegerOfT<T>>;

}
