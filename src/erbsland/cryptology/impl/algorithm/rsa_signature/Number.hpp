// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../mem/ByteSpan.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace erbsland::cryptology::impl::rsa_signature {

constexpr auto cWordBits = std::size_t{32U};
constexpr auto cMinimumModulusBits = std::size_t{2048U};
constexpr auto cMaximumModulusBits = std::size_t{8192U};
constexpr auto cMaximumWords = cMaximumModulusBits / cWordBits;
constexpr auto cMaximumExponentBytes = std::size_t{32U};

/// A bounded nonnegative integer for RSA arithmetic.
///
/// The active limbs are stored little-endian in fixed storage. Secret operations use fixed schedules based on the
/// represented width, and every instance can erase its complete capacity explicitly.
/// @tested{RsaSignatureTest SigningPrivateKeyTest X509CertificateBuilderTest}
class Number final {
public:
    /// Create a zero-width zero value.
    Number() = default;

    // defaults
    ~Number() = default;
    Number(const Number &) = default;
    Number(Number &&) noexcept = default;
    auto operator=(const Number &) -> Number & = default;
    auto operator=(Number &&) noexcept -> Number & = default;

public: // factories
    /// Create a fixed-width value from a small unsigned integer.
    [[nodiscard]] static auto fromValue(uint32_t value, std::size_t wordCount) noexcept -> Number;
    /// Convert a big-endian octet string to a fixed-width value.
    [[nodiscard]] static auto fromBigEndian(mem::ConstByteSpan bytes, std::size_t wordCount) -> Number;
    /// Generate a probable prime with the exact requested width.
    [[nodiscard]] static auto generatePrime(std::size_t bits) -> Number;

public: // access
    /// Return the active fixed width in 32-bit limbs.
    [[nodiscard]] auto wordCount() const noexcept -> std::size_t { return _wordCount; }
    /// Return one limb, or zero when the index is outside the active width.
    [[nodiscard]] auto word(std::size_t index) const noexcept -> uint32_t;
    /// Return whether the represented value is zero.
    [[nodiscard]] auto isZero() const noexcept -> bool;
    /// Return whether the represented value is one.
    [[nodiscard]] auto isOne() const noexcept -> bool;
    /// Return whether the represented value is odd.
    [[nodiscard]] auto isOdd() const noexcept -> bool;
    /// Return the exact represented bit length.
    [[nodiscard]] auto bitLength() const noexcept -> std::size_t;
    /// Compare this value with another equal-width value.
    [[nodiscard]] auto compare(const Number &other) const noexcept -> int;
    /// Compare the first `wordCount` limbs without content-dependent short circuiting.
    [[nodiscard]] auto isEqual(const Number &other, std::size_t wordCount) const noexcept -> bool;
    /// Convert this value to exactly `length` big-endian octets.
    [[nodiscard]] auto toBigEndian(std::size_t length) const -> mem::ByteBlock;
    /// Return this value padded to a larger represented width.
    [[nodiscard]] auto padded(std::size_t wordCount) const noexcept -> Number;

public: // basic arithmetic
    /// Subtract another value in place in the represented integer domain.
    void subtract(const Number &other) noexcept;
    /// Return this value minus another value in the represented integer domain.
    [[nodiscard]] auto subtracted(const Number &other) const noexcept -> Number;
    /// Return this positive value minus one.
    [[nodiscard]] auto subtractOne() const noexcept -> Number;
    /// Return this value shifted right by one bit.
    [[nodiscard]] auto shiftedRight() const noexcept -> Number;
    /// Return the remainder after division by a small value.
    [[nodiscard]] auto modulo(uint32_t divisor) const noexcept -> uint32_t;
    /// Multiply by a small value and add one, preserving a possible carry limb.
    [[nodiscard]] auto multipliedAndIncremented(uint32_t multiplier) const noexcept -> Number;
    /// Divide by a small value and retain the requested result width.
    [[nodiscard]] auto divided(uint32_t divisor, std::size_t resultWords) const noexcept -> Number;
    /// Add another value without modular reduction at the requested width.
    [[nodiscard]] auto added(const Number &other, std::size_t resultWords) const noexcept -> Number;
    /// Multiply two values exactly into the requested fixed width.
    [[nodiscard]] auto multiplied(const Number &other, std::size_t resultWords) const noexcept -> Number;
    /// Compute `(this + other) mod modulus` for reduced public operands.
    [[nodiscard]] auto addedModulo(const Number &other, const Number &modulus) const noexcept -> Number;
    /// Compute `(this * other) mod modulus` using public-input arithmetic.
    [[nodiscard]] auto multipliedModulo(const Number &other, const Number &modulus) const noexcept -> Number;

public: // fixed-schedule secret arithmetic
    /// Reduce this value modulo a secret modulus with a fixed bit schedule.
    [[nodiscard]] auto reducedSecret(const Number &modulus) const noexcept -> Number;
    /// Multiply reduced values modulo a secret modulus with a fixed bit schedule.
    [[nodiscard]] auto multipliedModuloSecret(const Number &other, const Number &modulus) const noexcept -> Number;
    /// Subtract reduced values modulo a secret modulus with masked correction.
    [[nodiscard]] auto subtractedModuloSecret(const Number &other, const Number &modulus) const noexcept -> Number;
    /// Raise this base to an exponent with fixed Montgomery square-and-multiply-always.
    [[nodiscard]] auto poweredModuloSecret(const Number &exponent, const Number &oddModulus) const noexcept -> Number;

public: // RSA generation
    /// Apply trial division and bounded Miller-Rabin rounds.
    [[nodiscard]] auto isProbablePrime() const -> bool;
    /// Calculate the inverse of 65537 modulo this even RSA totient factor.
    [[nodiscard]] auto inversePublicExponent() const noexcept -> Number;
    /// Test whether another candidate prime has the required separation.
    [[nodiscard]] auto isSeparatedFrom(const Number &other, std::size_t primeBits) const noexcept -> bool;

public: // security
    /// Erase all fixed storage and reset the active width.
    void secureErase() noexcept;

private:
    /// Select between two values with an all-zero or all-one mask.
    [[nodiscard]] static auto select(const Number &first, const Number &second, uint32_t mask) noexcept -> Number;
    /// Subtract without precondition checks and return the final borrow.
    [[nodiscard]] static auto subtractRaw(const Number &left, const Number &right, uint32_t &borrow) noexcept -> Number;
    /// Conditionally subtract one modulus using masked selection.
    [[nodiscard]] auto reducedOnce(const Number &modulus, uint32_t carry = 0U) const noexcept -> Number;
    /// Add reduced values modulo a secret modulus with masked reduction.
    [[nodiscard]] auto addedModuloSecret(const Number &other, const Number &modulus) const noexcept -> Number;
    /// Return the Montgomery reduction factor for an odd modulus.
    [[nodiscard]] auto montgomeryFactor() const noexcept -> uint32_t;
    /// Multiply two Montgomery-domain values.
    [[nodiscard]] static auto montgomeryMultiply(
        const Number &left, const Number &right, const Number &modulus, uint32_t factor) noexcept -> Number;
    /// Compute R squared modulo this modulus.
    [[nodiscard]] auto montgomeryR2() const noexcept -> Number;
    /// Select one random Miller-Rabin base.
    [[nodiscard]] auto randomMillerRabinBase() const -> Number;

private:
    std::array<uint32_t, cMaximumWords> _words{}; ///< Little-endian base-2^32 limbs.
    std::size_t _wordCount{};                     ///< Active fixed-width limbs.
};

}
