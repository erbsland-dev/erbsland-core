// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitAmount.hpp"
#include "IntegerUnitIndex_fwd.hpp"
#include "IntegerUnitOffset.hpp"

#include "impl/Throw.hpp"
#include "impl/TypeTraits.hpp"

#include "../math/ConstexprSaturatingMath.hpp"
#include "../math/IntegerMath.hpp"
#include "../math/SaturatingMath.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <concepts>
#include <cstddef>
#include <limits>
#include <utility>

namespace erbsland::unit {

/// An integer index with a unit.
/// @seedoc{/reference/unit/integer_unit_types}
/// @tparam tIntegerUnit The integer unit for this index.
template <impl::ValidIntegerUnit tIntegerUnit>
class IntegerUnitIndex {
public:
    /// The raw unsigned integer type.
    using Value = tIntegerUnit::IndexType;
    /// The unit type.
    using Unit = tIntegerUnit;
    /// The matching length type.
    using Length = IntegerUnitAmount<tIntegerUnit>;
    /// The matching offset type.
    using Offset = IntegerUnitOffset<tIntegerUnit>;

    /// Whether this index reserves a special no-index state.
    static constexpr auto cHasNoIndex = tIntegerUnit::cHasNoIndex;
    /// The raw value used for the special no-index state.
    static constexpr Value cRawNoIndex = std::numeric_limits<Value>::max();
    /// The largest valid raw index value.
    static constexpr Value cRawMaximum = cHasNoIndex ? static_cast<Value>(cRawNoIndex - Value{1U}) : cRawNoIndex;

public:
    /// Create a zero index.
    constexpr IntegerUnitIndex() noexcept = default;
    /// Create an index from a raw value.
    /// @param value The raw value. Passing cRawNoIndex creates the special no-index value if the unit supports it.
    explicit constexpr IntegerUnitIndex(Value value) noexcept : _value{value} {}

    // defaults
    ~IntegerUnitIndex() = default;
    IntegerUnitIndex(const IntegerUnitIndex &) noexcept = default;
    auto operator=(const IntegerUnitIndex &) noexcept -> IntegerUnitIndex & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const IntegerUnitIndex &other, other._value);

    /// Advance this index by a length and return the saturated result.
    [[nodiscard]] constexpr auto operator+(const Length length) const noexcept -> IntegerUnitIndex {
        return advanced(length);
    }
    /// Advance this index by a length with saturation.
    auto operator+=(const Length length) noexcept -> IntegerUnitIndex & { return advance(length); }
    /// Retreat this index by a length and return the saturated result.
    [[nodiscard]] constexpr auto operator-(const Length length) const noexcept -> IntegerUnitIndex {
        return retreated(length);
    }
    /// Retreat this index by a length with saturation.
    auto operator-=(const Length length) noexcept -> IntegerUnitIndex & { return retreat(length); }
    /// Move this index by an offset and return the saturated result.
    [[nodiscard]] constexpr auto operator+(const Offset offset) const noexcept -> IntegerUnitIndex {
        return moved(offset);
    }
    /// Move this index by an offset with saturation.
    auto operator+=(const Offset offset) noexcept -> IntegerUnitIndex & { return move(offset); }
    /// Move this index by the negated offset and return the saturated result.
    [[nodiscard]] constexpr auto operator-(const Offset offset) const noexcept -> IntegerUnitIndex {
        return offset.isMinimum() ? advanced(offset.absoluteLength()) : moved(offset.negated());
    }
    /// Move this index by the negated offset with saturation.
    auto operator-=(const Offset offset) noexcept -> IntegerUnitIndex & {
        *this = *this - offset;
        return *this;
    }
    /// Increment this index by one with saturation.
    auto operator++() noexcept -> IntegerUnitIndex & { return increment(); }
    /// Increment this index by one with saturation and return the previous value.
    auto operator++(int) noexcept -> IntegerUnitIndex {
        auto result = *this;
        increment();
        return result;
    }
    /// Decrement this index by one with saturation.
    auto operator--() noexcept -> IntegerUnitIndex & { return decrement(); }
    /// Decrement this index by one with saturation and return the previous value.
    auto operator--(int) noexcept -> IntegerUnitIndex {
        auto result = *this;
        decrement();
        return result;
    }

public: // tests
    /// Test if this index is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _value == 0U; }
    /// Test if this index is one.
    [[nodiscard]] constexpr auto isOne() const noexcept -> bool { return _value == 1U; }
    /// Test if this index is at its minimum.
    [[nodiscard]] constexpr auto isMinimum() const noexcept -> bool { return _value == 0U; }
    /// Test if this index is at its largest valid value.
    [[nodiscard]] constexpr auto isMaximum() const noexcept -> bool { return _value == cRawMaximum; }
    /// Test if this index is the special no-index value.
    [[nodiscard]] constexpr auto isNoIndex() const noexcept -> bool { return cHasNoIndex && _value == cRawNoIndex; }
    /// Test if this index is a valid position.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return !isNoIndex(); }
    /// Test if this index is a valid position within the given length.
    [[nodiscard]] constexpr auto isWithin(const Length length) const noexcept -> bool {
        return !isNoIndex() && (length.isInfinite() || _value < length.toRawValue());
    }
    /// Test if advancing by the given length would keep this index within the given bounds.
    [[nodiscard]] constexpr auto wouldBeWithinAfterAdvance(const Length step, const Length bounds) const noexcept
        -> bool {
        return advanced(step).isWithin(bounds);
    }
    /// Test if retreating by the given length would keep this index within the given bounds.
    [[nodiscard]] constexpr auto wouldBeWithinAfterRetreat(const Length step, const Length bounds) const noexcept
        -> bool {
        return retreated(step).isWithin(bounds);
    }
    /// Test if moving by the given offset would keep this index within the given bounds.
    [[nodiscard]] constexpr auto wouldBeWithinAfterMove(const Offset offset, const Length bounds) const noexcept
        -> bool {
        return moved(offset).isWithin(bounds);
    }
    /// Test if offsetFromZero() would return a saturated result.
    [[nodiscard]] constexpr auto wouldOffsetFromZeroSaturate() const noexcept -> bool {
        return isNoIndex() ||
            math::willAddBoundedSaturate(typename Offset::Value{0}, _value, Offset::cRawMinimum, Offset::cRawMaximum);
    }
    /// Test if offsetTo() would return a saturated result.
    [[nodiscard]] constexpr auto wouldOffsetToSaturate(const IntegerUnitIndex &other) const noexcept -> bool {
        if (isNoIndex() || other.isNoIndex()) {
            return true;
        }
        return math::willSubtractBoundedSaturate(other._value, _value, Offset::cRawMinimum, Offset::cRawMaximum);
    }

public: // accessors and modifiers
    /// Access the raw value of this index.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // math
    /// Advance this index by a non-negative length.
    /// Saturates at maximum() and keeps noIndex() unchanged.
    auto advance(const Length length) noexcept -> IntegerUnitIndex & {
        *this = advanced(length);
        return *this;
    }
    /// Advance this index by a non-negative length.
    /// @throws OverflowError if this index is noIndex() or the result would exceed maximum().
    auto advanceOrThrow(const Length length) -> IntegerUnitIndex & {
        *this = advancedOrThrow(length);
        return *this;
    }
    /// Create a new index advanced by a non-negative length.
    /// Saturates at maximum() and keeps noIndex() unchanged.
    [[nodiscard]] constexpr auto advanced(const Length length) const noexcept -> IntegerUnitIndex {
        if (isNoIndex()) {
            return *this;
        }
        if (length.isInfinite() || length.toRawValue() > (cRawMaximum - _value)) {
            return IntegerUnitIndex{math::saturatingAddBounded(_value, length.toRawValue(), Value{0U}, cRawMaximum)};
        }
        return IntegerUnitIndex{math::saturatingAddBounded(_value, length.toRawValue(), Value{0U}, cRawMaximum)};
    }
    /// Create a new index advanced by a non-negative length.
    /// @throws OverflowError if this index is noIndex() or the result would exceed maximum().
    [[nodiscard]] constexpr auto advancedOrThrow(const Length length) const -> IntegerUnitIndex {
        if (isNoIndex()) {
            impl::throwOverflow("Cannot advance the no-index value");
        }
        if (length.isInfinite() || length.toRawValue() > (cRawMaximum - _value)) {
            impl::throwOverflow("Advance would exceed index bounds");
        }
        return IntegerUnitIndex{math::saturatingAddBounded(_value, length.toRawValue(), Value{0U}, cRawMaximum)};
    }
    /// Retreat this index by a non-negative length.
    /// Saturates at zero and keeps noIndex() unchanged.
    auto retreat(const Length length) noexcept -> IntegerUnitIndex & {
        *this = retreated(length);
        return *this;
    }
    /// Retreat this index by a non-negative length.
    /// @throws OverflowError if this index is noIndex() or the result would be before zero.
    auto retreatOrThrow(const Length length) -> IntegerUnitIndex & {
        *this = retreatedOrThrow(length);
        return *this;
    }
    /// Create a new index retreated by a non-negative length.
    /// Saturates at zero and keeps noIndex() unchanged.
    [[nodiscard]] constexpr auto retreated(const Length length) const noexcept -> IntegerUnitIndex {
        if (isNoIndex()) {
            return *this;
        }
        if (length.isInfinite() || length.toRawValue() > _value) {
            return IntegerUnitIndex{
                math::saturatingSubtractBounded(_value, length.toRawValue(), Value{0U}, cRawMaximum)};
        }
        return IntegerUnitIndex{math::saturatingSubtractBounded(_value, length.toRawValue(), Value{0U}, cRawMaximum)};
    }
    /// Create a new index retreated by a non-negative length.
    /// @throws OverflowError if this index is noIndex() or the result would be before zero.
    [[nodiscard]] constexpr auto retreatedOrThrow(const Length length) const -> IntegerUnitIndex {
        if (isNoIndex()) {
            impl::throwOverflow("Cannot retreat the no-index value");
        }
        if (length.isInfinite() || length.toRawValue() > _value) {
            impl::throwOverflow("Retreat would exceed index bounds");
        }
        return IntegerUnitIndex{math::saturatingSubtractBounded(_value, length.toRawValue(), Value{0U}, cRawMaximum)};
    }
    /// Move this index by a signed offset.
    /// Saturates at zero or maximum() and keeps noIndex() unchanged.
    auto move(const Offset offset) noexcept -> IntegerUnitIndex & {
        *this = moved(offset);
        return *this;
    }
    /// Move this index by a signed offset.
    /// @throws OverflowError if this index is noIndex() or the result would be outside zero to maximum().
    auto moveOrThrow(const Offset offset) -> IntegerUnitIndex & {
        *this = movedOrThrow(offset);
        return *this;
    }
    /// Create a new index moved by a signed offset.
    /// Saturates at zero or maximum() and keeps noIndex() unchanged.
    [[nodiscard]] constexpr auto moved(const Offset offset) const noexcept -> IntegerUnitIndex {
        if (isNoIndex()) {
            return *this;
        }
        return IntegerUnitIndex{math::saturatingAddBounded(_value, offset.toRawValue(), Value{0U}, cRawMaximum)};
    }
    /// Create a new index moved by a signed offset.
    /// @throws OverflowError if this index is noIndex() or the result would be outside zero to maximum().
    [[nodiscard]] constexpr auto movedOrThrow(const Offset offset) const -> IntegerUnitIndex {
        if (isNoIndex()) {
            impl::throwOverflow("Cannot move the no-index value");
        }
        if (math::willAddBoundedSaturate(_value, offset.toRawValue(), Value{0U}, cRawMaximum)) {
            impl::throwOverflow("Move would exceed index bounds");
        }
        return IntegerUnitIndex{math::saturatingAddBounded(_value, offset.toRawValue(), Value{0U}, cRawMaximum)};
    }
    /// Increment this index by one.
    auto increment() noexcept -> IntegerUnitIndex & { return advance(Length::one()); }
    /// Decrement this index by one.
    auto decrement() noexcept -> IntegerUnitIndex & { return retreat(Length::one()); }
    /// Create a new index incremented by one.
    [[nodiscard]] constexpr auto incremented() const noexcept -> IntegerUnitIndex { return advanced(Length::one()); }
    /// Create a new index decremented by one.
    [[nodiscard]] constexpr auto decremented() const noexcept -> IntegerUnitIndex { return retreated(Length::one()); }
    /// Advance this index without checking special states or arithmetic bounds.
    /// The caller must ensure this index and `length` are finite and that the result remains a valid index.
    constexpr auto uncheckedAdvance(const Length length) noexcept -> IntegerUnitIndex & {
        _value = static_cast<Value>(_value + length.toRawValue());
        return *this;
    }
    /// Increment this index without checking special states or arithmetic bounds.
    /// The caller must ensure this is a valid index below maximum().
    constexpr auto uncheckedIncrement() noexcept -> IntegerUnitIndex & {
        ++_value;
        return *this;
    }

public: // conversion
    /// Create a grid index from row and column indices, bound to with and height of a grid.
    /// A grid index is a sequential index that represents a position in a grid.
    /// If x or y are out of bounds, or any of the parameters is infinite or no index, returns noIndex.
    /// @param x The index in the x position.
    /// @param y The index in the y position.
    /// @param width The width of the grid.
    /// @param height The height of the grid.
    /// @return The grid index or noIndex if any parameter is invalid.
    [[nodiscard]] static auto fromGrid(
        const IntegerUnitIndex x, const IntegerUnitIndex y, const Length width, const Length height) noexcept
        -> IntegerUnitIndex {
        if (x.isNoIndex() || y.isNoIndex() || width.isInfinite() || height.isInfinite() || !x.isWithin(width) ||
            !y.isWithin(height)) {
            return noIndex();
        }
        if (math::willMultiplyOverflow(width.toRawValue(), y.toRawValue())) {
            return noIndex();
        }
        const auto rowIndex = width.toRawValue() * y.toRawValue();
        if (math::willAddOverflow(rowIndex, x.toRawValue())) {
            return noIndex();
        }
        return IntegerUnitIndex{rowIndex + x.toRawValue()};
    }
    /// Return the index, flipped in the given range.
    [[nodiscard]] auto flipped(Length length) const noexcept -> IntegerUnitIndex {
        if (isNoIndex() || length.isInfinite()) {
            return noIndex();
        }
        return IntegerUnitIndex{length.toRawValue() - _value - Value{1U}};
    }
    /// Get the distance from zero to this index.
    /// @return The distance from zero, or infinite if this index is noIndex().
    [[nodiscard]] constexpr auto distanceFromZero() const noexcept -> Length { return Length{_value}; }
    /// Get the absolute distance from this index to another index.
    /// @return The absolute distance, or infinite if either index is noIndex().
    [[nodiscard]] constexpr auto absoluteDistanceTo(const IntegerUnitIndex &other) const noexcept -> Length {
        if (isNoIndex() || other.isNoIndex()) {
            return Length::infinite();
        }
        return Length{math::integerAbsoluteDifference(_value, other._value)};
    }
    /// Get the signed offset from zero to this index.
    /// @return The offset from zero, saturated to Offset::maximum() if this index is noIndex() or too large.
    [[nodiscard]] constexpr auto offsetFromZero() const noexcept -> Offset {
        if (wouldOffsetFromZeroSaturate()) {
            return Offset::maximum();
        }
        return Offset{
            math::saturatingAddBounded(typename Offset::Value{0}, _value, Offset::cRawMinimum, Offset::cRawMaximum)};
    }
    /// Get the signed offset from zero to this index.
    /// @throws OverflowError if this index is noIndex() or the result does not fit into Offset.
    [[nodiscard]] constexpr auto offsetFromZeroOrThrow() const -> Offset {
        if (wouldOffsetFromZeroSaturate()) {
            impl::throwOverflow("Offset from zero would exceed offset bounds");
        }
        return Offset{static_cast<Offset::Value>(_value)};
    }
    /// Get the signed offset from this index to another index.
    /// @return The directional offset, saturated to Offset::minimum() or Offset::maximum() if necessary.
    ///   If either index is noIndex(), this returns Offset::maximum().
    [[nodiscard]] constexpr auto offsetTo(const IntegerUnitIndex &other) const noexcept -> Offset {
        if (isNoIndex() || other.isNoIndex()) {
            return Offset::maximum();
        }
        return Offset{math::saturatingSubtractBounded(other._value, _value, Offset::cRawMinimum, Offset::cRawMaximum)};
    }
    /// Get the signed offset from this index to another index.
    /// @throws OverflowError if either index is noIndex() or the result does not fit into Offset.
    [[nodiscard]] constexpr auto offsetToOrThrow(const IntegerUnitIndex &other) const -> Offset {
        if (wouldOffsetToSaturate(other)) {
            impl::throwOverflow("Offset between indexes would exceed offset bounds");
        }
        return offsetTo(other);
    }
    /// Convert this index to `std::size_t`, saturating if the raw value is too large.
    [[nodiscard]] constexpr auto toSizeT() const noexcept -> std::size_t {
        // Only check for overflow if the value type is larger than std::size_t.
        // This mainly speeds up debug builds by avoiding unnecessary checks.
        if constexpr (sizeof(Value) > sizeof(std::size_t)) {
            return math::saturatingCast<std::size_t>(_value);
        } else {
            return static_cast<std::size_t>(_value);
        }
    }
    /// Convert this index to `std::size_t`.
    /// @throws OverflowError if the raw value does not fit into `std::size_t`.
    [[nodiscard]] constexpr auto toSizeTOrThrow() const -> std::size_t {
        // Only check for overflow if the value type is larger than std::size_t.
        // This mainly speeds up debug builds by avoiding unnecessary checks.
        if constexpr (sizeof(Value) > sizeof(std::size_t)) {
            if (math::willCastOverflow<std::size_t>(_value)) {
                impl::throwOverflow("Index value exceeds maximum std::size_t");
            }
        }
        return static_cast<std::size_t>(_value);
    }
    /// Swap two indexes.
    friend void swap(IntegerUnitIndex &first, IntegerUnitIndex &second) noexcept {
        std::swap(first._value, second._value);
    }

public: // factory methods
    /// Return the smallest valid index.
    [[nodiscard]] constexpr static auto minimum() noexcept -> IntegerUnitIndex { return IntegerUnitIndex{0U}; }
    /// Return the largest valid index.
    [[nodiscard]] constexpr static auto maximum() noexcept -> IntegerUnitIndex { return IntegerUnitIndex{cRawMaximum}; }
    /// Return the zero index.
    [[nodiscard]] constexpr static auto zero() noexcept -> IntegerUnitIndex { return IntegerUnitIndex{0U}; }
    /// Return the index one.
    [[nodiscard]] constexpr static auto one() noexcept -> IntegerUnitIndex { return IntegerUnitIndex{1U}; }
    /// Return the special no-index value.
    [[nodiscard]] constexpr static auto noIndex() noexcept -> IntegerUnitIndex { return IntegerUnitIndex{cRawNoIndex}; }
    /// Return the end index for a length measured from zero.
    /// Infinite length is converted into noIndex().
    [[nodiscard]] constexpr static auto end(const Length length) noexcept -> IntegerUnitIndex {
        return length.isInfinite() ? noIndex() : IntegerUnitIndex{length.toRawValue()};
    }
    /// Create an index from a size_t value.
    /// Saturating if the value overflows.
    [[nodiscard]] constexpr static auto fromSizeT(const std::size_t value) noexcept -> IntegerUnitIndex {
        if constexpr (sizeof(Value) < sizeof(std::size_t)) {
            if (math::willCastOverflow<Value>(value)) {
                return maximum();
            }
        }
        return static_cast<Value>(value) > cRawMaximum ? maximum() : IntegerUnitIndex{static_cast<Value>(value)};
    }
    /// Create an index from a size_t value.
    /// @throws OverflowError if the size_t value exceeds the maximum index.
    [[nodiscard]] constexpr static auto fromSizeTOrThrow(const std::size_t value) -> IntegerUnitIndex {
        if constexpr (sizeof(Value) < sizeof(std::size_t)) {
            if (math::willCastOverflow<Value>(value)) {
                impl::throwOverflow("The size_t value exceeds maximum index.");
            }
        }
        if (static_cast<Value>(value) > cRawMaximum) {
            impl::throwOverflow("The size_t value exceeds maximum index.");
        }
        return IntegerUnitIndex{static_cast<Value>(value)};
    }

private:
    Value _value{0U}; ///< The raw value of this index.
};

}

template <erbsland::unit::impl::ValidIntegerUnit tIntegerUnit>
struct std::hash<erbsland::unit::IntegerUnitIndex<tIntegerUnit>> {
    auto operator()(const erbsland::unit::IntegerUnitIndex<tIntegerUnit> &value) const noexcept -> std::size_t {
        return std::hash<typename erbsland::unit::IntegerUnitIndex<tIntegerUnit>::Value>{}(value.toRawValue());
    }
};
