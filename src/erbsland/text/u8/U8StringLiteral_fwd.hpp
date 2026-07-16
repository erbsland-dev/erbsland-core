// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String_fwd.hpp"
#include "U8StringView_fwd.hpp"

#include "impl/U8StringLiteralFactory_fwd.hpp"

#include <cstddef>

namespace erbsland::text {

template <typename tChar>
class U8StringLiteral;

namespace literals {

constexpr auto operator""_el(const char *data, std::size_t size) noexcept -> U8StringLiteral<char>;
constexpr auto operator""_el(const char8_t *data, std::size_t size) noexcept -> U8StringLiteral<char8_t>;
auto operator""_elv(const char *data, std::size_t size) noexcept -> U8StringView;
auto operator""_elv(const char8_t *data, std::size_t size) noexcept -> U8StringView;
auto operator""_els(const char *data, std::size_t size) -> U8String;
auto operator""_els(const char8_t *data, std::size_t size) -> U8String;

}

}
