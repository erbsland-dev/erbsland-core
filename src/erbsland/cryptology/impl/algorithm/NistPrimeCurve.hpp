// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../mem/SecureErase.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace erbsland::cryptology::impl {

/// Bounded arithmetic and point handling for the NIST P-256 and P-384 prime curves.
///
/// Curve parameters and representations follow NIST SP 800-186 sections 3.2.1.3 and 3.2.1.4. Coordinates and scalars
/// use at most twelve little-endian 32-bit limbs. Public-point conversion follows RFC 5480 section 2.2 and SEC 1
/// sections 2.3.3 and 2.3.4. The public scalar multiplication routine is deliberately variable-time and must never be
/// used with an ECDHE private scalar or any other secret value.
///
/// ECDSA verification creates no secret state: every scalar and point processed by the public-input path comes from a
/// public key, message digest, signature, or fixed curve parameter. The separately named signing operations implement
/// a fixed 256-bit schedule for P-256 secret inputs and require their callers to erase all returned scratch values.
/// @tested{EcdsaSignatureTest EcdsaSignatureFullTest SigningPrivateKeyTest}
class NistPrimeCurve final {
public:
    /// Supported NIST prime curves.
    enum class Name : uint8_t {
        P256, ///< NIST P-256, also named secp256r1 or prime256v1.
        P384, ///< NIST P-384, also named secp384r1.
    };

    /// One bounded nonnegative integer.
    struct Number final {
        std::array<uint32_t, 12U> words{}; ///< Little-endian base-2^32 limbs.

        /// Erase every fixed-width limb.
        void secureErase() noexcept { mem::secureErase(std::span{words}); }
    };

    /// One Jacobian curve point; `z == 0` represents the identity.
    struct Point final {
        Number x; ///< Jacobian X coordinate.
        Number y; ///< Jacobian Y coordinate.
        Number z; ///< Jacobian Z coordinate, or zero for the identity.

        /// Erase all projective coordinates.
        void secureErase() noexcept {
            x.secureErase();
            y.secureErase();
            z.secureErase();
        }
    };

public:
    /// Select one fixed NIST curve.
    explicit NistPrimeCurve(Name name) noexcept;

    // defaults
    ~NistPrimeCurve() = default;
    NistPrimeCurve(const NistPrimeCurve &) = default;
    NistPrimeCurve(NistPrimeCurve &&) noexcept = default;
    auto operator=(const NistPrimeCurve &) -> NistPrimeCurve & = default;
    auto operator=(NistPrimeCurve &&) noexcept -> NistPrimeCurve & = default;

public: // accessors
    /// Get the selected curve.
    [[nodiscard]] auto name() const noexcept -> Name { return _name; }
    /// Get the fixed coordinate/scalar width in bytes.
    [[nodiscard]] auto byteLength() const noexcept -> std::size_t;
    /// Get the prime subgroup order.
    [[nodiscard]] auto order() const noexcept -> const Number &;
    /// Get the standard base point in Jacobian form.
    [[nodiscard]] auto basePoint() const noexcept -> Point;

public: // number conversion and tests
    /// Decode a big-endian nonnegative integer no wider than the selected curve.
    [[nodiscard]] auto numberFromBigEndian(mem::ConstByteSpan bytes) const noexcept -> std::optional<Number>;
    /// Compare two represented integers.
    [[nodiscard]] auto compare(const Number &left, const Number &right) const noexcept -> int;
    /// Test whether an integer is zero.
    [[nodiscard]] auto isZero(const Number &value) const noexcept -> bool;
    /// Test one integer bit.
    [[nodiscard]] auto bit(const Number &value, std::size_t index) const noexcept -> bool;
    /// Encode one number at the selected curve width using big-endian octets.
    [[nodiscard]] auto numberToBigEndian(const Number &value) const noexcept -> mem::ByteBlock;

public: // point conversion and tests
    /// Decode and fully validate one compressed or uncompressed RFC 5480 public point.
    [[nodiscard]] auto pointFromBytes(mem::ConstByteSpan bytes) const noexcept -> std::optional<Point>;
    /// Test whether a point is the identity.
    [[nodiscard]] auto isIdentity(const Point &point) const noexcept -> bool;
    /// Convert a non-identity point to affine form.
    [[nodiscard]] auto toAffine(const Point &point) const noexcept -> std::optional<Point>;

public: // ECDSA public-input verification arithmetic
    /// Reduce an integer modulo the curve order.
    [[nodiscard]] auto reduceOrder(const Number &value) const noexcept -> Number;
    /// Multiply two scalars modulo the curve order.
    [[nodiscard]] auto multiplyOrder(const Number &left, const Number &right) const noexcept -> Number;
    /// Invert a nonzero scalar modulo the curve order.
    [[nodiscard]] auto invertOrder(const Number &value) const noexcept -> Number;
    /// Add two public points.
    [[nodiscard]] auto addPublic(const Point &left, const Point &right) const noexcept -> Point;
    /// Multiply a point by a public scalar using a variable-time double-and-add schedule.
    [[nodiscard]] auto multiplyPublic(const Point &point, const Number &scalar) const noexcept -> Point;

public: // ECDSA secret-input signing arithmetic
    /// Reduce one curve-width secret scalar modulo the curve order with masked selection.
    [[nodiscard]] auto reduceOrderSecret(const Number &value) const noexcept -> Number;
    /// Add two reduced secret scalars modulo the curve order with a fixed operation schedule.
    [[nodiscard]] auto addOrderSecret(const Number &left, const Number &right) const noexcept -> Number;
    /// Multiply two reduced secret scalars modulo the curve order with a fixed operation schedule.
    [[nodiscard]] auto multiplyOrderSecret(const Number &left, const Number &right) const noexcept -> Number;
    /// Invert one nonzero secret scalar modulo the curve order with a fixed exponent schedule.
    [[nodiscard]] auto invertOrderSecret(const Number &value) const noexcept -> Number;
    /// Multiply the standard base point by one secret scalar using double-and-add-always and masked selection.
    [[nodiscard]] auto multiplyBaseSecret(const Number &scalar) const noexcept -> Point;
    /// Convert a nonidentity secret-derived point to affine form with a fixed inversion schedule.
    [[nodiscard]] auto toAffineSecret(const Point &point) const noexcept -> Point;
    /// Encode one affine point in uncompressed SEC 1 form.
    [[nodiscard]] auto pointToUncompressed(const Point &point) const noexcept -> mem::ByteBlock;
    /// Test whether a secret scalar is zero using a fixed limb schedule.
    [[nodiscard]] auto isZeroSecret(const Number &value) const noexcept -> bool;
    /// Erase one bounded number.
    static void secureErase(Number &value) noexcept;
    /// Erase all coordinates in one point.
    static void secureErase(Point &point) noexcept;

private: // curve parameters
    /// Get the prime field modulus.
    [[nodiscard]] auto fieldModulus() const noexcept -> const Number &;
    /// Get the curve coefficient `b`.
    [[nodiscard]] auto coefficientB() const noexcept -> const Number &;

private: // bounded integer arithmetic
    /// Add two reduced integers modulo `modulus` without overflowing the fixed representation.
    [[nodiscard]] auto addModulo(const Number &left, const Number &right, const Number &modulus) const noexcept
        -> Number;
    /// Subtract two reduced integers modulo `modulus`.
    [[nodiscard]] auto subtractModulo(const Number &left, const Number &right, const Number &modulus) const noexcept
        -> Number;
    /// Multiply two reduced integers modulo `modulus` using a bounded public-input schedule.
    [[nodiscard]] auto multiplyModulo(const Number &left, const Number &right, const Number &modulus) const noexcept
        -> Number;
    /// Multiply two Montgomery-domain integers and divide the product by the radix.
    [[nodiscard]] auto montgomeryMultiply(
        const Number &left, const Number &right, const Number &modulus, uint32_t factor) const noexcept -> Number;
    /// Get the precomputed squared Montgomery radix for a supported modulus.
    [[nodiscard]] auto montgomeryR2(const Number &modulus) const noexcept -> const Number &;
    /// Get `-modulus^-1 mod 2^32` for Montgomery reduction.
    [[nodiscard]] auto montgomeryFactor(const Number &modulus) const noexcept -> uint32_t;
    /// Raise `value` to a public exponent modulo `modulus`.
    [[nodiscard]] auto powerModulo(const Number &value, const Number &exponent, const Number &modulus) const noexcept
        -> Number;
    /// Subtract a small value from an integer.
    [[nodiscard]] auto subtractSmall(Number value, uint32_t amount) const noexcept -> Number;
    /// Add a small value to an integer.
    [[nodiscard]] auto addSmall(Number value, uint32_t amount) const noexcept -> Number;
    /// Shift an integer right by one bit.
    [[nodiscard]] auto shiftRight(Number value) const noexcept -> Number;
    /// Subtract `right` from `left`, requiring `left >= right`.
    [[nodiscard]] auto subtract(const Number &left, const Number &right) const noexcept -> Number;
    /// Add `left` and `right`, requiring the represented sum not to overflow.
    [[nodiscard]] auto add(const Number &left, const Number &right) const noexcept -> Number;
    /// Select `second` when the mask is all ones, otherwise select `first`.
    [[nodiscard]] auto selectNumber(const Number &first, const Number &second, uint32_t secondMask) const noexcept
        -> Number;
    /// Return all ones when two represented numbers are equal, otherwise zero.
    [[nodiscard]] auto equalMask(const Number &left, const Number &right) const noexcept -> uint32_t;
    /// Return all ones when the represented number is zero, otherwise zero.
    [[nodiscard]] auto zeroMask(const Number &value) const noexcept -> uint32_t;
    /// Conditionally subtract one modulus using masked selection.
    [[nodiscard]] auto reduceOnceSecret(const Number &value, const Number &modulus, uint32_t carry = 0U) const noexcept
        -> Number;

private: // field arithmetic
    /// Add two field elements.
    [[nodiscard]] auto fieldAdd(const Number &left, const Number &right) const noexcept -> Number;
    /// Subtract two field elements.
    [[nodiscard]] auto fieldSubtract(const Number &left, const Number &right) const noexcept -> Number;
    /// Multiply two field elements.
    [[nodiscard]] auto fieldMultiply(const Number &left, const Number &right) const noexcept -> Number;
    /// Square one field element.
    [[nodiscard]] auto fieldSquare(const Number &value) const noexcept -> Number;
    /// Multiply a field element by a small public constant.
    [[nodiscard]] auto fieldMultiplySmall(const Number &value, uint32_t factor) const noexcept -> Number;
    /// Invert a nonzero field element.
    [[nodiscard]] auto fieldInvert(const Number &value) const noexcept -> Number;
    /// Calculate a square root for the selected prime, returning no value for a nonsquare.
    [[nodiscard]] auto fieldSquareRoot(const Number &value) const noexcept -> std::optional<Number>;

private: // point arithmetic
    /// Create the identity point.
    [[nodiscard]] static auto identity() noexcept -> Point;
    /// Create an affine point with `z = 1`.
    [[nodiscard]] static auto affine(Number x, Number y) noexcept -> Point;
    /// Test the affine curve equation `y^2 = x^3 - 3x + b`.
    [[nodiscard]] auto isOnCurve(const Point &point) const noexcept -> bool;
    /// Double one Jacobian point.
    [[nodiscard]] auto doublePoint(const Point &point) const noexcept -> Point;
    /// Add an affine right-hand point to a Jacobian left-hand point.
    [[nodiscard]] auto addMixed(const Point &left, const Point &rightAffine) const noexcept -> Point;
    /// Double one point without secret-dependent control flow.
    [[nodiscard]] auto doubleSecret(const Point &point) const noexcept -> Point;
    /// Add an affine point without secret-dependent control flow.
    [[nodiscard]] auto addMixedSecret(const Point &left, const Point &rightAffine) const noexcept -> Point;
    /// Select one point using one all-zero or all-one mask.
    [[nodiscard]] auto selectPoint(const Point &first, const Point &second, uint32_t secondMask) const noexcept
        -> Point;

private:
    static const Number cOne;         ///< Multiplicative identity.
    static const Number cP256Modulus; ///< P-256 field modulus.
    static const Number cP256Order;   ///< P-256 subgroup order.
    static const Number cP256B;       ///< P-256 coefficient b.
    static const Number cP256Gx;      ///< P-256 base-point x-coordinate.
    static const Number cP256Gy;      ///< P-256 base-point y-coordinate.
    static const Number cP256FieldR2; ///< P-256 field Montgomery radix squared modulo p.
    static const Number cP256OrderR2; ///< P-256 scalar Montgomery radix squared modulo n.
    static const Number cP384Modulus; ///< P-384 field modulus.
    static const Number cP384Order;   ///< P-384 subgroup order.
    static const Number cP384B;       ///< P-384 coefficient b.
    static const Number cP384Gx;      ///< P-384 base-point x-coordinate.
    static const Number cP384Gy;      ///< P-384 base-point y-coordinate.
    static const Number cP384FieldR2; ///< P-384 field Montgomery radix squared modulo p.
    static const Number cP384OrderR2; ///< P-384 scalar Montgomery radix squared modulo n.

private:
    Name _name; ///< Selected fixed curve.
};

}
