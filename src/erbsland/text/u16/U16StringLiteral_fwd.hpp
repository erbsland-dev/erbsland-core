// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String_fwd.hpp"
#include "U16StringView_fwd.hpp"

#include "impl/U16StringLiteralFactory_fwd.hpp"

#include "../../mem/UnsafeCharPtr.hpp"

#include <cstddef>

namespace erbsland::text {

class U16StringLiteral;

namespace literals {

constexpr auto operator""_el(const char16_t *data, std::size_t size) noexcept -> U16StringLiteral;
auto operator""_elv(const char16_t *data, std::size_t size) noexcept -> U16StringView;
auto operator""_els(const char16_t *data, std::size_t size) -> U16String;

}

}
