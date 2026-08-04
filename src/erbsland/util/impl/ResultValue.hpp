// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::util::impl {

/// Stores the raw byte representation of a Result state.
/// @notest{Covered by Result value construction tests.}
class ResultValue final {
    /// Create a result value from its raw byte representation.
    constexpr explicit ResultValue(const uint8_t value) : value{value} {}

public:
    /// Create a success value.
    template <uint8_t N>
    [[nodiscard]] constexpr static auto success() noexcept -> ResultValue {
        static_assert(N < uint8_t{0x80U});
        return ResultValue{N};
    }
    /// Create a failure value.
    template <uint8_t N>
    [[nodiscard]] constexpr static auto failure() noexcept -> ResultValue {
        static_assert(N < uint8_t{0x80U});
        return ResultValue{static_cast<std::uint8_t>(0xFFU - N)};
    }

public:
    /// Raw byte representation of the result state.
    uint8_t value{};
};

}
