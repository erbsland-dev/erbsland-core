// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>

namespace erbsland::text {

template <typename tChar>
class U8StringLiteral;

}

namespace erbsland::text::literals {

constexpr auto operator""_el(const char *data, std::size_t size) noexcept -> U8StringLiteral<char>;
constexpr auto operator""_el(const char8_t *data, std::size_t size) noexcept -> U8StringLiteral<char8_t>;
}

#include "impl/U8StringLiteralFactory_fwd.hpp"
