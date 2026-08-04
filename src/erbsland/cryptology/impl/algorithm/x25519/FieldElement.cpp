// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FieldElement.hpp"

#include "../../../../mem/SecureErase.hpp"
#include "../../../../unit/ByteIndex.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace erbsland::cryptology::impl::x25519 {

FieldElement::~FieldElement() {
    secureErase();
}

auto FieldElement::add(const FieldElement &a, const FieldElement &b) noexcept -> FieldElement {
    // RFC 7748 section 5: all ladder arithmetic is in GF(2^255 - 19); normalize after each field operation.
    auto result = FieldElement{};
    for (auto i = std::size_t{0}; i < result._limbs.size(); ++i) {
        result._limbs[i] = a._limbs[i] + b._limbs[i];
    }
    result.normalize();
    return result;
}

auto FieldElement::subtract(const FieldElement &a, const FieldElement &b) noexcept -> FieldElement {
    // RFC 7748 section 5: add 2p limb-wise before subtracting so unsigned subtraction cannot underflow.
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
    // RFC 7748 section 4.1: represent GF(2^255 - 19) with ten little-endian limbs of alternating width 26/25.
    // Normalized inputs are below p. Each product slot accumulates at most ten products below 2^52, including the
    // mixed-radix alignment factor, so the unreduced value fits uint64_t without requiring a 128-bit integer.
    auto product = std::array<uint64_t, 19U>{};
    for (auto i = std::size_t{0}; i < a._limbs.size(); ++i) {
        for (auto j = std::size_t{0}; j < b._limbs.size(); ++j) {
            // Two 25-bit odd limbs start one bit below the offset implied by their summed limb indices.
            const auto alignmentFactor = ((i & 1U) != 0U && (j & 1U) != 0U) ? uint64_t{2U} : uint64_t{1U};
            product[i + j] += a._limbs[i] * b._limbs[j] * alignmentFactor;
        }
    }

    // RFC 7748 section 4.1: p = 2^255 - 19, hence every coefficient at 2^255 or above folds back by a factor of 19.
    for (auto i = product.size(); i-- > 10U;) {
        product[i - 10U] += product[i] * 19U;
    }
    auto result = FieldElement{};
    for (auto i = std::size_t{0}; i < result._limbs.size(); ++i) {
        result._limbs[i] = product[i];
    }
    result.normalize();

    // Product coefficients can depend on the private scalar through ladder state; erase them before releasing storage.
    mem::secureErase(std::span{product});
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

auto FieldElement::invert(const FieldElement &value) noexcept -> FieldElement {
    // RFC 7748 section 5 returns x_2 * z_2^(p-2). For p = 2^255 - 19, p-2 = 2^255 - 21. This fixed
    // square-and-multiply chain tests only public exponent bits and therefore executes identically for every value.
    auto result = one();
    for (auto bit = 255U; bit-- > 0U;) {
        result = square(result);
        if (bit >= 5U || bit == 3U || bit == 1U || bit == 0U) {
            result = multiply(result, value);
        }
    }
    return result;
}

void FieldElement::conditionalSwap(FieldElement &a, FieldElement &b, const uint64_t swap) noexcept {
    // RFC 7748 section 5: mask(swap) = 0 - swap and dummy = mask(swap) AND (a XOR b).
    const auto mask = uint64_t{0U} - (swap & 1U);
    for (auto i = std::size_t{0}; i < a._limbs.size(); ++i) {
        const auto difference = mask & (a._limbs[i] ^ b._limbs[i]);
        a._limbs[i] ^= difference;
        b._limbs[i] ^= difference;
    }
}

void FieldElement::secureErase() noexcept {
    mem::secureErase(std::span{_limbs});
}

auto FieldElement::one() noexcept -> FieldElement {
    auto result = FieldElement{};
    result._limbs[0] = 1U;
    return result;
}

auto FieldElement::fromBytes(const mem::ConstByteSpan bytes) noexcept -> FieldElement {
    auto result = FieldElement{};
    if (bytes.size() != 32U) {
        return result;
    }

    // RFC 7748 section 5, decodeUCoordinate: decode little-endian bits 0..254 and ignore bit 255. Normalization below
    // reduces the required non-canonical encodings 2^255 - 19 through 2^255 - 1 modulo p.
    for (auto limb = std::size_t{0}; limb < result._limbs.size(); ++limb) {
        const auto offset = limbOffset(limb);
        const auto width = limbBits(limb);
        auto value = uint64_t{0U};
        for (auto bit = std::size_t{0}; bit < width; ++bit) {
            const auto inputBit = offset + bit;
            const auto byte = bytes[inputBit / 8U].toUInt64();
            value |= ((byte >> (inputBit % 8U)) & 1U) << bit;
        }
        result._limbs[limb] = value;
    }
    result.normalize();
    return result;
}

auto FieldElement::toBytes() const noexcept -> mem::ByteArray<32U> {
    // RFC 7748 section 5, encodeUCoordinate: reduce modulo p and emit the minimal 32-byte little-endian value.
    auto canonical = *this;
    canonical.normalize();
    auto result = mem::ByteArray<32U>{};
    for (auto limb = std::size_t{0}; limb < canonical._limbs.size(); ++limb) {
        const auto offset = limbOffset(limb);
        const auto width = limbBits(limb);
        for (auto bit = std::size_t{0}; bit < width; ++bit) {
            const auto outputBit = offset + bit;
            const auto byteIndex = unit::ByteIndex::fromSizeT(outputBit / 8U);
            const auto current = result.get(byteIndex).toUInt8();
            const auto value = static_cast<uint8_t>(((canonical._limbs[limb] >> bit) & 1U) << (outputBit % 8U));
            result.set(byteIndex, mem::Byte{static_cast<uint8_t>(current | value)});
        }
    }
    return result;
}

void FieldElement::normalize() noexcept {
    // RFC 7748 section 4.1: propagate carries in the alternating 26/25-bit radix. The final carry represents a
    // multiple of 2^255 and folds into limb zero as carry * 19. Four fixed passes cover the largest multiplication
    // and small-constant bounds used here without making control flow depend on field values.
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

    // RFC 7748 section 5 requires reduction modulo p. Two branchless subtractions canonicalize the post-carry bound.
    subtractModulus();
    subtractModulus();
}

void FieldElement::subtractModulus() noexcept {
    // RFC 7748 section 5: subtract p in the internal radix, then select the difference only when no borrow occurred.
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

    // The candidate difference can contain scalar-derived ladder state; erase it before releasing storage.
    mem::secureErase(std::span{difference});
}

}
