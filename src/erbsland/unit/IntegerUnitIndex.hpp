// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitCount.hpp"
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
#include <cstdint>
#include <limits>
#include <utility>

namespace erbsland::unit {

/// An integer index with a unit.
/// @seedoc{/reference/unit/units_and_versions}
/// @tparam tIntegerUnit The integer unit for this index.
/// @tested{IntegerUnitTest LastIndexTest}
template <impl::ValidIntegerUnit tIntegerUnit>
class IntegerUnitIndex {
public:
    /// Tag for unchecked construction.
    struct Unchecked {};
    /// The raw unsigned integer type.
    using Value = uint64_t;
    /// The unit type.
    using Unit = tIntegerUnit;
    /// The matching count type.
    using Count = IntegerUnitCount<tIntegerUnit>;
    /// The matching offset type.
    using Offset = IntegerUnitOffset<tIntegerUnit>;

    /// The raw value used for the special no-index state.
    static constexpr Value cRawNoIndex = std::numeric_limits<Value>::max();
    /// The largest valid raw index value.
    static constexpr Value cRawMaximum = cRawNoIndex - 1U;

public:
    /// Create a zero index.
    constexpr IntegerUnitIndex() noexcept = default;
    /// Construct a finite value from a compile-time integer.
    /// @param value An integer within the finite domain; invalid values fail compilation.
    template <math::NativeInteger T>
    explicit consteval IntegerUnitIndex(T value) : _value{checkedInteger<T>(value)} {}
    /// Construct directly from a raw representation without conversion or range checks.
    /// The caller must ensure the input fits Value before conversion and represents the intended finite or special
    /// state.
    /// @param value The raw unsigned integer value.
    constexpr IntegerUnitIndex(const Value value, Unchecked) noexcept : _value{value} {}

    // defaults
    ~IntegerUnitIndex() = default;
    IntegerUnitIndex(const IntegerUnitIndex &) noexcept = default;
    constexpr auto operator=(const IntegerUnitIndex &) noexcept -> IntegerUnitIndex & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const IntegerUnitIndex &other, other._value);

    /// Advance this index by a length and return the saturated result.
    [[nodiscard]] constexpr auto operator+(const Count length) const noexcept -> IntegerUnitIndex {
        return advanced(length);
    }
    /// Advance this index by a length with saturation.
    constexpr auto operator+=(const Count length) noexcept -> IntegerUnitIndex & { return advance(length); }
    /// Retreat this index by a length and return the saturated result.
    [[nodiscard]] constexpr auto operator-(const Count length) const noexcept -> IntegerUnitIndex {
        return retreated(length);
    }
    /// Retreat this index by a length with saturation.
    constexpr auto operator-=(const Count length) noexcept -> IntegerUnitIndex & { return retreat(length); }
    /// Move this index by an offset and return the saturated result.
    [[nodiscard]] constexpr auto operator+(const Offset offset) const noexcept -> IntegerUnitIndex {
        return moved(offset);
    }
    /// Move this index by an offset with saturation.
    constexpr auto operator+=(const Offset offset) noexcept -> IntegerUnitIndex & { return move(offset); }
    /// Move this index by the negated offset and return the saturated result.
    [[nodiscard]] constexpr auto operator-(const Offset offset) const noexcept -> IntegerUnitIndex {
        if (offset.isMinimum()) {
            return advanced(offset.absoluteLength());
        }
        return moved(offset.negated());
    }
    /// Move this index by the negated offset with saturation.
    constexpr auto operator-=(const Offset offset) noexcept -> IntegerUnitIndex & {
        *this = *this - offset;
        return *this;
    }
    /// Increment this index by one with saturation.
    constexpr auto operator++() noexcept -> IntegerUnitIndex & { return increment(); }
    /// Increment this index by one with saturation and return the previous value.
    constexpr auto operator++(int) noexcept -> IntegerUnitIndex {
        auto result = *this;
        increment();
        return result;
    }
    /// Decrement this index by one with saturation.
    constexpr auto operator--() noexcept -> IntegerUnitIndex & { return decrement(); }
    /// Decrement this index by one with saturation and return the previous value.
    constexpr auto operator--(int) noexcept -> IntegerUnitIndex {
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
    [[nodiscard]] constexpr auto isNoIndex() const noexcept -> bool { return _value == cRawNoIndex; }
    /// Test if this index is a valid position.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return !isNoIndex(); }
    /// Test if this index is a valid position within the given length.
    [[nodiscard]] constexpr auto isWithin(const Count length) const noexcept -> bool {
        return _value < length.toRawValue();
    }
    /// Test whether exact finite advance is impossible.
    [[nodiscard]] constexpr auto wouldAdvanceSaturate(Count length) const noexcept -> bool {
        return isNoIndex() || length.toRawValue() > cRawMaximum - _value;
    }
    /// Test whether exact movement lands within bounds without saturation.
    [[nodiscard]] constexpr auto canAdvance(Count length, Count bounds) const noexcept -> bool {
        return !wouldAdvanceSaturate(length) && uncheckedAdvanced(length).isWithin(bounds);
    }
    /// Test whether exact finite retreat is impossible.
    [[nodiscard]] constexpr auto wouldRetreatSaturate(Count length) const noexcept -> bool {
        return isNoIndex() || length.toRawValue() > _value;
    }
    /// Test whether exact movement lands within bounds without saturation.
    [[nodiscard]] constexpr auto canRetreat(Count length, Count bounds) const noexcept -> bool {
        return !wouldRetreatSaturate(length) && uncheckedRetreated(length).isWithin(bounds);
    }
    /// Test whether exact finite move is impossible.
    [[nodiscard]] constexpr auto wouldMoveSaturate(Offset offset) const noexcept -> bool {
        if (offset.isNegative()) {
            return wouldRetreatSaturate(offset.absoluteLength());
        }
        return wouldAdvanceSaturate(offset.absoluteLength());
    }
    /// Test whether exact movement lands within bounds without saturation.
    [[nodiscard]] constexpr auto canMove(Offset offset, Count bounds) const noexcept -> bool {
        return !wouldMoveSaturate(offset) && uncheckedMoved(offset).isWithin(bounds);
    }

public: // accessors and modifiers
    /// Access the raw value of this index.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // math
    /// Advance this index by a non-negative length.
    /// Saturates at maximum() and keeps noIndex() unchanged.
    constexpr auto advance(const Count length) noexcept -> IntegerUnitIndex & {
        *this = advanced(length);
        return *this;
    }
    /// Advance this index by a non-negative length.
    /// @throws OverflowError if this index is noIndex() or the result would exceed maximum().
    constexpr auto advanceOrThrow(const Count length) -> IntegerUnitIndex & {
        *this = advancedOrThrow(length);
        return *this;
    }
    /// Create a new index advanced by a non-negative length.
    /// Saturates at maximum() and keeps noIndex() unchanged.
    [[nodiscard]] constexpr auto advanced(const Count length) const noexcept -> IntegerUnitIndex {
        if (isNoIndex()) {
            return *this;
        }
        if (length.toRawValue() > cRawMaximum - _value) {
            return maximum();
        }
        return uncheckedAdvanced(length);
    }
    /// Create a new index advanced by a non-negative length.
    /// @throws OverflowError if this index is noIndex() or the result would exceed maximum().
    [[nodiscard]] constexpr auto advancedOrThrow(const Count length) const -> IntegerUnitIndex {
        if (isNoIndex()) {
            impl::throwOverflow("Cannot advance the no-index value");
        }
        if (length.toRawValue() > cRawMaximum - _value) {
            impl::throwOverflow("Advance would exceed index bounds");
        }
        return uncheckedAdvanced(length);
    }
    /// Retreat this index by a non-negative length.
    /// Saturates at zero and keeps noIndex() unchanged.
    constexpr auto retreat(const Count length) noexcept -> IntegerUnitIndex & {
        *this = retreated(length);
        return *this;
    }
    /// Retreat this index by a non-negative length.
    /// @throws OverflowError if this index is noIndex() or the result would be before zero.
    constexpr auto retreatOrThrow(const Count length) -> IntegerUnitIndex & {
        *this = retreatedOrThrow(length);
        return *this;
    }
    /// Create a new index retreated by a non-negative length.
    /// Saturates at zero and keeps noIndex() unchanged.
    [[nodiscard]] constexpr auto retreated(const Count length) const noexcept -> IntegerUnitIndex {
        if (isNoIndex()) {
            return *this;
        }
        if (length.toRawValue() > _value) {
            return zero();
        }
        return uncheckedRetreated(length);
    }
    /// Create a new index retreated by a non-negative length.
    /// @throws OverflowError if this index is noIndex() or the result would be before zero.
    [[nodiscard]] constexpr auto retreatedOrThrow(const Count length) const -> IntegerUnitIndex {
        if (isNoIndex()) {
            impl::throwOverflow("Cannot retreat the no-index value");
        }
        if (length.toRawValue() > _value) {
            impl::throwOverflow("Retreat would exceed index bounds");
        }
        return uncheckedRetreated(length);
    }
    /// Move this index by a signed offset.
    /// Saturates at zero or maximum() and keeps noIndex() unchanged.
    constexpr auto move(const Offset offset) noexcept -> IntegerUnitIndex & {
        *this = moved(offset);
        return *this;
    }
    /// Move this index by a signed offset.
    /// @throws OverflowError if this index is noIndex() or the result would be outside zero to maximum().
    constexpr auto moveOrThrow(const Offset offset) -> IntegerUnitIndex & {
        *this = movedOrThrow(offset);
        return *this;
    }
    /// Create a new index moved by a signed offset.
    /// Saturates at zero or maximum() and keeps noIndex() unchanged.
    [[nodiscard]] constexpr auto moved(const Offset offset) const noexcept -> IntegerUnitIndex {
        if (offset.isNegative()) {
            return retreated(offset.absoluteLength());
        }
        return advanced(offset.absoluteLength());
    }
    /// Create a new index moved by a signed offset.
    /// @throws OverflowError if this index is noIndex() or the result would be outside zero to maximum().
    [[nodiscard]] constexpr auto movedOrThrow(const Offset offset) const -> IntegerUnitIndex {
        if (wouldMoveSaturate(offset)) {
            impl::throwOverflow("Move would exceed index bounds");
        }
        return uncheckedMoved(offset);
    }
    /// Increment this index by one.
    constexpr auto increment() noexcept -> IntegerUnitIndex & {
        if (_value >= cRawMaximum) {
            return *this;
        }
        ++_value;
        return *this;
    }
    /// Decrement this index by one.
    constexpr auto decrement() noexcept -> IntegerUnitIndex & {
        if (_value == 0U || _value == cRawNoIndex) {
            return *this;
        }
        --_value;
        return *this;
    }
    /// Create a new index incremented by one.
    [[nodiscard]] constexpr auto incremented() const noexcept -> IntegerUnitIndex {
        if (_value >= cRawMaximum) {
            return {_value, Unchecked{}};
        }
        return {_value + 1U, Unchecked{}};
    }
    /// Create a new index decremented by one.
    [[nodiscard]] constexpr auto decremented() const noexcept -> IntegerUnitIndex {
        if (_value == 0U || _value == cRawNoIndex) {
            return {_value, Unchecked{}};
        }
        return {_value - 1U, Unchecked{}};
    }

public: // unchecked fast-track methods
    /// Advance this index without saturation; inputs must be finite and the result a valid index.
    /// @param length The exact movement, already checked by the caller.
    constexpr auto uncheckedAdvance(const Count length) noexcept -> IntegerUnitIndex & {
        _value = _value + length.toRawValue();
        return *this;
    }
    /// Return an index moved without saturation; inputs must be finite and the result a valid index.
    /// @param length The exact movement, already checked by the caller.
    /// @return The exactly moved index.
    [[nodiscard]] constexpr auto uncheckedAdvanced(const Count length) const noexcept -> IntegerUnitIndex {
        return {_value + length.toRawValue(), Unchecked{}};
    }
    /// Retreat this index without saturation; inputs must be finite and the result a valid index.
    /// @param length The exact movement, already checked by the caller.
    constexpr auto uncheckedRetreat(const Count length) noexcept -> IntegerUnitIndex & {
        _value = _value - length.toRawValue();
        return *this;
    }
    /// Return an index moved without saturation; inputs must be finite and the result a valid index.
    /// @param length The exact movement, already checked by the caller.
    /// @return The exactly moved index.
    [[nodiscard]] constexpr auto uncheckedRetreated(const Count length) const noexcept -> IntegerUnitIndex {
        return {_value - length.toRawValue(), Unchecked{}};
    }
    /// Move this index without saturation; inputs must be finite and the result a valid index.
    /// @param offset The exact movement, already checked by the caller.
    constexpr auto uncheckedMove(const Offset offset) noexcept -> IntegerUnitIndex & {
        _value = _value + static_cast<Value>(offset.toRawValue());
        return *this;
    }
    /// Return an index moved without saturation; inputs must be finite and the result a valid index.
    /// @param offset The exact movement, already checked by the caller.
    /// @return The exactly moved index.
    [[nodiscard]] constexpr auto uncheckedMoved(const Offset offset) const noexcept -> IntegerUnitIndex {
        return {_value + static_cast<Value>(offset.toRawValue()), Unchecked{}};
    }
    /// Increment this index without checks; this index and the result must be valid.
    constexpr auto uncheckedIncrement() noexcept -> IntegerUnitIndex & {
        ++_value;
        return *this;
    }
    /// Return the adjacent index without checks; this index and the result must be valid.
    /// @return The exactly moved index.
    [[nodiscard]] constexpr auto uncheckedIncremented() const noexcept -> IntegerUnitIndex {
        return {_value + 1U, Unchecked{}};
    }
    /// Decrement this index without checks; this index and the result must be valid.
    constexpr auto uncheckedDecrement() noexcept -> IntegerUnitIndex & {
        --_value;
        return *this;
    }
    /// Return the adjacent index without checks; this index and the result must be valid.
    /// @return The exactly moved index.
    [[nodiscard]] constexpr auto uncheckedDecremented() const noexcept -> IntegerUnitIndex {
        return {_value - 1U, Unchecked{}};
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
        const IntegerUnitIndex x, const IntegerUnitIndex y, const Count width, const Count height) noexcept
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
        return {rowIndex + x.toRawValue(), Unchecked{}};
    }
    /// Mirror a position within a finite length, or return noIndex() if outside it.
    [[nodiscard]] constexpr auto mirroredWithin(Count length) const noexcept -> IntegerUnitIndex {
        if (length.isInfinite() || !isWithin(length)) {
            return noIndex();
        }
        return {length.toRawValue() - _value - Value{1U}, Unchecked{}};
    }
    /// Get the distance from zero to this index.
    /// @return The distance from zero, or infinite if this index is noIndex().
    [[nodiscard]] constexpr auto distanceFromZero() const noexcept -> Count {
        return Count{_value, typename Count::Unchecked{}};
    }
    /// Get the absolute distance from this index to another index.
    /// @return The absolute distance, or infinite if either index is noIndex().
    [[nodiscard]] constexpr auto absoluteDistanceTo(const IntegerUnitIndex &other) const noexcept -> Count {
        if (isNoIndex() || other.isNoIndex()) {
            return Count::infinite();
        }
        return Count{math::integerAbsoluteDifference(_value, other._value), typename Count::Unchecked{}};
    }
    /// Get the signed offset from zero to this index.
    /// @return The offset from zero, saturated to Offset::maximum() if this index is noIndex() or too large.
    [[nodiscard]] constexpr auto offsetFromZero() const noexcept -> Offset {
        if (offsetFromZeroOverflows()) {
            return Offset::maximum();
        }
        return Offset{static_cast<typename Offset::Value>(_value), typename Offset::Unchecked{}};
    }
    /// Get the signed offset from zero to this index.
    /// @throws OverflowError if this index is noIndex() or the result does not fit into Offset.
    [[nodiscard]] constexpr auto offsetFromZeroOrThrow() const -> Offset {
        if (offsetFromZeroOverflows()) {
            impl::throwOverflow("Offset from zero would exceed offset bounds");
        }
        return Offset{static_cast<typename Offset::Value>(_value), typename Offset::Unchecked{}};
    }
    /// Get the signed offset from this index to another index.
    /// @return The directional offset, saturated to Offset::minimum() or Offset::maximum() if necessary.
    ///   If either index is noIndex(), this returns Offset::maximum().
    [[nodiscard]] constexpr auto offsetTo(const IntegerUnitIndex &other) const noexcept -> Offset {
        if (isNoIndex() || other.isNoIndex()) {
            return Offset::maximum();
        }
        return Offset{
            math::saturatingSubtractBounded(other._value, _value, Offset::cRawMinimum, Offset::cRawMaximum),
            typename Offset::Unchecked{}};
    }
    /// Get the signed offset from this index to another index.
    /// @throws OverflowError if either index is noIndex() or the result does not fit into Offset.
    [[nodiscard]] constexpr auto offsetToOrThrow(const IntegerUnitIndex &other) const -> Offset {
        if (offsetToOverflows(other)) {
            impl::throwOverflow("Offset between indexes would exceed offset bounds");
        }
        // Unsigned subtraction followed by conversion preserves the exact signed difference in C++20.
        return Offset{static_cast<typename Offset::Value>(other._value - _value), typename Offset::Unchecked{}};
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
    [[nodiscard]] constexpr static auto minimum() noexcept -> IntegerUnitIndex { return {0U, Unchecked{}}; }
    /// Return the largest valid index.
    [[nodiscard]] constexpr static auto maximum() noexcept -> IntegerUnitIndex { return {cRawMaximum, Unchecked{}}; }
    /// Return the zero index.
    [[nodiscard]] constexpr static auto zero() noexcept -> IntegerUnitIndex { return {0U, Unchecked{}}; }
    /// Return the index one.
    [[nodiscard]] constexpr static auto one() noexcept -> IntegerUnitIndex { return {1U, Unchecked{}}; }
    /// Return the special no-index value.
    [[nodiscard]] constexpr static auto noIndex() noexcept -> IntegerUnitIndex { return {cRawNoIndex, Unchecked{}}; }
    /// Return the end index for a length measured from zero.
    /// Infinite length is converted into noIndex().
    [[nodiscard]] constexpr static auto end(const Count length) noexcept -> IntegerUnitIndex {
        // Infinite length is the same raw-value as no-index - therefore no conversion is needed.
        return {length.toRawValue(), Unchecked{}};
    }
    /// Return the last index within a length measured from zero.
    /// @param length The sequence length in the matching unit family.
    /// @return The final included index, or noIndex() for a zero or infinite length.
    [[nodiscard]] constexpr static auto last(const Count length) noexcept -> IntegerUnitIndex {
        if (length.isInfinite()) {
            return noIndex();
        }
        // Unsigned subtraction maps zero directly to the no-index representation.
        return {length.toRawValue() - 1U, Unchecked{}};
    }
    /// Create a finite value from a native size, saturating at maximum().
    [[nodiscard]] constexpr static auto fromSizeT(std::size_t value) noexcept -> IntegerUnitIndex {
        return fromInteger(value);
    }
    /// Create a finite value from a native size.
    /// @throws err::OverflowError if the value exceeds maximum().
    [[nodiscard]] constexpr static auto fromSizeTOrThrow(std::size_t value) -> IntegerUnitIndex {
        return fromIntegerOrThrow(value);
    }
    /// Convert an integer, saturating into the finite domain.
    /// @param value The integer to convert.
    /// @return The bounded finite value.
    template <math::NativeInteger T>
    [[nodiscard]] constexpr static auto fromInteger(T value) noexcept -> IntegerUnitIndex {
        // NativeInteger guarantees max 64bit, therefore  T is same size or a smaller than Value.
        if constexpr (std::is_signed_v<T>) {
            if (value < 0) {
                return minimum();
            }
            return {static_cast<Value>(value), Unchecked{}};
        } else if constexpr (sizeof(T) < sizeof(Value)) {
            return {static_cast<Value>(value), Unchecked{}};
        } else {
            const auto raw = static_cast<Value>(value);
            if (raw > cRawMaximum) {
                return maximum();
            }
            return {static_cast<Value>(raw), Unchecked{}};
        }
    }
    /// Convert an integer exactly into the finite domain.
    /// @param value The integer to convert.
    /// @return The finite value.
    /// @throws err::OverflowError if value is outside the finite domain.
    template <math::NativeInteger T>
    [[nodiscard]] constexpr static auto fromIntegerOrThrow(T value) -> IntegerUnitIndex {
        return {checkedInteger<T>(value), Unchecked{}};
    }

private:
    /// Check if an integer is in the range of this unit.
    /// @param value The integer to check.
    /// @return The checked value.
    /// @throws err::OverflowError if value is outside the finite domain.
    template <math::NativeInteger T>
    [[nodiscard]] constexpr static auto checkedInteger(T value) -> Value {
        if (math::willCastOverflow<Value>(value)) {
            impl::throwOverflow("Integer is outside the unit's finite range");
        }
        const auto raw = static_cast<Value>(value);
        if (raw > cRawMaximum) {
            impl::throwOverflow("Integer is outside the unit's finite range");
        }
        return raw;
    }
    /// Test if offsetFromZero() would return a saturated result.
    [[nodiscard]] constexpr auto offsetFromZeroOverflows() const noexcept -> bool {
        return _value > static_cast<Value>(Offset::cRawMaximum);
    }
    /// Test if offsetTo() would return a saturated result.
    [[nodiscard]] constexpr auto offsetToOverflows(const IntegerUnitIndex &other) const noexcept -> bool {
        if (isNoIndex() || other.isNoIndex()) {
            return true;
        }
        return math::willSubtractBoundedSaturate(other._value, _value, Offset::cRawMinimum, Offset::cRawMaximum);
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
