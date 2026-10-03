// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PiDigitGenerator.hpp"

#include <erbsland/BigInteger.hpp>
#include <erbsland/Char.hpp>

#include <utility>

namespace demo {

auto PiDigitGenerator::advance() -> std::optional<el::Char> {
    // Section 5 of Gibbons, "Unbounded Spigot Algorithms for the Digits of Pi", corrected September 2024:
    // https://www.cs.ox.ac.uk/people/jeremy.gibbons/publications/spigot.pdf
    // The unconsumed tail lies in [3,4]. Equal endpoint floors determine the digit for the entire interval.
    const auto digit = extract(3);
    if (digit == extract(4)) {
        emitDigit(digit);
        return el::Char{static_cast<char32_t>(U'0' + digit.castOrThrow<unsigned int>())};
    }
    consumeTerm();
    return std::nullopt;
}

auto PiDigitGenerator::extract(const int endpoint) const -> el::BigInteger {
    const auto numerator = _q * el::BigInteger{endpoint} + _r;
    auto result = numerator / _t;
    // BigInteger division truncates toward zero; the algorithm requires floor for a negative non-integer ratio.
    if (numerator.isNegative() && !(numerator % _t).isZero()) {
        result -= el::BigInteger{1};
    }
    return result;
}

void PiDigitGenerator::consumeTerm() {
    // Compose the current transformation with (k*x + 4*k + 2)/(2*k + 1).
    // Compute from the old coefficients before replacing them; updating q early would change the expansion.
    const auto denominator = el::BigInteger{2} * _k + el::BigInteger{1};
    auto nextQ = _q * _k;
    auto nextR = _q * (el::BigInteger{4} * _k + el::BigInteger{2}) + _r * denominator;
    _t *= denominator;
    _q = std::move(nextQ);
    _r = std::move(nextR);
    _k += el::BigInteger{1};
}

void PiDigitGenerator::emitDigit(const el::BigInteger &digit) {
    _r = el::BigInteger{10} * (_r - digit * _t);
    _q *= el::BigInteger{10};
}

}
