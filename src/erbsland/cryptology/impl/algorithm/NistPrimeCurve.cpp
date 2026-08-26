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

}
