// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerConversion.hpp"
#include "IntegerMath.hpp"
#include "IntegerTraits.hpp"

#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <concepts>
#include <exception>
#include <limits>
#include <type_traits>

namespace erbsland::math {

/// A same-width integer value represented as sign and unsigned magnitude.
/// @seedoc{/reference/math/mathematics}
/// @tparam tValue The native integer type whose unsigned counterpart is used for the magnitude.
/// @tested{SignedMagnitudeTest}
template <NativeInteger tValue>
class SignedMagnitude final {
public:
    /// The native integer type this signed-magnitude value is based on.
    using Value = NativeIntegerOfT<tValue>;
    /// The unsigned type used to store the magnitude.
    using Unsigned = std::make_unsigned_t<Value>;

public:
    /// Create a zero value.
    constexpr SignedMagnitude() noexcept = default;
    /// Create a value from a sign and magnitude.
    ///
    /// A zero magnitude is always normalized to a positive value.
    /// @param negative Set to `true` to create a negative value.
    /// @param magnitude The absolute magnitude.
    constexpr SignedMagnitude(bool negative, Unsigned magnitude) noexcept :
        _negative{magnitude != 0U && negative}, _magnitude{magnitude} {}
    /// Create a signed-magnitude value from a same-width integer operand.
    ///
    /// Signed negative input uses `toUnsignedAbsolute()` so the signed minimum value is converted safely.
    /// Unsigned input is treated as a positive magnitude.
    /// @tparam tSource The source integer type. It must have the same byte width as `Value`.
    /// @param value The integer operand to represent.
    template <AnyIntegerType tSource>
        requires(sizeof(NativeIntegerOfT<tSource>) == sizeof(Value))
    explicit constexpr SignedMagnitude(tSource value) noexcept {
        *this = fromValue(value);
    }

public:
    /// Create a value from a sign and magnitude.
    ///
    /// This factory mirrors the sign/magnitude constructor and can make call sites easier to read when the arguments
    /// are computed expressions.
    /// @param negative Set to `true` to create a negative value.
    /// @param magnitude The absolute magnitude.
    /// @return The normalized signed-magnitude value.
    [[nodiscard]] static constexpr auto fromSignAndMagnitude(bool negative, Unsigned magnitude) noexcept
        -> SignedMagnitude {
        return {negative, magnitude};
    }
    /// Create a signed-magnitude value from a same-width integer operand.
    /// @tparam tSource The source integer type. It must have the same byte width as `Value`.
    /// @param value The integer operand to represent.
    /// @return The value represented as sign and unsigned magnitude.
    template <AnyIntegerType tSource>
        requires(sizeof(NativeIntegerOfT<tSource>) == sizeof(Value))
    [[nodiscard]] static constexpr auto fromValue(tSource value) noexcept -> SignedMagnitude {
        const auto nativeValue = toNativeInteger(value);
        if constexpr (std::signed_integral<NativeIntegerOfT<tSource>>) {
            if (nativeValue < 0) {
                return {true, static_cast<Unsigned>(toUnsignedAbsolute(nativeValue))};
            }
        }
        return {false, static_cast<Unsigned>(nativeValue)};
    }

public: // operators
    /// Compare two signed-magnitude values mathematically.
    ///
    /// Negative magnitudes are ordered in reverse magnitude order, so `-2 < -1 < 0 < 1 < 2`.
    /// @param other The other value to compare with this value.
    /// @return The ordering of this value compared with `other`.
    [[nodiscard]] constexpr auto operator<=>(const SignedMagnitude &other) const noexcept -> std::strong_ordering {
        if (_negative != other._negative) {
            return _negative ? std::strong_ordering::less : std::strong_ordering::greater;
        }
        if (_magnitude == other._magnitude) {
            return std::strong_ordering::equal;
        }
        const auto thisMagnitudeIsSmaller = _magnitude < other._magnitude;
        if (_negative) {
            return thisMagnitudeIsSmaller ? std::strong_ordering::greater : std::strong_ordering::less;
        }
        return thisMagnitudeIsSmaller ? std::strong_ordering::less : std::strong_ordering::greater;
    }
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FROM_SPACESHIP(const SignedMagnitude &other, other);

public: // accessors
    /// Test if this value is negative.
    /// @return `true` if this value is smaller than zero.
    [[nodiscard]] constexpr auto isNegative() const noexcept -> bool { return _negative; }
    /// Test if this value is zero.
    /// @return `true` if the magnitude is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _magnitude == 0U; }
    /// Get the unsigned magnitude.
    /// @return The absolute magnitude of this value.
    [[nodiscard]] constexpr auto magnitude() const noexcept -> Unsigned { return _magnitude; }

public:
    /// Return this value with the sign changed.
    ///
    /// Zero remains positive after negation.
    /// @return The negated signed-magnitude value.
    [[nodiscard]] constexpr auto negated() const noexcept -> SignedMagnitude { return {!_negative, _magnitude}; }
    /// Test if this value is outside a bounded native result range.
    ///
    /// The bounds must form a valid range. They are converted into the same sign/magnitude representation before
    /// comparison, so signed and unsigned result domains share the same code path.
    /// @param minimum The smallest allowed result value.
    /// @param maximum The largest allowed result value.
    /// @return `true` if this value is outside `[minimum, maximum]`.
    [[nodiscard]] constexpr auto wouldSaturate(Value minimum, Value maximum) const noexcept -> bool {
        return (*this <=> fromValue(minimum)) == std::strong_ordering::less ||
            (*this <=> fromValue(maximum)) == std::strong_ordering::greater;
    }
    /// Convert this value into a bounded native result type.
    ///
    /// The conversion clamps to the explicit bounds before converting back into `Value`. This keeps signed-minimum
    /// magnitudes and unsigned magnitudes above a signed maximum well-defined in constant expressions.
    /// @param minimum The smallest allowed result value.
    /// @param maximum The largest allowed result value.
    /// @return The native value clamped to `[minimum, maximum]`.
    [[nodiscard]] constexpr auto toSaturatingValue(Value minimum, Value maximum) const noexcept -> Value {
        if ((*this <=> fromValue(minimum)) == std::strong_ordering::less) {
            return minimum;
        }
        if ((*this <=> fromValue(maximum)) == std::strong_ordering::greater) {
            return maximum;
        }
        if (_magnitude == 0U) {
            return Value{0};
        }
        if (_negative) {
            if constexpr (std::signed_integral<Value>) {
                constexpr auto cMinimumMagnitude = Unsigned{1} << std::numeric_limits<Value>::digits;
                if (_magnitude == cMinimumMagnitude) {
                    return std::numeric_limits<Value>::min();
                }
                return static_cast<Value>(-static_cast<Value>(_magnitude));
            } else {
                return minimum;
            }
        }
        return static_cast<Value>(_magnitude);
    }
    /// Add another signed-magnitude value and clamp the result into a bounded native result type.
    ///
    /// Equal-sign magnitude overflow means the mathematical result is outside the representable sign/magnitude domain,
    /// so the result immediately clamps to the bound for that sign.
    /// @param other The value to add to this value.
    /// @param minimum The smallest allowed result value.
    /// @param maximum The largest allowed result value.
    /// @return The mathematical sum clamped to `[minimum, maximum]`.
    [[nodiscard]] constexpr auto saturatingAddBounded(
        const SignedMagnitude &other, Value minimum, Value maximum) const noexcept -> Value {
        auto result = SignedMagnitude{};
        if (addMagnitude(other, result)) {
            return _negative ? minimum : maximum;
        }
        return result.toSaturatingValue(minimum, maximum);
    }
    /// Test if adding another signed-magnitude value would clamp to a bounded native result type.
    /// @param other The value to add to this value.
    /// @param minimum The smallest allowed result value.
    /// @param maximum The largest allowed result value.
    /// @return `true` if the mathematical sum is outside `[minimum, maximum]`.
    [[nodiscard]] constexpr auto wouldAddBoundedSaturate(
        const SignedMagnitude &other, Value minimum, Value maximum) const noexcept -> bool {
        auto result = SignedMagnitude{};
        if (addMagnitude(other, result)) {
            return true;
        }
        return result.wouldSaturate(minimum, maximum);
    }
    /// Multiply by another signed-magnitude value and clamp the result into a bounded native result type.
    /// @param other The value to multiply this value with.
    /// @param minimum The smallest allowed result value.
    /// @param maximum The largest allowed result value.
    /// @return The mathematical product clamped to `[minimum, maximum]`.
    [[nodiscard]] constexpr auto saturatingMultiplyBounded(
        const SignedMagnitude &other, Value minimum, Value maximum) const noexcept -> Value {
        auto result = SignedMagnitude{};
        if (multiplyMagnitude(other, result)) {
            return (_negative != other._negative) ? minimum : maximum;
        }
        return result.toSaturatingValue(minimum, maximum);
    }
    /// Test if multiplying another signed-magnitude value would clamp to a bounded native result type.
    /// @param other The value to multiply this value with.
    /// @param minimum The smallest allowed result value.
    /// @param maximum The largest allowed result value.
    /// @return `true` if the mathematical product is outside `[minimum, maximum]`.
    [[nodiscard]] constexpr auto wouldMultiplyBoundedSaturate(
        const SignedMagnitude &other, Value minimum, Value maximum) const noexcept -> bool {
        auto result = SignedMagnitude{};
        if (multiplyMagnitude(other, result)) {
            return true;
        }
        return result.wouldSaturate(minimum, maximum);
    }
    /// Divide by another signed-magnitude value and clamp the result into a bounded native result type.
    /// @param other The divisor.
    /// @param minimum The smallest allowed result value.
    /// @param maximum The largest allowed result value.
    /// @return The mathematical quotient clamped to `[minimum, maximum]`.
    [[nodiscard]] constexpr auto saturatingDivideBounded(
        const SignedMagnitude &other, Value minimum, Value maximum) const noexcept -> Value {
        return dividedMagnitude(other).toSaturatingValue(minimum, maximum);
    }
    /// Test if dividing by another signed-magnitude value would clamp to a bounded native result type.
    /// @param other The divisor.
    /// @param minimum The smallest allowed result value.
    /// @param maximum The largest allowed result value.
    /// @return `true` if the mathematical quotient is outside `[minimum, maximum]`.
    [[nodiscard]] constexpr auto wouldDivideBoundedSaturate(
        const SignedMagnitude &other, Value minimum, Value maximum) const noexcept -> bool {
        return dividedMagnitude(other).wouldSaturate(minimum, maximum);
    }
    /// Calculate the modulo with another signed-magnitude value and clamp the result into a bounded native result type.
    /// @param other The divisor.
    /// @param minimum The smallest allowed result value.
    /// @param maximum The largest allowed result value.
    /// @return The mathematical remainder clamped to `[minimum, maximum]`.
    [[nodiscard]] constexpr auto saturatingModuloBounded(
        const SignedMagnitude &other, Value minimum, Value maximum) const noexcept -> Value {
        return moduloMagnitude(other).toSaturatingValue(minimum, maximum);
    }
    /// Test if modulo with another signed-magnitude value would clamp to a bounded native result type.
    /// @param other The divisor.
    /// @param minimum The smallest allowed result value.
    /// @param maximum The largest allowed result value.
    /// @return `true` if the mathematical remainder is outside `[minimum, maximum]`.
    [[nodiscard]] constexpr auto wouldModuloBoundedSaturate(
        const SignedMagnitude &other, Value minimum, Value maximum) const noexcept -> bool {
        return moduloMagnitude(other).wouldSaturate(minimum, maximum);
    }

private:
    /// Add two values into `result` and return `true` if the unsigned magnitude overflowed.
    [[nodiscard]] constexpr auto addMagnitude(const SignedMagnitude &other, SignedMagnitude &result) const noexcept
        -> bool {
        if (_negative == other._negative) {
            if (_magnitude > (std::numeric_limits<Unsigned>::max() - other._magnitude)) {
                return true;
            }
            result = SignedMagnitude{_negative, static_cast<Unsigned>(_magnitude + other._magnitude)};
            return false;
        }
        if (_magnitude >= other._magnitude) {
            result = SignedMagnitude{_negative, static_cast<Unsigned>(_magnitude - other._magnitude)};
            return false;
        }
        result = SignedMagnitude{other._negative, static_cast<Unsigned>(other._magnitude - _magnitude)};
        return false;
    }
    /// Multiply two values into `result` and return `true` if the unsigned magnitude overflowed.
    [[nodiscard]] constexpr auto multiplyMagnitude(const SignedMagnitude &other, SignedMagnitude &result) const noexcept
        -> bool {
        if (_magnitude == Unsigned{0U} || other._magnitude == Unsigned{0U}) {
            result = SignedMagnitude{};
            return false;
        }
        if (_magnitude > std::numeric_limits<Unsigned>::max() / other._magnitude) {
            return true;
        }
        result = SignedMagnitude{_negative != other._negative, static_cast<Unsigned>(_magnitude * other._magnitude)};
        return false;
    }
    /// Return the signed-magnitude quotient.
    [[nodiscard]] constexpr auto dividedMagnitude(const SignedMagnitude &other) const noexcept -> SignedMagnitude {
        if (other._magnitude == Unsigned{0U}) {
            std::terminate();
        }
        return SignedMagnitude{_negative != other._negative, static_cast<Unsigned>(_magnitude / other._magnitude)};
    }
    /// Return the signed-magnitude remainder.
    [[nodiscard]] constexpr auto moduloMagnitude(const SignedMagnitude &other) const noexcept -> SignedMagnitude {
        if (other._magnitude == Unsigned{0U}) {
            std::terminate();
        }
        return SignedMagnitude{_negative, static_cast<Unsigned>(_magnitude % other._magnitude)};
    }

private:
    bool _negative{};      ///< `true` if the represented value is negative.
    Unsigned _magnitude{}; ///< The absolute magnitude.
};

}
