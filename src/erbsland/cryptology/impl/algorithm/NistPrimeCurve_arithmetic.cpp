// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NistPrimeCurve.hpp"

#include "../../../mem/Byte.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/SecureErase.hpp"
#include "../../../unit/ByteIndex.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace erbsland::cryptology::impl {

auto NistPrimeCurve::pointToUncompressed(const Point &point) const noexcept -> mem::ByteBlock {
    auto result = mem::ByteBlockEditor{};
    result.append(mem::Byte{0x04U});
    result.append(numberToBigEndian(point.x));
    result.append(numberToBigEndian(point.y));
    return mem::ByteBlock{result};
}

auto NistPrimeCurve::isZeroSecret(const Number &value) const noexcept -> bool {
    return zeroMask(value) != 0U;
}

void NistPrimeCurve::secureErase(Number &value) noexcept {
    mem::secureErase(std::span{value.words});
}

void NistPrimeCurve::secureErase(Point &point) noexcept {
    secureErase(point.x);
    secureErase(point.y);
    secureErase(point.z);
}

auto NistPrimeCurve::addModulo(const Number &left, const Number &right, const Number &modulus) const noexcept
    -> Number {
    // Compute left + right with a fixed limb schedule, retaining the carry for masked modular reduction.
    auto sum = Number{};
    auto carry = uint64_t{};
    for (auto index = std::size_t{}; index < byteLength() / 4U; ++index) {
        const auto wordSum = uint64_t{left.words[index]} + right.words[index] + carry;
        sum.words[index] = static_cast<uint32_t>(wordSum);
        carry = wordSum >> 32U;
    }
    return reduceOnceSecret(sum, modulus, static_cast<uint32_t>(carry));
}

auto NistPrimeCurve::subtractModulo(const Number &left, const Number &right, const Number &modulus) const noexcept
    -> Number {
    auto difference = Number{};
    auto borrow = uint64_t{};
    for (auto index = std::size_t{}; index < byteLength() / 4U; ++index) {
        const auto subtrahend = uint64_t{right.words[index]} + borrow;
        const auto minuend = uint64_t{left.words[index]};
        difference.words[index] = static_cast<uint32_t>(minuend - subtrahend);
        borrow = minuend < subtrahend ? 1U : 0U;
    }
    const auto corrected = add(difference, modulus);
    return selectNumber(difference, corrected, uint32_t{0U} - static_cast<uint32_t>(borrow));
}

auto NistPrimeCurve::multiplyModulo(const Number &left, const Number &right, const Number &modulus) const noexcept
    -> Number {
    // SP 800-186 section 3.2.1 uses arithmetic modulo p or n. Convert through a fixed 2^(32*limbs) Montgomery
    // domain. R^2 and -modulus^-1 mod 2^32 are fixed precomputations documented beside the curve parameters.
    const auto factor = montgomeryFactor(modulus);
    const auto &r2 = montgomeryR2(modulus);
    const auto leftMontgomery = montgomeryMultiply(left, r2, modulus, factor);
    const auto rightMontgomery = montgomeryMultiply(right, r2, modulus, factor);
    const auto productMontgomery = montgomeryMultiply(leftMontgomery, rightMontgomery, modulus, factor);
    return montgomeryMultiply(productMontgomery, cOne, modulus, factor);
}

auto NistPrimeCurve::montgomeryMultiply(
    const Number &left, const Number &right, const Number &modulus, const uint32_t factor) const noexcept -> Number {
    const auto wordCount = byteLength() / 4U;
    auto temporary = std::array<uint32_t, 14U>{};

    // HAC section 14.3.2, algorithm 14.36: interleave one product limb with one Montgomery reduction limb.
    for (auto rightIndex = std::size_t{}; rightIndex < wordCount; ++rightIndex) {
        auto carry = uint64_t{};
        for (auto leftIndex = std::size_t{}; leftIndex < wordCount; ++leftIndex) {
            const auto product =
                uint64_t{left.words[leftIndex]} * right.words[rightIndex] + temporary[leftIndex] + carry;
            temporary[leftIndex] = static_cast<uint32_t>(product);
            carry = product >> 32U;
        }
        const auto upper = uint64_t{temporary[wordCount]} + carry;
        temporary[wordCount] = static_cast<uint32_t>(upper);
        temporary[wordCount + 1U] = static_cast<uint32_t>(upper >> 32U);

        const auto reduction = static_cast<uint32_t>(uint64_t{temporary[0U]} * factor);
        carry = 0U;
        for (auto index = std::size_t{}; index < wordCount; ++index) {
            const auto product = uint64_t{reduction} * modulus.words[index] + temporary[index] + carry;
            if (index != 0U) {
                temporary[index - 1U] = static_cast<uint32_t>(product);
            }
            carry = product >> 32U;
        }
        const auto reducedUpper = uint64_t{temporary[wordCount]} + carry;
        temporary[wordCount - 1U] = static_cast<uint32_t>(reducedUpper);
        temporary[wordCount] = temporary[wordCount + 1U] + static_cast<uint32_t>(reducedUpper >> 32U);
        temporary[wordCount + 1U] = 0U;
    }

    auto result = Number{};
    std::copy_n(temporary.begin(), wordCount, result.words.begin());
    return reduceOnceSecret(result, modulus, temporary[wordCount]);
}

auto NistPrimeCurve::montgomeryR2(const Number &modulus) const noexcept -> const Number & {
    if (compare(modulus, fieldModulus()) == 0) {
        return _name == Name::P256 ? cP256FieldR2 : cP384FieldR2;
    }
    return _name == Name::P256 ? cP256OrderR2 : cP384OrderR2;
}

auto NistPrimeCurve::montgomeryFactor(const Number &modulus) const noexcept -> uint32_t {
    if (compare(modulus, fieldModulus()) == 0) {
        return 1U; // Both field moduli end in 0xffffffff.
    }
    return _name == Name::P256 ? 0xee00bc4fU : 0xe88fdc45U;
}

auto NistPrimeCurve::powerModulo(const Number &value, const Number &exponent, const Number &modulus) const noexcept
    -> Number {
    const auto montgomeryFactorValue = montgomeryFactor(modulus);
    const auto &r2 = montgomeryR2(modulus);
    auto result = montgomeryMultiply(cOne, r2, modulus, montgomeryFactorValue);
    auto factor = montgomeryMultiply(value, r2, modulus, montgomeryFactorValue);
    for (auto index = std::size_t{}; index < byteLength() * 8U; ++index) {
        if (bit(exponent, index)) {
            result = montgomeryMultiply(result, factor, modulus, montgomeryFactorValue);
        }
        if (index + 1U < byteLength() * 8U) {
            factor = montgomeryMultiply(factor, factor, modulus, montgomeryFactorValue);
        }
    }
    return montgomeryMultiply(result, cOne, modulus, montgomeryFactorValue);
}

auto NistPrimeCurve::subtractSmall(Number value, const uint32_t amount) const noexcept -> Number {
    auto borrow = uint64_t{amount};
    for (auto index = std::size_t{}; index < byteLength() / 4U && borrow != 0U; ++index) {
        const auto current = uint64_t{value.words[index]};
        value.words[index] = static_cast<uint32_t>(current - borrow);
        borrow = current < borrow ? 1U : 0U;
    }
    return value;
}

auto NistPrimeCurve::addSmall(Number value, const uint32_t amount) const noexcept -> Number {
    auto carry = uint64_t{amount};
    for (auto index = std::size_t{}; index < byteLength() / 4U && carry != 0U; ++index) {
        const auto sum = uint64_t{value.words[index]} + carry;
        value.words[index] = static_cast<uint32_t>(sum);
        carry = sum >> 32U;
    }
    return value;
}

auto NistPrimeCurve::shiftRight(Number value) const noexcept -> Number {
    auto carry = uint32_t{};
    for (auto index = byteLength() / 4U; index-- > 0U;) {
        const auto nextCarry = static_cast<uint32_t>(value.words[index] << 31U);
        value.words[index] = (value.words[index] >> 1U) | carry;
        carry = nextCarry;
    }
    return value;
}

auto NistPrimeCurve::subtract(const Number &left, const Number &right) const noexcept -> Number {
    auto result = Number{};
    auto borrow = uint64_t{};
    for (auto index = std::size_t{}; index < byteLength() / 4U; ++index) {
        const auto subtrahend = uint64_t{right.words[index]} + borrow;
        const auto minuend = uint64_t{left.words[index]};
        result.words[index] = static_cast<uint32_t>(minuend - subtrahend);
        borrow = minuend < subtrahend ? 1U : 0U;
    }
    return result;
}

auto NistPrimeCurve::add(const Number &left, const Number &right) const noexcept -> Number {
    auto result = Number{};
    auto carry = uint64_t{};
    for (auto index = std::size_t{}; index < byteLength() / 4U; ++index) {
        const auto sum = uint64_t{left.words[index]} + right.words[index] + carry;
        result.words[index] = static_cast<uint32_t>(sum);
        carry = sum >> 32U;
    }
    return result;
}

auto NistPrimeCurve::selectNumber(const Number &first, const Number &second, const uint32_t secondMask) const noexcept
    -> Number {
    auto result = Number{};
    for (auto index = std::size_t{}; index < byteLength() / 4U; ++index) {
        result.words[index] = (first.words[index] & ~secondMask) | (second.words[index] & secondMask);
    }
    return result;
}

auto NistPrimeCurve::equalMask(const Number &left, const Number &right) const noexcept -> uint32_t {
    auto difference = uint32_t{};
    for (auto index = std::size_t{}; index < byteLength() / 4U; ++index) {
        difference |= left.words[index] ^ right.words[index];
    }
    const auto nonzero = (difference | (uint32_t{0U} - difference)) >> 31U;
    return uint32_t{0U} - (nonzero ^ 1U);
}

auto NistPrimeCurve::zeroMask(const Number &value) const noexcept -> uint32_t {
    return equalMask(value, Number{});
}

auto NistPrimeCurve::reduceOnceSecret(const Number &value, const Number &modulus, const uint32_t carry) const noexcept
    -> Number {
    const auto difference = subtract(value, modulus);
    auto borrow = uint64_t{};
    for (auto index = std::size_t{}; index < byteLength() / 4U; ++index) {
        const auto subtrahend = uint64_t{modulus.words[index]} + borrow;
        borrow = uint64_t{value.words[index]} < subtrahend ? 1U : 0U;
    }
    const auto useDifference = (carry | static_cast<uint32_t>(borrow ^ 1U)) & 1U;
    return selectNumber(value, difference, uint32_t{0U} - useDifference);
}

auto NistPrimeCurve::fieldAdd(const Number &left, const Number &right) const noexcept -> Number {
    return addModulo(left, right, fieldModulus());
}

auto NistPrimeCurve::fieldSubtract(const Number &left, const Number &right) const noexcept -> Number {
    return subtractModulo(left, right, fieldModulus());
}

auto NistPrimeCurve::fieldMultiply(const Number &left, const Number &right) const noexcept -> Number {
    return multiplyModulo(left, right, fieldModulus());
}

auto NistPrimeCurve::fieldSquare(const Number &value) const noexcept -> Number {
    return fieldMultiply(value, value);
}

auto NistPrimeCurve::fieldMultiplySmall(const Number &value, const uint32_t factor) const noexcept -> Number {
    auto result = Number{};
    for (auto count = uint32_t{}; count < factor; ++count) {
        result = fieldAdd(result, value);
    }
    return result;
}

auto NistPrimeCurve::fieldInvert(const Number &value) const noexcept -> Number {
    // SP 800-186 sections 3.2.1.3 and 3.2.1.4 define prime fields; Fermat inversion uses z^(p-2).
    return powerModulo(value, subtractSmall(fieldModulus(), 2U), fieldModulus());
}

auto NistPrimeCurve::fieldSquareRoot(const Number &value) const noexcept -> std::optional<Number> {
    // Both SP 800-186 primes satisfy p mod 4 = 3, so value^((p+1)/4) is a square root when one exists.
    auto exponent = shiftRight(shiftRight(addSmall(fieldModulus(), 1U)));
    const auto candidate = powerModulo(value, exponent, fieldModulus());
    return compare(fieldSquare(candidate), value) == 0 ? std::optional<Number>{candidate} : std::nullopt;
}

auto NistPrimeCurve::identity() noexcept -> Point {
    return {};
}

auto NistPrimeCurve::affine(Number x, Number y) noexcept -> Point {
    return Point{.x = std::move(x), .y = std::move(y), .z = cOne};
}

auto NistPrimeCurve::isOnCurve(const Point &point) const noexcept -> bool {
    if (isIdentity(point) || compare(point.x, fieldModulus()) >= 0 || compare(point.y, fieldModulus()) >= 0) {
        return false;
    }
    // SP 800-186 sections 3.2.1.3 and 3.2.1.4: P-256/P-384 use y^2 = x^3 - 3x + b.
    const auto left = fieldSquare(point.y);
    const auto right = fieldAdd(
        fieldSubtract(fieldMultiply(fieldSquare(point.x), point.x), fieldMultiplySmall(point.x, 3U)), coefficientB());
    // Both curves have cofactor one, so a finite point satisfying the curve equation is in the required subgroup.
    return compare(left, right) == 0;
}

auto NistPrimeCurve::doublePoint(const Point &point) const noexcept -> Point {
    if (isIdentity(point) || isZero(point.y)) {
        return identity();
    }
    // FIPS 186-5 section 6.4.2 step 6 requires group doubling. For a = -3, the direct Jacobian formulas use
    // delta=Z^2, gamma=Y^2, beta=X*gamma, and alpha=3*(X-delta)*(X+delta).
    const auto delta = fieldSquare(point.z);
    const auto gamma = fieldSquare(point.y);
    const auto beta = fieldMultiply(point.x, gamma);
    const auto alpha = fieldMultiplySmall(fieldMultiply(fieldSubtract(point.x, delta), fieldAdd(point.x, delta)), 3U);
    const auto x = fieldSubtract(fieldSquare(alpha), fieldMultiplySmall(beta, 8U));
    const auto z = fieldSubtract(fieldSubtract(fieldSquare(fieldAdd(point.y, point.z)), gamma), delta);
    const auto y = fieldSubtract(
        fieldMultiply(alpha, fieldSubtract(fieldMultiplySmall(beta, 4U), x)),
        fieldMultiplySmall(fieldSquare(gamma), 8U));
    return Point{.x = x, .y = y, .z = z};
}

auto NistPrimeCurve::addMixed(const Point &left, const Point &rightAffine) const noexcept -> Point {
    if (isIdentity(left)) {
        return rightAffine;
    }
    if (isIdentity(rightAffine)) {
        return left;
    }
    // FIPS 186-5 section 6.4.2 step 6 requires point addition. Translate affine Q into left's Jacobian scale.
    const auto z1z1 = fieldSquare(left.z);
    const auto u2 = fieldMultiply(rightAffine.x, z1z1);
    const auto s2 = fieldMultiply(rightAffine.y, fieldMultiply(left.z, z1z1));
    const auto h = fieldSubtract(u2, left.x);
    if (isZero(h)) {
        return compare(s2, left.y) == 0 ? doublePoint(left) : identity();
    }

    // Complete the mixed Jacobian addition with H=U2-X1, I=4H^2, J=H*I, r=2(S2-Y1), and V=X1*I.
    const auto hh = fieldSquare(h);
    const auto i = fieldMultiplySmall(hh, 4U);
    const auto j = fieldMultiply(h, i);
    const auto r = fieldMultiplySmall(fieldSubtract(s2, left.y), 2U);
    const auto v = fieldMultiply(left.x, i);
    const auto x = fieldSubtract(fieldSubtract(fieldSquare(r), j), fieldMultiplySmall(v, 2U));
    const auto y =
        fieldSubtract(fieldMultiply(r, fieldSubtract(v, x)), fieldMultiplySmall(fieldMultiply(left.y, j), 2U));
    const auto z = fieldSubtract(fieldSubtract(fieldSquare(fieldAdd(left.z, h)), z1z1), hh);
    return Point{.x = x, .y = y, .z = z};
}

auto NistPrimeCurve::doubleSecret(const Point &point) const noexcept -> Point {
    // FIPS 186-5 section 6.4.1 step 3: use the same direct a=-3 Jacobian formulas as public doubling, then mask the
    // defined identity result for Z=0 or Y=0 without changing the arithmetic schedule.
    const auto delta = fieldSquare(point.z);
    const auto gamma = fieldSquare(point.y);
    const auto beta = fieldMultiply(point.x, gamma);
    const auto alpha = fieldMultiplySmall(fieldMultiply(fieldSubtract(point.x, delta), fieldAdd(point.x, delta)), 3U);
    const auto x = fieldSubtract(fieldSquare(alpha), fieldMultiplySmall(beta, 8U));
    const auto z = fieldSubtract(fieldSubtract(fieldSquare(fieldAdd(point.y, point.z)), gamma), delta);
    const auto y = fieldSubtract(
        fieldMultiply(alpha, fieldSubtract(fieldMultiplySmall(beta, 4U), x)),
        fieldMultiplySmall(fieldSquare(gamma), 8U));
    const auto candidate = Point{.x = x, .y = y, .z = z};
    return selectPoint(candidate, identity(), zeroMask(point.z) | zeroMask(point.y));
}

auto NistPrimeCurve::addMixedSecret(const Point &left, const Point &rightAffine) const noexcept -> Point {
    // FIPS 186-5 section 6.4.1 step 3: evaluate mixed Jacobian addition and all exceptional alternatives, then select
    // by masks. The right input is the fixed affine base point during signing.
    const auto z1z1 = fieldSquare(left.z);
    const auto u2 = fieldMultiply(rightAffine.x, z1z1);
    const auto s2 = fieldMultiply(rightAffine.y, fieldMultiply(left.z, z1z1));
    const auto h = fieldSubtract(u2, left.x);
    const auto hh = fieldSquare(h);
    const auto i = fieldMultiplySmall(hh, 4U);
    const auto j = fieldMultiply(h, i);
    const auto r = fieldMultiplySmall(fieldSubtract(s2, left.y), 2U);
    const auto v = fieldMultiply(left.x, i);
    const auto x = fieldSubtract(fieldSubtract(fieldSquare(r), j), fieldMultiplySmall(v, 2U));
    const auto y =
        fieldSubtract(fieldMultiply(r, fieldSubtract(v, x)), fieldMultiplySmall(fieldMultiply(left.y, j), 2U));
    const auto z = fieldSubtract(fieldSubtract(fieldSquare(fieldAdd(left.z, h)), z1z1), hh);
    auto result = Point{.x = x, .y = y, .z = z};

    const auto sameXMask = zeroMask(h);
    const auto samePointMask = sameXMask & equalMask(s2, left.y);
    const auto inversePointMask = sameXMask & ~samePointMask;
    result = selectPoint(result, doubleSecret(left), samePointMask);
    result = selectPoint(result, identity(), inversePointMask);
    result = selectPoint(result, rightAffine, zeroMask(left.z));
    return result;
}

auto NistPrimeCurve::selectPoint(const Point &first, const Point &second, const uint32_t secondMask) const noexcept
    -> Point {
    return Point{
        .x = selectNumber(first.x, second.x, secondMask),
        .y = selectNumber(first.y, second.y, secondMask),
        .z = selectNumber(first.z, second.z, secondMask)};
}

}
