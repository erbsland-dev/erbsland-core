// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../U16StringLiteral.hpp"

namespace erbsland::text::impl {

constexpr auto createU16StringLiteral(const char16_t *data, const std::size_t size) noexcept -> U16StringLiteral {
    return {data, size};
}

}
