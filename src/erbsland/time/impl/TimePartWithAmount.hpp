// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimePart.hpp"

#include "../../err/ThrowHelper.hpp"

#include <compare>

namespace erbsland::time::impl {

/// Extension for `TimePart` that links a concrete time part to an amount type.
///
/// Amount zero maps to the raw minimum value of the part. Arithmetic with amounts is performed in amount space and
/// clamped back into the supported part range.
/// @tested{TimeCoreTest}
template <
    typename tDerived,
    typename tAmount,
    typename tValue,
    tValue tMinimum,
    tValue tMaximum,
    tValue tDefault = tMinimum>
class TimePartWithAmount : public TimePart<tDerived, tValue, tMinimum, tMaximum, tDefault> {
    using Base = TimePart<tDerived, tValue, tMinimum, tMaximum, tDefault>;

public:
    using Amount = tAmount;
    using Value = typename Base::Value;
    using SaturatingValue = typename Base::SaturatingValue;

public: // importing the constructors from the base class.
    using Base::Base;

public: // operators
    using Base::operator+;
    using Base::operator+=;
    using Base::operator-;
    using Base::operator-=;

    /// Compare this part with an amount.
    [[nodiscard]] auto operator<=>(Amount other) const noexcept -> std::strong_ordering { return toAmount() <=> other; }
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(Amount other, other);

    /// Compare an amount with this part.
    friend auto operator<=>(Amount left, const TimePartWithAmount &right) noexcept -> std::strong_ordering {
        return left <=> right.toAmount();
    }
    /// Test if an amount equals this part.
    friend auto operator==(Amount left, const TimePartWithAmount &right) noexcept -> bool {
        return left == right.toAmount();
    }
    /// Test if an amount differs from this part.
    friend auto operator!=(Amount left, const TimePartWithAmount &right) noexcept -> bool {
        return left != right.toAmount();
    }
    /// Test if an amount is less than this part.
    friend auto operator<(Amount left, const TimePartWithAmount &right) noexcept -> bool {
        return left < right.toAmount();
    }
    /// Test if an amount is less than or equal to this part.
    friend auto operator<=(Amount left, const TimePartWithAmount &right) noexcept -> bool {
        return left <= right.toAmount();
    }
    /// Test if an amount is greater than this part.
    friend auto operator>(Amount left, const TimePartWithAmount &right) noexcept -> bool {
        return left > right.toAmount();
    }
    /// Test if an amount is greater than or equal to this part.
    friend auto operator>=(Amount left, const TimePartWithAmount &right) noexcept -> bool {
        return left >= right.toAmount();
    }

    /// Return this value plus an amount, clamped to the supported range.
    [[nodiscard]] auto operator+(Amount amount) const noexcept -> tDerived { return added(amount); }
    /// Add an amount in place, clamped to the supported range.
    auto operator+=(Amount amount) noexcept -> tDerived & {
        add(amount);
        return static_cast<tDerived &>(*this);
    }
    /// Return this value minus an amount, clamped to the supported range.
    [[nodiscard]] auto operator-(Amount amount) const noexcept -> tDerived { return subtracted(amount); }
    /// Subtract an amount in place, clamped to the supported range.
    auto operator-=(Amount amount) noexcept -> tDerived & {
        subtract(amount);
        return static_cast<tDerived &>(*this);
    }

public: // arithmetic
    using Base::add;
    using Base::added;
    using Base::subtract;
    using Base::subtracted;

    /// Return this value plus an amount, clamped to the supported range.
    [[nodiscard]] auto added(Amount amount) const noexcept -> tDerived { return fromAmount(toAmount() + amount); }
    /// Add an amount in place, clamped to the supported range.
    void add(Amount amount) noexcept { *this = added(amount); }
    /// Return this value minus an amount, clamped to the supported range.
    [[nodiscard]] auto subtracted(Amount amount) const noexcept -> tDerived { return fromAmount(toAmount() - amount); }
    /// Subtract an amount in place, clamped to the supported range.
    void subtract(Amount amount) noexcept { *this = subtracted(amount); }

public: // conversion
    /// Convert this time part to its zero-based amount.
    [[nodiscard]] auto toAmount() const noexcept -> Amount { return Amount{this->toValue().subtracted(tMinimum)}; }
    /// Create a time part from a zero-based amount, clamping to the supported range.
    [[nodiscard]] static auto fromAmount(Amount amount) noexcept -> tDerived {
        return tDerived{amount.toValue().added(tMinimum)};
    }
    /// Create a time part from a zero-based amount or throw if it is outside the supported range.
    /// @throws err::OutOfRangeError if the amount is outside the supported range.
    [[nodiscard]] static auto fromAmountOrThrow(Amount amount) -> tDerived {
        if (amount < Amount{} || amount > maximumAmount()) {
            err::throwOutOfRange("Time part amount is outside the supported range");
        }
        return fromAmount(amount);
    }

private:
    /// Return the largest amount accepted by this part type.
    [[nodiscard]] static auto maximumAmount() noexcept -> Amount {
        using NativeAmount = typename Amount::NativeValue;
        return Amount{static_cast<NativeAmount>(tMaximum - tMinimum)};
    }
};

}
