// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Ed25519Signature.hpp"

#include "../../../../err/ParseError.hpp"
#include "../../../../mem/Byte.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../HashAlgorithm.hpp"
#include "../../../Hasher.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::cryptology::impl::ed25519_signature {

using namespace text::literals;
using namespace unit;

auto isSignatureAlgorithm(const X509AlgorithmIdentifier &algorithm) noexcept -> bool {
    return algorithm.oid().toString() == "1.3.101.112"_el;
}

auto verify(
    const PublicKey &publicKey,
    const X509AlgorithmIdentifier &signatureAlgorithm,
    const mem::ConstByteSpan message,
    const mem::ConstByteSpan signature) -> bool {
    requireEd25519Algorithm(signatureAlgorithm, "signature"_el);
    requireEd25519Algorithm(publicKey.algorithm(), "public key"_el);
    if (publicKey.unusedBitCount() != 0U || publicKey.keyData().length() != ByteLength{32U}) {
        throwParseError("An Ed25519 SubjectPublicKeyInfo must contain exactly 32 octets and no unused bits."_el);
    }
    if (signature.size() != 64U) {
        throwParseError("An Ed25519 signature must contain exactly 64 octets."_el);
    }

    // RFC 8032 section 5.1.7 step 1: split ENC(R) || ENC(S), decode R and A, and require 0 <= S < L.
    const auto rBytes = signature.first<32U>();
    const auto sBytes = signature.last<32U>();
    const auto publicPoint = decodePoint(publicKey.keyData().span());
    if (!publicPoint.has_value()) {
        throwParseError("The Ed25519 public key is not a canonical curve point."_el);
    }
    const auto r = decodePoint(rBytes);
    const auto s = decodeScalar(sBytes);
    if (!r.has_value() || !s.has_value()) {
        return false;
    }

    // Security hardening around RFC 8032 sections 5.1.7 and 8.8: use the permitted exact equation below and reject
    // identity, torsion, and mixed-order encodings so every accepted A and R belongs to the prime-order subgroup.
    if (!isPrimeOrderPoint(*publicPoint)) {
        throwParseError("The Ed25519 public key is not a nonzero prime-order point."_el);
    }
    if (!isPrimeOrderPoint(*r)) {
        return false;
    }

    // RFC 8032 section 5.1.7 step 2: k = SHA-512(ENC(R) || ENC(A) || M), interpreted little-endian modulo L.
    auto hasher = Hasher{HashAlgorithm::Sha2_512};
    hasher.update(rBytes);
    hasher.update(publicKey.keyData().span());
    hasher.update(message);
    const auto k = reduceScalar(hasher.finalize().span());

    // RFC 8032 section 5.1.7 step 3: use the explicitly permitted sufficient equation [S]B = R + [k]A. Scalar
    // multiplication is variable-time because S, k, A, and R are public verification values and no secret exists.
    const auto left = multiply(basePoint(), s->span());
    const auto right = add(*r, multiply(*publicPoint, k.span()));
    return equal(left, right);
}

auto decodePoint(const mem::ConstByteSpan bytes) noexcept -> std::optional<Point> {
    if (bytes.size() != 32U) {
        return std::nullopt;
    }

    // RFC 8032 section 5.1.3 step 1: extract x_0 from bit 255 and decode the remaining bits as canonical y < p.
    const auto xSign = (bytes[31U].toUInt8() >> 7U) != 0U;
    auto yEncoding = std::array<uint8_t, 32U>{};
    for (auto i = std::size_t{0}; i < yEncoding.size(); ++i) {
        yEncoding[i] = bytes[i].toUInt8();
    }
    yEncoding[31U] &= 0x7fU;
    const auto y = FieldElement::fromCanonicalBytes(mem::toConstByteSpan(std::span{yEncoding}));
    if (!y.has_value()) {
        return std::nullopt;
    }

    // RFC 8032 section 5.1.3 step 2: u = y^2-1 and v = d*y^2+1, then
    // x = u*v^3*(u*v^7)^((p-5)/8). The fixed d encoding is RFC 8032 section 5.1's -121665/121666 modulo p.
    static constexpr auto dBytes = std::array<uint8_t, 32U>{
        0xa3U,
        0x78U,
        0x59U,
        0x13U,
        0xcaU,
        0x4dU,
        0xebU,
        0x75U,
        0xabU,
        0xd8U,
        0x41U,
        0x41U,
        0x4dU,
        0x0aU,
        0x70U,
        0x00U,
        0x98U,
        0xe8U,
        0x79U,
        0x77U,
        0x79U,
        0x40U,
        0xc7U,
        0x8cU,
        0x73U,
        0xfeU,
        0x6fU,
        0x2bU,
        0xeeU,
        0x6cU,
        0x03U,
        0x52U};
    const auto d = FieldElement::fromConstant(dBytes);
    const auto ySquared = FieldElement::square(*y);
    const auto u = FieldElement::subtract(ySquared, FieldElement::one());
    const auto v = FieldElement::add(FieldElement::multiply(d, ySquared), FieldElement::one());
    const auto vSquared = FieldElement::square(v);
    const auto vCubed = FieldElement::multiply(vSquared, v);
    const auto vSeventh = FieldElement::multiply(FieldElement::square(vCubed), v);
    auto x = FieldElement::multiply(
        FieldElement::multiply(u, vCubed), FieldElement::powerPMinus5Over8(FieldElement::multiply(u, vSeventh)));

    // RFC 8032 section 5.1.3 step 3: accept v*x^2=u, or correct v*x^2=-u by multiplying with sqrt(-1).
    const auto check = FieldElement::multiply(v, FieldElement::square(x));
    if (!FieldElement::equal(check, u)) {
        if (!FieldElement::equal(check, FieldElement::subtract(FieldElement::zero(), u))) {
            return std::nullopt;
        }
        static constexpr auto sqrtMinusOneBytes = std::array<uint8_t, 32U>{
            0xb0U,
            0xa0U,
            0x0eU,
            0x4aU,
            0x27U,
            0x1bU,
            0xeeU,
            0xc4U,
            0x78U,
            0xe4U,
            0x2fU,
            0xadU,
            0x06U,
            0x18U,
            0x43U,
            0x2fU,
            0xa7U,
            0xd7U,
            0xfbU,
            0x3dU,
            0x99U,
            0x00U,
            0x4dU,
            0x2bU,
            0x0bU,
            0xdfU,
            0xc1U,
            0x4fU,
            0x80U,
            0x24U,
            0x83U,
            0x2bU};
        x = FieldElement::multiply(x, FieldElement::fromConstant(sqrtMinusOneBytes));
    }

    // RFC 8032 section 5.1.3 step 4: reject negative zero and select the root whose low bit equals x_0.
    if (x.isZero() && xSign) {
        return std::nullopt;
    }
    if (x.isNegative() != xSign) {
        x = FieldElement::subtract(FieldElement::zero(), x);
    }
    return Point{x, *y, FieldElement::one(), FieldElement::multiply(x, *y)};
}

auto basePoint() noexcept -> Point {
    // RFC 8032 sections 5.1 and 5.1.2: B has the canonical encoding 0x58 followed by thirty-one 0x66 octets.
    auto encoding = std::array<uint8_t, 32U>{};
    encoding.fill(0x66U);
    encoding[0U] = 0x58U;
    const auto point = decodePoint(mem::toConstByteSpan(std::span{encoding}));
    return point.has_value() ? *point : identity();
}

auto identity() noexcept -> Point {
    return Point{FieldElement::zero(), FieldElement::one(), FieldElement::one(), FieldElement::zero()};
}

auto add(const Point &first, const Point &second) noexcept -> Point {
    // RFC 8032 section 5.1.4: retain the specification's A--H names and complete extended-coordinate addition order.
    static constexpr auto dBytes = std::array<uint8_t, 32U>{
        0xa3U,
        0x78U,
        0x59U,
        0x13U,
        0xcaU,
        0x4dU,
        0xebU,
        0x75U,
        0xabU,
        0xd8U,
        0x41U,
        0x41U,
        0x4dU,
        0x0aU,
        0x70U,
        0x00U,
        0x98U,
        0xe8U,
        0x79U,
        0x77U,
        0x79U,
        0x40U,
        0xc7U,
        0x8cU,
        0x73U,
        0xfeU,
        0x6fU,
        0x2bU,
        0xeeU,
        0x6cU,
        0x03U,
        0x52U};
    const auto a =
        FieldElement::multiply(FieldElement::subtract(first.y, first.x), FieldElement::subtract(second.y, second.x));
    const auto b = FieldElement::multiply(FieldElement::add(first.y, first.x), FieldElement::add(second.y, second.x));
    const auto c = FieldElement::multiply(
        FieldElement::multiplySmall(FieldElement::fromConstant(dBytes), 2U), FieldElement::multiply(first.t, second.t));
    const auto d = FieldElement::multiplySmall(FieldElement::multiply(first.z, second.z), 2U);
    const auto e = FieldElement::subtract(b, a);
    const auto f = FieldElement::subtract(d, c);
    const auto g = FieldElement::add(d, c);
    const auto h = FieldElement::add(b, a);
    return Point{
        FieldElement::multiply(e, f),
        FieldElement::multiply(g, h),
        FieldElement::multiply(f, g),
        FieldElement::multiply(e, h)};
}

auto doublePoint(const Point &point) noexcept -> Point {
    // RFC 8032 section 5.1.4: retain the specification's A--H names and complete extended-coordinate doubling order.
    const auto a = FieldElement::square(point.x);
    const auto b = FieldElement::square(point.y);
    const auto c = FieldElement::multiplySmall(FieldElement::square(point.z), 2U);
    const auto h = FieldElement::add(a, b);
    const auto e = FieldElement::subtract(h, FieldElement::square(FieldElement::add(point.x, point.y)));
    const auto g = FieldElement::subtract(a, b);
    const auto f = FieldElement::add(c, g);
    return Point{
        FieldElement::multiply(e, f),
        FieldElement::multiply(g, h),
        FieldElement::multiply(f, g),
        FieldElement::multiply(e, h)};
}

auto multiply(const Point &point, const mem::ConstByteSpan scalar) noexcept -> Point {
    auto result = identity();
    if (scalar.size() != 32U) {
        return result;
    }

    // RFC 8032 section 5.1.7 notation [n]X: process all 256 little-endian scalar bits from most to least
    // significant. Branches are permitted because verification scalars and points are entirely public.
    for (auto position = 256U; position-- > 0U;) {
        result = doublePoint(result);
        const auto bit = (scalar[position / 8U].toUInt8() >> (position & 7U)) & 1U;
        if (bit != 0U) {
            result = add(result, point);
        }
    }
    return result;
}

auto equal(const Point &first, const Point &second) noexcept -> bool {
    // RFC 8032 section 5.1.4: x=X/Z and y=Y/Z, so compare both coordinates by cross multiplication.
    return FieldElement::equal(FieldElement::multiply(first.x, second.z), FieldElement::multiply(second.x, first.z)) &&
        FieldElement::equal(FieldElement::multiply(first.y, second.z), FieldElement::multiply(second.y, first.z));
}

auto isPrimeOrderPoint(const Point &point) noexcept -> bool {
    // RFC 8032 section 5.1 gives cofactor 8 and prime subgroup order L. Requiring [L]P=0 and P!=0 excludes every
    // identity, low-order, and mixed-order encoding before the sufficient non-cofactored verification equation.
    return !equal(point, identity()) && equal(multiply(point, subgroupOrderBytes().span()), identity());
}

auto decodeScalar(const mem::ConstByteSpan bytes) noexcept -> std::optional<mem::ByteArray<32U>> {
    if (bytes.size() != 32U) {
        return std::nullopt;
    }
    const auto order = subgroupOrderBytes();
    auto comparison = 0;
    for (auto i = bytes.size(); i-- > 0U;) {
        if (bytes[i] != order.get(ByteIndex::fromSizeT(i))) {
            comparison = bytes[i].toUInt8() < order.get(ByteIndex::fromSizeT(i)).toUInt8() ? -1 : 1;
            break;
        }
    }
    if (comparison >= 0) {
        return std::nullopt;
    }
    return mem::ByteArray<32U>::fromSpanOrThrow(bytes);
}

auto reduceScalar(const mem::ConstByteSpan bytes) noexcept -> mem::ByteArray<32U> {
    auto remainder = Scalar{};
    if (bytes.size() != 64U) {
        return {};
    }

    // RFC 8032 section 5.1.7 step 2: interpret SHA-512 little-endian and reduce modulo L. Fixed bitwise long division
    // keeps the representation at eight 32-bit limbs and bounds every intermediate below 2L.
    const auto modulus = subgroupOrder();
    for (auto position = 512U; position-- > 0U;) {
        auto carry = static_cast<uint32_t>((bytes[position / 8U].toUInt8() >> (position & 7U)) & 1U);
        for (auto &limb : remainder) {
            const auto nextCarry = limb >> 31U;
            limb = static_cast<uint32_t>((limb << 1U) | carry);
            carry = nextCarry;
        }
        if (compareScalar(remainder, modulus) >= 0) {
            subtractScalar(remainder, modulus);
        }
    }

    auto result = mem::ByteArray<32U>{};
    for (auto limb = std::size_t{0}; limb < remainder.size(); ++limb) {
        for (auto byte = std::size_t{0}; byte < 4U; ++byte) {
            result.set(
                ByteIndex::fromSizeT((limb * 4U) + byte),
                mem::Byte{static_cast<uint8_t>(remainder[limb] >> (byte * 8U))});
        }
    }
    return result;
}

auto compareScalar(const Scalar &a, const Scalar &b) noexcept -> int {
    for (auto i = a.size(); i-- > 0U;) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
    }
    return 0;
}

void subtractScalar(Scalar &a, const Scalar &b) noexcept {
    auto borrow = uint64_t{0U};
    for (auto i = std::size_t{0}; i < a.size(); ++i) {
        const auto minuend = uint64_t{a[i]};
        const auto subtrahend = uint64_t{b[i]} + borrow;
        a[i] = static_cast<uint32_t>(minuend - subtrahend);
        borrow = minuend < subtrahend ? 1U : 0U;
    }
}

auto subgroupOrderBytes() noexcept -> mem::ByteArray<32U> {
    auto result = mem::ByteArray<32U>{};
    const auto order = subgroupOrder();
    for (auto limb = std::size_t{0}; limb < order.size(); ++limb) {
        for (auto byte = std::size_t{0}; byte < 4U; ++byte) {
            result.set(
                ByteIndex::fromSizeT((limb * 4U) + byte), mem::Byte{static_cast<uint8_t>(order[limb] >> (byte * 8U))});
        }
    }
    return result;
}

void requireEd25519Algorithm(const X509AlgorithmIdentifier &algorithm, [[maybe_unused]] const text::String &use) {
    // RFC 8410 section 3: id-Ed25519 is 1.3.101.112 and AlgorithmIdentifier parameters MUST be absent for keys and
    // signatures. In particular, a DER NULL is not an interoperable alternative.
    if (!isSignatureAlgorithm(algorithm)) {
        throwParseError("The X.509 algorithm is not id-Ed25519."_el);
    }
    if (!algorithm.parameters().isEmpty()) {
        throwParseError("Ed25519 AlgorithmIdentifier parameters must be absent."_el);
    }
}

void throwParseError(const text::String &reason) {
    throw err::ParseError{reason};
}

}
