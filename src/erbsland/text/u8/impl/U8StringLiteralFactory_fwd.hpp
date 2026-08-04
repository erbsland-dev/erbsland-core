// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../U8StringLiteral_fwd.hpp"

#include <cstddef>

namespace erbsland::text::impl {

constexpr auto createU8StringLiteral(const char *data, std::size_t size) noexcept -> U8StringLiteral<char>;
constexpr auto createU8StringLiteral(const char8_t *data, std::size_t size) noexcept -> U8StringLiteral<char8_t>;

}
