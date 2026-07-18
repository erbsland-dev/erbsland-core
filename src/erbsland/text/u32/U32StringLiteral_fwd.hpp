// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/U32StringLiteralFactory_fwd.hpp"

#include <cstddef>

namespace erbsland::text {

class U32StringLiteral;

namespace literals {

constexpr auto operator""_el(const char32_t *data, std::size_t size) noexcept -> U32StringLiteral;
}

}
