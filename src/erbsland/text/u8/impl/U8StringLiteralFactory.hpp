// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringLiteralFactory_fwd.hpp"

#include "../U8StringLiteral.hpp"

namespace erbsland::text::impl {

constexpr auto createU8StringLiteral(const char *data, const std::size_t size) noexcept -> U8StringLiteral<char> {
    return {data, size};
}

constexpr auto createU8StringLiteral(const char8_t *data, const std::size_t size) noexcept -> U8StringLiteral<char8_t> {
    return {data, size};
}

}
