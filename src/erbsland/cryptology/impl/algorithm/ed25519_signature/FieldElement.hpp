// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteSpan.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace erbsland::cryptology::impl::ed25519_signature {

/// One field element modulo `2^255 - 19`, using the RFC 7748 section 4.1 radix.
/// @tested{Ed25519SignatureTest Ed25519SignatureFullTest}
class FieldElement final {
public:
    /// Create zero.
    FieldElement() noexcept = default;

    // defaults
    ~FieldElement() = default;
    FieldElement(const FieldElement &) = default;
    FieldElement(FieldElement &&) noexcept = default;
    auto operator=(const FieldElement &) -> FieldElement & = default;
    auto operator=(FieldElement &&) noexcept -> FieldElement & = default;

public: // arithmetic
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
    /// Raise one field element to `(p-5)/8 = 2^252-3` with a fixed public exponent.
    [[nodiscard]] static auto powerPMinus5Over8(const FieldElement &value) noexcept -> FieldElement;
    /// Raise one field element to `p-2` with a fixed public exponent.
    [[nodiscard]] static auto invert(const FieldElement &value) noexcept -> FieldElement;
    /// Select `second` when `selectSecond` is one, otherwise select `first`, without secret-dependent branches.
    [[nodiscard]] static auto select(
        const FieldElement &first, const FieldElement &second, uint64_t selectSecond) noexcept -> FieldElement;
    /// Erase the represented field value.
    void secureErase() noexcept;

public: // comparison and conversion
    /// Test two canonical field values for equality.
    [[nodiscard]] static auto equal(const FieldElement &a, const FieldElement &b) noexcept -> bool;
    /// Test whether a field value is zero.
    [[nodiscard]] auto isZero() const noexcept -> bool;
    /// Test the least-significant bit of a canonical field value.
    [[nodiscard]] auto isNegative() const noexcept -> bool;
    /// Create zero.
    [[nodiscard]] static auto zero() noexcept -> FieldElement;
    /// Create one.
    [[nodiscard]] static auto one() noexcept -> FieldElement;
    /// Decode one canonical RFC 8032 little-endian field element.
    [[nodiscard]] static auto fromCanonicalBytes(mem::ConstByteSpan bytes) noexcept -> std::optional<FieldElement>;
    /// Decode a known canonical RFC constant.
    [[nodiscard]] static auto fromConstant(const std::array<uint8_t, 32U> &bytes) noexcept -> FieldElement;
    /// Encode one canonical RFC 8032 little-endian field element.
    [[nodiscard]] auto toBytes() const noexcept -> mem::ByteArray<32U>;

private:
    /// Propagate alternating-radix carries and canonicalize the element.
    void normalize() noexcept;
    /// Conditionally subtract the field modulus once.
    void subtractModulus() noexcept;
    /// Return one limb's radix width.
    [[nodiscard]] static constexpr auto limbBits(std::size_t index) noexcept -> std::size_t {
        return (index % 2U) == 0U ? 26U : 25U;
    }
    /// Return one limb's bit offset in the encoded value.
    [[nodiscard]] static constexpr auto limbOffset(std::size_t index) noexcept -> std::size_t {
        return (index / 2U) * 51U + ((index % 2U) == 0U ? 0U : 26U);
    }

private:
    std::array<uint64_t, 10U> _limbs{}; ///< Alternating 26/25-bit little-endian limbs.
};

}
