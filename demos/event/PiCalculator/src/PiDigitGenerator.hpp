// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/BigInteger.hpp>
#include <erbsland/Char.hpp>

#include <optional>

namespace demo {

/// Produce exact decimal digits through independent steps of an unbounded spigot algorithm.
/// Arithmetic belongs to the calculation thread; no terminal or event dependencies are needed.
/// @tested{PiDigitGeneratorTest}
class PiDigitGenerator final {
public: // calculation
    /// Perform one transformation, optionally emitting the next digit (starting with the integer part, 3).
    /// This exposes cancellation points even while several transformations are needed for one digit.
    /// @return The next determined digit, or no digit when more input was consumed.
    /// @throws Any exception from arbitrary-precision arithmetic or allocation.
    [[nodiscard]] auto advance() -> std::optional<el::Char>;

private:
    /// Evaluate the current transformation at an interval endpoint using mathematical floor.
    [[nodiscard]] auto extract(int endpoint) const -> el::BigInteger;
    /// Consume one term of the infinite expansion without committing an uncertain digit.
    void consumeTerm();
    /// Remove the emitted digit and shift the remaining fractional value one decimal place.
    void emitDigit(const el::BigInteger &digit);

private:
    el::BigInteger _q{1}; ///< Numerator scale of the transformation (q*x+r)/t.
    el::BigInteger _r{0}; ///< Signed numerator offset.
    el::BigInteger _t{1}; ///< Strictly positive denominator.
    el::BigInteger _k{1}; ///< Next input term, also unbounded.
};

}
