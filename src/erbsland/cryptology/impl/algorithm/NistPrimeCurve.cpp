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

const NistPrimeCurve::Number NistPrimeCurve::cOne{{1U}};

const NistPrimeCurve::Number NistPrimeCurve::cP256Modulus{{
    0xffffffffU,
    0xffffffffU,
    0xffffffffU,
    0x00000000U,
    0x00000000U,
    0x00000000U,
    0x00000001U,
    0xffffffffU,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP256Order{{
    0xfc632551U,
    0xf3b9cac2U,
    0xa7179e84U,
    0xbce6faadU,
    0xffffffffU,
    0xffffffffU,
    0x00000000U,
    0xffffffffU,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP256B{{
    0x27d2604bU,
    0x3bce3c3eU,
    0xcc53b0f6U,
    0x651d06b0U,
    0x769886bcU,
    0xb3ebbd55U,
    0xaa3a93e7U,
    0x5ac635d8U,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP256Gx{{
    0xd898c296U,
    0xf4a13945U,
    0x2deb33a0U,
    0x77037d81U,
    0x63a440f2U,
    0xf8bce6e5U,
    0xe12c4247U,
    0x6b17d1f2U,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP256Gy{{
    0x37bf51f5U,
    0xcbb64068U,
    0x6b315eceU,
    0x2bce3357U,
    0x7c0f9e16U,
    0x8ee7eb4aU,
    0xfe1a7f9bU,
    0x4fe342e2U,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP256FieldR2{{
    0x00000003U,
    0x00000000U,
    0xffffffffU,
    0xfffffffbU,
    0xfffffffeU,
    0xffffffffU,
    0xfffffffdU,
    0x00000004U,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP256OrderR2{{
    0xbe79eea2U,
    0x83244c95U,
    0x49bd6fa6U,
    0x4699799cU,
    0x2b6bec59U,
    0x2845b239U,
    0xf3d95620U,
    0x66e12d94U,
}};

const NistPrimeCurve::Number NistPrimeCurve::cP384Modulus{{
    0xffffffffU,
    0x00000000U,
    0x00000000U,
    0xffffffffU,
    0xfffffffeU,
    0xffffffffU,
    0xffffffffU,
    0xffffffffU,
    0xffffffffU,
    0xffffffffU,
    0xffffffffU,
    0xffffffffU,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP384Order{{
    0xccc52973U,
    0xecec196aU,
    0x48b0a77aU,
    0x581a0db2U,
    0xf4372ddfU,
    0xc7634d81U,
    0xffffffffU,
    0xffffffffU,
    0xffffffffU,
    0xffffffffU,
    0xffffffffU,
    0xffffffffU,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP384B{{
    0xd3ec2aefU,
    0x2a85c8edU,
    0x8a2ed19dU,
    0xc656398dU,
    0x5013875aU,
    0x0314088fU,
    0xfe814112U,
    0x181d9c6eU,
    0xe3f82d19U,
    0x988e056bU,
    0xe23ee7e4U,
    0xb3312fa7U,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP384Gx{{
    0x72760ab7U,
    0x3a545e38U,
    0xbf55296cU,
    0x5502f25dU,
    0x82542a38U,
    0x59f741e0U,
    0x8ba79b98U,
    0x6e1d3b62U,
    0xf320ad74U,
    0x8eb1c71eU,
    0xbe8b0537U,
    0xaa87ca22U,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP384Gy{{
    0x90ea0e5fU,
    0x7a431d7cU,
    0x1d7e819dU,
    0x0a60b1ceU,
    0xb5f0b8c0U,
    0xe9da3113U,
    0x289a147cU,
    0xf8f41dbdU,
    0x9292dc29U,
    0x5d9e98bfU,
    0x96262c6fU,
    0x3617de4aU,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP384FieldR2{{
    0x00000001U,
    0xfffffffeU,
    0x00000000U,
    0x00000002U,
    0x00000000U,
    0xfffffffeU,
    0x00000000U,
    0x00000002U,
    0x00000001U,
}};
const NistPrimeCurve::Number NistPrimeCurve::cP384OrderR2{{
    0x19b409a9U,
    0x2d319b24U,
    0xdf1aa419U,
    0xff3d81e5U,
    0xfcb82947U,
    0xbc3e483aU,
    0x4aab1cc5U,
    0xd40d4917U,
    0x28266895U,
    0x3fb05b7aU,
    0x2b39bf21U,
    0x0c84ee01U,
}};

NistPrimeCurve::NistPrimeCurve(const Name name) noexcept : _name{name} {
}

auto NistPrimeCurve::byteLength() const noexcept -> std::size_t {
    return _name == Name::P256 ? 32U : 48U;
}

auto NistPrimeCurve::order() const noexcept -> const Number & {
    return _name == Name::P256 ? cP256Order : cP384Order;
}

auto NistPrimeCurve::basePoint() const noexcept -> Point {
    return _name == Name::P256 ? affine(cP256Gx, cP256Gy) : affine(cP384Gx, cP384Gy);
}

auto NistPrimeCurve::fieldModulus() const noexcept -> const Number & {
    return _name == Name::P256 ? cP256Modulus : cP384Modulus;
}

auto NistPrimeCurve::coefficientB() const noexcept -> const Number & {
    return _name == Name::P256 ? cP256B : cP384B;
}

auto NistPrimeCurve::numberFromBigEndian(const mem::ConstByteSpan bytes) const noexcept -> std::optional<Number> {
    if (bytes.size() > byteLength()) {
        return std::nullopt;
    }
    auto result = Number{};
    for (auto inputIndex = std::size_t{}; inputIndex < bytes.size(); ++inputIndex) {
        const auto reverseIndex = bytes.size() - inputIndex - 1U;
        const auto wordIndex = inputIndex / 4U;
        const auto shift = static_cast<unsigned>((inputIndex % 4U) * 8U);
        result.words[wordIndex] |= static_cast<uint32_t>(bytes[reverseIndex].toUInt8()) << shift;
    }
    return result;
}

auto NistPrimeCurve::compare(const Number &left, const Number &right) const noexcept -> int {
    for (auto index = byteLength() / 4U; index-- > 0U;) {
        if (left.words[index] < right.words[index]) {
            return -1;
        }
        if (left.words[index] > right.words[index]) {
            return 1;
        }
    }
    return 0;
}

auto NistPrimeCurve::isZero(const Number &value) const noexcept -> bool {
    for (auto index = std::size_t{}; index < byteLength() / 4U; ++index) {
        if (value.words[index] != 0U) {
            return false;
        }
    }
    return true;
}

auto NistPrimeCurve::bit(const Number &value, const std::size_t index) const noexcept -> bool {
    return ((value.words[index / 32U] >> (index % 32U)) & 1U) != 0U;
}

auto NistPrimeCurve::numberToBigEndian(const Number &value) const noexcept -> mem::ByteBlock {
    auto result = mem::ByteBlockEditor{unit::ByteLength::fromSizeT(byteLength())};
    for (auto outputIndex = std::size_t{}; outputIndex < byteLength(); ++outputIndex) {
        const auto inputIndex = byteLength() - outputIndex - 1U;
        const auto word = value.words[inputIndex / 4U];
        const auto shift = static_cast<unsigned>((inputIndex % 4U) * 8U);
        result.set(unit::ByteIndex::fromSizeT(outputIndex), mem::Byte{static_cast<uint8_t>(word >> shift)});
    }
    return mem::ByteBlock{result};
}

auto NistPrimeCurve::pointFromBytes(const mem::ConstByteSpan bytes) const noexcept -> std::optional<Point> {
    const auto coordinateLength = byteLength();
    if (bytes.empty()) {
        return std::nullopt;
    }
    const auto prefix = bytes.front().toUInt8();
    if (prefix == 0x04U) {
        // RFC 5480 section 2.2 and SEC 1 section 2.3.4: an uncompressed point is 0x04 || X || Y.
        if (bytes.size() != 1U + 2U * coordinateLength) {
            return std::nullopt;
        }
        const auto x = numberFromBigEndian(bytes.subspan(1U, coordinateLength));
        const auto y = numberFromBigEndian(bytes.subspan(1U + coordinateLength, coordinateLength));
        if (!x.has_value() || !y.has_value() || compare(*x, fieldModulus()) >= 0 || compare(*y, fieldModulus()) >= 0) {
            return std::nullopt;
        }
        const auto result = affine(*x, *y);
        return isOnCurve(result) ? std::optional<Point>{result} : std::nullopt;
    }
    if (prefix != 0x02U && prefix != 0x03U) {
        // RFC 5480 section 2.2 rejects the identity, hybrid encodings, and every unknown point prefix.
        return std::nullopt;
    }
    if (bytes.size() != 1U + coordinateLength) {
        return std::nullopt;
    }
    const auto x = numberFromBigEndian(bytes.subspan(1U, coordinateLength));
    if (!x.has_value() || compare(*x, fieldModulus()) >= 0) {
        return std::nullopt;
    }

    // SEC 1 section 2.3.4: recover y from alpha = x^3 + ax + b, where a = -3 for both selected curves.
    const auto xSquared = fieldSquare(*x);
    const auto xCubed = fieldMultiply(xSquared, *x);
    const auto alpha = fieldAdd(fieldSubtract(xCubed, fieldMultiplySmall(*x, 3U)), coefficientB());
    auto y = fieldSquareRoot(alpha);
    if (!y.has_value()) {
        return std::nullopt;
    }

    // SEC 1 section 2.3.4 step 2.4: choose the root whose least-significant bit matches the compressed prefix.
    const auto requestedOdd = prefix == 0x03U;
    if (bit(*y, 0U) != requestedOdd) {
        *y = fieldSubtract(Number{}, *y);
    }
    if (bit(*y, 0U) != requestedOdd) {
        return std::nullopt;
    }
    const auto result = affine(*x, *y);
    return isOnCurve(result) ? std::optional<Point>{result} : std::nullopt;
}

auto NistPrimeCurve::isIdentity(const Point &point) const noexcept -> bool {
    return isZero(point.z);
}

auto NistPrimeCurve::toAffine(const Point &point) const noexcept -> std::optional<Point> {
    if (isIdentity(point)) {
        return std::nullopt;
    }
    // SEC 1 section 2.2: Jacobian (X,Y,Z) represents affine (X/Z^2,Y/Z^3); perform one field inversion.
    const auto inverseZ = fieldInvert(point.z);
    const auto inverseZSquared = fieldSquare(inverseZ);
    const auto x = fieldMultiply(point.x, inverseZSquared);
    const auto y = fieldMultiply(point.y, fieldMultiply(inverseZSquared, inverseZ));
    return affine(x, y);
}

auto NistPrimeCurve::reduceOrder(const Number &value) const noexcept -> Number {
    return compare(value, order()) >= 0 ? subtract(value, order()) : value;
}

auto NistPrimeCurve::multiplyOrder(const Number &left, const Number &right) const noexcept -> Number {
    return multiplyModulo(left, right, order());
}

auto NistPrimeCurve::invertOrder(const Number &value) const noexcept -> Number {
    // FIPS 186-5 section 6.4.2 step 4: compute s^-1 modulo the prime order n using s^(n-2).
    return powerModulo(value, subtractSmall(order(), 2U), order());
}

auto NistPrimeCurve::addPublic(const Point &left, const Point &right) const noexcept -> Point {
    if (isIdentity(left)) {
        return right;
    }
    if (isIdentity(right)) {
        return left;
    }
    const auto affineRight = toAffine(right);
    return affineRight.has_value() ? addMixed(left, *affineRight) : left;
}

auto NistPrimeCurve::multiplyPublic(const Point &point, const Number &scalar) const noexcept -> Point {
    // FIPS 186-5 section 6.4.2 step 6 requires public scalar multiplication. This direct MSB-first double-and-add
    // branches on public scalar bits and is intentionally not a secret-scalar API.
    auto result = identity();
    for (auto index = byteLength() * 8U; index-- > 0U;) {
        result = doublePoint(result);
        if (bit(scalar, index)) {
            result = addMixed(result, point);
        }
    }
    return result;
}

auto NistPrimeCurve::addOrderSecret(const Number &left, const Number &right) const noexcept -> Number {
    auto sum = Number{};
    auto carry = uint64_t{};
    for (auto index = std::size_t{}; index < byteLength() / 4U; ++index) {
        const auto wordSum = uint64_t{left.words[index]} + right.words[index] + carry;
        sum.words[index] = static_cast<uint32_t>(wordSum);
        carry = wordSum >> 32U;
    }
    return reduceOnceSecret(sum, order(), static_cast<uint32_t>(carry));
}

auto NistPrimeCurve::reduceOrderSecret(const Number &value) const noexcept -> Number {
    return reduceOnceSecret(value, order());
}

auto NistPrimeCurve::multiplyOrderSecret(const Number &left, const Number &right) const noexcept -> Number {
    return multiplyModulo(left, right, order());
}

auto NistPrimeCurve::invertOrderSecret(const Number &value) const noexcept -> Number {
    // FIPS 186-5 section 6.4.1 step 4: k^-1 = k^(n-2) mod n. The exponent and its branch schedule are public and
    // fixed; Montgomery operands and table-free multiplication retain no input-dependent loop or lookup.
    return powerModulo(value, subtractSmall(order(), 2U), order());
}

auto NistPrimeCurve::multiplyBaseSecret(const Number &scalar) const noexcept -> Point {
    auto result = identity();
    const auto base = basePoint();
    // FIPS 186-5 section 6.4.1 step 3: R = [k]G. Always compute both candidates and select with a mask for all 256
    // P-256 scalar bits; no branch, loop bound, or memory index depends on k.
    for (auto index = byteLength() * 8U; index-- > 0U;) {
        const auto doubled = doubleSecret(result);
        const auto added = addMixedSecret(doubled, base);
        const auto mask = uint32_t{0U} - static_cast<uint32_t>(bit(scalar, index));
        result = selectPoint(doubled, added, mask);
    }
    return result;
}

auto NistPrimeCurve::toAffineSecret(const Point &point) const noexcept -> Point {
    // SEC 1 section 2.2: Jacobian (X,Y,Z) maps to (X/Z^2,Y/Z^3). Signing produces a nonidentity subgroup point.
    const auto inverseZ = fieldInvert(point.z);
    const auto inverseZSquared = fieldSquare(inverseZ);
    return affine(
        fieldMultiply(point.x, inverseZSquared), fieldMultiply(point.y, fieldMultiply(inverseZSquared, inverseZ)));
}

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
