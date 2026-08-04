// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FieldElement.hpp"

#include "../../../../mem/Byte.hpp"
#include "../../../../mem/SecureErase.hpp"
#include "../../../../unit/ByteIndex.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::cryptology::impl::ed25519_signature {

using namespace unit;

auto FieldElement::add(const FieldElement &a, const FieldElement &b) noexcept -> FieldElement {
    // RFC 8032 section 5.1.1: every coordinate operation is in GF(p), where p = 2^255 - 19.
    auto result = FieldElement{};
    for (auto i = std::size_t{0}; i < result._limbs.size(); ++i) {
        result._limbs[i] = a._limbs[i] + b._limbs[i];
    }
    result.normalize();
    return result;
}

auto FieldElement::subtract(const FieldElement &a, const FieldElement &b) noexcept -> FieldElement {
    // RFC 8032 section 5.1.1: add 2p limb-wise before subtraction so the unsigned representation cannot underflow.
    auto result = FieldElement{};
    for (auto i = std::size_t{0}; i < result._limbs.size(); ++i) {
        const auto base = uint64_t{1U} << limbBits(i);
        const auto modulusLimb = i == 0U ? base - 19U : base - 1U;
        result._limbs[i] = a._limbs[i] + (2U * modulusLimb) - b._limbs[i];
    }
    result.normalize();
    return result;
}

auto FieldElement::multiply(const FieldElement &a, const FieldElement &b) noexcept -> FieldElement {
    // RFC 8032 section 5.1.1 and RFC 7748 section 4.1: use ten little-endian limbs with alternating 26/25-bit
    // widths. Each aligned product is below 2^53, each slot accumulates at most ten products below 2^57, and folding
    // one high slot by 19 keeps the destination below 2^62. Every value therefore fits in uint64_t on all supported
    // platforms without a nonportable 128-bit integer.
    auto product = std::array<uint64_t, 19U>{};
    for (auto i = std::size_t{0}; i < a._limbs.size(); ++i) {
        for (auto j = std::size_t{0}; j < b._limbs.size(); ++j) {
            const auto alignmentFactor = ((i & 1U) != 0U && (j & 1U) != 0U) ? uint64_t{2U} : uint64_t{1U};
            product[i + j] += a._limbs[i] * b._limbs[j] * alignmentFactor;
        }
    }

    // RFC 8032 section 5.1.1: because 2^255 is congruent to 19 modulo p, fold every high coefficient by 19.
    for (auto i = product.size(); i-- > 10U;) {
        product[i - 10U] += product[i] * 19U;
    }
    auto result = FieldElement{};
    for (auto i = std::size_t{0}; i < result._limbs.size(); ++i) {
        result._limbs[i] = product[i];
    }
    result.normalize();
    return result;
}

auto FieldElement::square(const FieldElement &value) noexcept -> FieldElement {
    return multiply(value, value);
}

auto FieldElement::multiplySmall(const FieldElement &value, const uint64_t factor) noexcept -> FieldElement {
    auto result = FieldElement{};
    for (auto i = std::size_t{0}; i < result._limbs.size(); ++i) {
        result._limbs[i] = value._limbs[i] * factor;
    }
    result.normalize();
    return result;
}

auto FieldElement::powerPMinus5Over8(const FieldElement &value) noexcept -> FieldElement {
    // RFC 8032 section 5.1.3 step 2: exponentiate by (p-5)/8 = 2^252-3. Its bits are 251..2 and 0, so this fixed
    // square-and-multiply schedule is independent of all point coordinates.
    auto result = one();
    for (auto bit = 252U; bit-- > 0U;) {
        result = square(result);
        if (bit >= 2U || bit == 0U) {
            result = multiply(result, value);
        }
    }
    return result;
}

auto FieldElement::invert(const FieldElement &value) noexcept -> FieldElement {
    // RFC 8032 section 5.1.1: inversion in GF(p) uses z^(p-2), where p-2 = 2^255-21. The exponent is public and
    // fixed, so all inputs execute the same 255 squarings and the same predetermined multiplications.
    auto result = one();
    for (auto bit = 255U; bit-- > 0U;) {
        result = square(result);
        const auto multiplyAtBit = bit >= 5U || bit == 3U || bit == 1U || bit == 0U;
        if (multiplyAtBit) {
            result = multiply(result, value);
        }
    }
    return result;
}

auto FieldElement::select(const FieldElement &first, const FieldElement &second, const uint64_t selectSecond) noexcept
    -> FieldElement {
    const auto secondMask = uint64_t{0U} - (selectSecond & 1U);
    auto result = FieldElement{};
    for (auto index = std::size_t{}; index < result._limbs.size(); ++index) {
        result._limbs[index] = (first._limbs[index] & ~secondMask) | (second._limbs[index] & secondMask);
    }
    return result;
}

void FieldElement::secureErase() noexcept {
    mem::secureErase(std::span{_limbs});
}

auto FieldElement::equal(const FieldElement &a, const FieldElement &b) noexcept -> bool {
    return a.toBytes() == b.toBytes();
}

auto FieldElement::isZero() const noexcept -> bool {
    return equal(*this, zero());
}

auto FieldElement::isNegative() const noexcept -> bool {
    return (toBytes().get(ByteIndex::zero()).toUInt8() & 1U) != 0U;
}

auto FieldElement::zero() noexcept -> FieldElement {
    return {};
}

auto FieldElement::one() noexcept -> FieldElement {
    auto result = FieldElement{};
    result._limbs[0] = 1U;
    return result;
}

auto FieldElement::fromCanonicalBytes(const mem::ConstByteSpan bytes) noexcept -> std::optional<FieldElement> {
    if (bytes.size() != 32U || (bytes[31U].toUInt8() & 0x80U) != 0U) {
        return std::nullopt;
    }

    // RFC 8032 section 5.1.3 step 1: y must use the unique little-endian representative below p = 2^255 - 19.
    static constexpr auto modulus = std::array<uint8_t, 32U>{
        0xedU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0xffU,
        0x7fU};
    auto comparison = 0;
    for (auto i = bytes.size(); i-- > 0U;) {
        if (bytes[i].toUInt8() != modulus[i]) {
            comparison = bytes[i].toUInt8() < modulus[i] ? -1 : 1;
            break;
        }
    }
    if (comparison >= 0) {
        return std::nullopt;
    }

    auto result = FieldElement{};
    for (auto limb = std::size_t{0}; limb < result._limbs.size(); ++limb) {
        const auto offset = limbOffset(limb);
        const auto width = limbBits(limb);
        for (auto bit = std::size_t{0}; bit < width; ++bit) {
            const auto inputBit = offset + bit;
            const auto value = (bytes[inputBit / 8U].toUInt64() >> (inputBit % 8U)) & 1U;
            result._limbs[limb] |= value << bit;
        }
    }
    return result;
}

auto FieldElement::fromConstant(const std::array<uint8_t, 32U> &bytes) noexcept -> FieldElement {
    const auto result = fromCanonicalBytes(mem::toConstByteSpan(std::span{bytes}));
    return result.has_value() ? *result : FieldElement{};
}

auto FieldElement::toBytes() const noexcept -> mem::ByteArray<32U> {
    auto canonical = *this;
    canonical.normalize();
    auto result = mem::ByteArray<32U>{};
    for (auto limb = std::size_t{0}; limb < canonical._limbs.size(); ++limb) {
        const auto offset = limbOffset(limb);
        const auto width = limbBits(limb);
        for (auto bit = std::size_t{0}; bit < width; ++bit) {
            const auto outputBit = offset + bit;
            const auto byteIndex = ByteIndex::fromSizeT(outputBit / 8U);
            const auto value = static_cast<uint8_t>(((canonical._limbs[limb] >> bit) & 1U) << (outputBit % 8U));
            result.set(byteIndex, result.get(byteIndex) | mem::Byte{value});
        }
    }
    return result;
}

void FieldElement::normalize() noexcept {
    // RFC 8032 section 5.1.1: propagate carries in the 26/25-bit radix and fold the final 2^255 carry by 19. Four
    // fixed passes cover the largest product and small-constant bounds used by the point formulas.
    for (auto round = 0U; round < 4U; ++round) {
        for (auto i = std::size_t{0}; i < 9U; ++i) {
            const auto width = limbBits(i);
            const auto carry = _limbs[i] >> width;
            _limbs[i] &= (uint64_t{1U} << width) - 1U;
            _limbs[i + 1U] += carry;
        }
        const auto carry = _limbs[9U] >> 25U;
        _limbs[9U] &= (uint64_t{1U} << 25U) - 1U;
        _limbs[0] += carry * 19U;
    }
    subtractModulus();
    subtractModulus();
}

void FieldElement::subtractModulus() noexcept {
    auto difference = std::array<uint64_t, 10U>{};
    auto borrow = uint64_t{0U};
    for (auto i = std::size_t{0}; i < _limbs.size(); ++i) {
        const auto base = uint64_t{1U} << limbBits(i);
        const auto modulusLimb = i == 0U ? base - 19U : base - 1U;
        const auto subtrahend = modulusLimb + borrow;
        difference[i] = _limbs[i] - subtrahend;
        borrow = difference[i] >> 63U;
        difference[i] += borrow * base;
    }
    const auto keepOriginalMask = uint64_t{0U} - borrow;
    for (auto i = std::size_t{0}; i < _limbs.size(); ++i) {
        _limbs[i] = (difference[i] & ~keepOriginalMask) | (_limbs[i] & keepOriginalMask);
    }
}

}
