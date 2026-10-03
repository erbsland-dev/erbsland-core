// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/BigInteger.hpp>
#include <erbsland/List.hpp>
#include <erbsland/render/Value.hpp>
#include <erbsland/String.hpp>

#include <cstdint>

namespace tool::test_data::impl {

/// Independent exact arithmetic oracle and cached boundary samples for integer fixtures.
class SaturatingMathData final {
public:
    /// The reviewed fixture operations, in generation order.
    enum class Operation : uint8_t { Cast, Add, Subtract, Multiply, Divide, Modulo };

public:
    /// Prepare each type's bounds, sorted boundary samples, and decimal spellings once.
    SaturatingMathData();
    /// Prepare structured rows in destination/source/sample order.
    /// @param operation Operation to calculate with exact integer arithmetic.
    /// @return Rows containing input types, operands, the clamped expectation, and overflow.
    [[nodiscard]] auto rows(Operation operation) const -> el::render::ValueList;
    /// Get the stable operation filename stem.
    [[nodiscard]] static auto name(Operation operation) -> el::String;

private:
    /// Cached data for one signed or unsigned integer width.
    struct Samples {
        el::String name;
        el::BigInteger minimum;
        el::BigInteger maximum;
        el::List<el::BigInteger> values;
        el::StringList decimals;
        /// Clamp an exact result to this destination's bounds.
        [[nodiscard]] auto clamp(const el::BigInteger &value) const -> el::BigInteger;
    };
    /// Calculate an exact binary operation; zero divisors must have been excluded.
    [[nodiscard]] static auto calculate(Operation operation, const el::BigInteger &first, const el::BigInteger &second)
        -> el::BigInteger;

private:
    el::List<Samples> _types; ///< Reviewed type order and reusable boundary samples.
};

}
