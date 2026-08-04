// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../U32StringLiteral_fwd.hpp"

#include <cstddef>

namespace erbsland::text::impl {

constexpr auto createU32StringLiteral(const char32_t *data, std::size_t size) noexcept -> U32StringLiteral;

}
