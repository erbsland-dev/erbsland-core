// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>

namespace erbsland::conf::impl::lexer {

/// Stores the result of parsing decimal digits.
/// @notest{Used only by the ValueFloat implementation.}
struct ParseDecimalDigitsResult final {
    std::size_t digitCount; ///< The number of digits.
    bool zeroPrefixed;      ///< If the number has more than one digit and a zero prefix.
};

}
