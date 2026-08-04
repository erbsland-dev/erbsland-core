// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteSpan.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace erbsland::cryptology::impl::x25519 {

/// One RFC 7748 section 4.1 field element modulo `2^255 - 19`.
/// @tested{X25519FullTest}
class FieldElement final {
public:
    /// Create zero.
    FieldElement() noexcept = default;
    /// Erase all limbs.
    ~FieldElement();

    // defaults
    FieldElement(const FieldElement &) = default;
    FieldElement(FieldElement &&) noexcept = default;
    auto operator=(const FieldElement &) -> FieldElement & = default;
    auto operator=(FieldElement &&) noexcept -> FieldElement & = default;

public:
    /// Add two field elements.
    [[nodiscard]] static auto add(const FieldElement &a, const FieldElement &b) noexcept -> FieldElement;
    /// Subtract two field elements.
    [[nodiscard]] static auto subtract(const FieldElement &a, const FieldElement &b) noexcept -> FieldElement;
    /// Multiply two field elements.
    [[nodiscard]] static auto multiply(const FieldElement &a, const FieldElement &b) noexcept -> FieldElement;
    /// Square one field element.
    [[nodiscard]] static auto square(const FieldElement &value) noexcept -> FieldElement;
    /// Multiply one field element by a small constant.
    [[nodiscard]] static auto multiplySmall(const FieldElement &value, uint64_t factor) noexcept -> FieldElement;
    /// Calculate the RFC 7748 section 5 multiplicative inverse with the fixed `p - 2` exponent.
    [[nodiscard]] static auto invert(const FieldElement &value) noexcept -> FieldElement;
    /// Perform the branchless `cswap` operation from RFC 7748 section 5.
    static void conditionalSwap(FieldElement &a, FieldElement &b, uint64_t swap) noexcept;
    /// Securely erase all limbs.
    void secureErase() noexcept;

public: // factories and conversion
    /// Create one.
    [[nodiscard]] static auto one() noexcept -> FieldElement;
    /// Decode and reduce one RFC 7748 section 5 little-endian coordinate, ignoring its top bit.
    [[nodiscard]] static auto fromBytes(mem::ConstByteSpan bytes) noexcept -> FieldElement;
    /// Encode the canonical little-endian coordinate as specified by RFC 7748 section 5.
    [[nodiscard]] auto toBytes() const noexcept -> mem::ByteArray<32U>;

private:
    /// Propagate alternating-radix carries and canonicalize the element.
    void normalize() noexcept;
    /// Conditionally subtract the field modulus once.
    void subtractModulus() noexcept;
    /// Return a limb's radix width.
    [[nodiscard]] static constexpr auto limbBits(std::size_t index) noexcept -> std::size_t {
        return (index % 2U) == 0U ? 26U : 25U;
    }
    /// Return a limb's bit offset in the encoded coordinate.
    [[nodiscard]] static constexpr auto limbOffset(std::size_t index) noexcept -> std::size_t {
        return (index / 2U) * 51U + ((index % 2U) == 0U ? 0U : 26U);
    }

private:
    std::array<uint64_t, 10U> _limbs{}; ///< Alternating 26/25-bit little-endian limbs.
};

}
