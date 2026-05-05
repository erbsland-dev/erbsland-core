// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>

namespace erbsland::text {

class U16StringLiteral;

namespace impl {

constexpr auto createU16StringLiteral(const char16_t *data, std::size_t size) noexcept -> U16StringLiteral;

}

}
