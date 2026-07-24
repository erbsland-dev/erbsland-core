// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitAmount.hpp"
#include "IntegerUnitIndex.hpp"
#include "IntegerUnitOffset.hpp"
#include "IntegerUnitRange_fwd.hpp"

#include "impl/TypeTraits.hpp"

#include "../util/HashHelper.hpp"
#include "../util/impl/ComparisonHelper.hpp"
#include "../util/impl/LoopControl.hpp"
#include "../util/LoopResult.hpp"
#include "../util/LoopStatus.hpp"

#include <compare>
#include <cstddef>
#include <utility>

namespace erbsland::unit {

/// A range with a unit tag.
///
/// A range is represented as a start index and a length. The start index may be noIndex() only to represent an invalid
/// or absent range; contains() and isWithin() return false for such ranges.
///
/// @tparam tIntegerUnit The unit tag for this range.
template <impl::ValidIntegerUnit tIntegerUnit>
class IntegerUnitRange {
public:
    /// The unit type.
    using Unit = tIntegerUnit;
    /// The matching index type.
    using Index = IntegerUnitIndex<tIntegerUnit>;
    /// The matching length type.
    using Length = IntegerUnitAmount<tIntegerUnit>;
    /// The matching offset type.
    using Offset = IntegerUnitOffset<tIntegerUnit>;

public:
    /// Create an empty range starting at zero.
    constexpr IntegerUnitRange() noexcept = default;
    /// Create a range from a start index and length.
    constexpr IntegerUnitRange(Index index, Length length) noexcept : _index{index}, _length{length} {}
    /// Create a range from a beginning and end index.
    /// The end index must be greater than or equal to the start index.
    /// If the end index is "no index", the length of the range is infinite.
    /// The end index is considered one *after* the last unit.
    /// @param begin The index that marks the first element.
    /// @param end The index after the last element, or no-index for infinite length.
    constexpr IntegerUnitRange(Index begin, Index end) noexcept :
        _index{begin}, _length{end < begin ? Length{} : begin.absoluteDistanceTo(end)} {}

    /// Destroy this range.
    ~IntegerUnitRange() = default;
    /// Copy a range.
    IntegerUnitRange(const IntegerUnitRange &) noexcept = default;
    /// Copy another range into this range.
    auto operator=(const IntegerUnitRange &) noexcept -> IntegerUnitRange & = default;

public: // operators
    /// Compare this range with another range.
    constexpr auto operator<=>(const IntegerUnitRange &other) const noexcept -> std::strong_ordering {
        if (const auto result = _index <=> other._index; result != std::strong_ordering::equal) {
            return result;
        }
        return _length <=> other._length;
    }
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FROM_SPACESHIP(const IntegerUnitRange &other, other);

    /// Move this range by an offset and return the saturated result.
    [[nodiscard]] constexpr auto operator+(const Offset offset) const noexcept -> IntegerUnitRange {
        return moved(offset);
    }
    /// Move this range by an offset with saturation.
    auto operator+=(const Offset offset) noexcept -> IntegerUnitRange & { return move(offset); }
    /// Move this range by the negated offset and return the saturated result.
    [[nodiscard]] constexpr auto operator-(const Offset offset) const noexcept -> IntegerUnitRange {
        return offset.isMinimum() ? advanced(offset.absoluteLength()) : moved(offset.negated());
    }
    /// Move this range by the negated offset with saturation.
    auto operator-=(const Offset offset) noexcept -> IntegerUnitRange & {
        *this = *this - offset;
        return *this;
    }

public: // tests
    /// Test if this range starts at a valid index.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _index.isValid(); }
    /// Test if this range is empty.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _length.isZero(); }
    /// Test if this range has an infinite length.
    [[nodiscard]] constexpr auto isInfinite() const noexcept -> bool { return _length.isInfinite(); }
    /// Test if this range contains the given index.
    [[nodiscard]] constexpr auto contains(const Index index) const noexcept -> bool {
        return isValid() && !index.isNoIndex() && index >= _index && (isInfinite() || index < endIndex());
    }
    /// Test if this whole range fits within the given bounds length.
    [[nodiscard]] constexpr auto isWithin(const Length bounds) const noexcept -> bool {
        if (!isValid()) {
            return false;
        }
        if (bounds.isInfinite()) {
            return true;
        }
        if (isInfinite()) {
            return false;
        }
        const auto end = endIndex();
        return !end.isNoIndex() && end.toRawValue() <= bounds.toRawValue();
    }
    /// Return this range clamped to a sequence with the given bounds length.
    /// Invalid ranges stay invalid. Ranges starting beyond the bounds become empty at the end of the bounds.
    [[nodiscard]] constexpr auto clampedTo(const Length bounds) const noexcept -> IntegerUnitRange {
        if (!isValid()) {
            return noRange();
        }
        if (bounds.isInfinite()) {
            return *this;
        }
        if (!_index.isWithin(bounds)) {
            return emptyAt(Index::end(bounds));
        }

        const auto availableLength =
            Length{static_cast<typename Length::Value>(bounds.toRawValue() - _index.toRawValue())};
        if (_length.isInfinite() || _length > availableLength) {
            return IntegerUnitRange{_index, availableLength};
        }
        return *this;
    }

public: // accessors and modifiers
    /// Get the start index of this range.
    [[nodiscard]] constexpr auto index() const noexcept -> Index { return _index; }
    /// Get the length of this range.
    [[nodiscard]] constexpr auto length() const noexcept -> Length { return _length; }
    /// Set the start index of this range.
    constexpr void setIndex(Index index) noexcept { _index = index; }
    /// Set the length of this range.
    constexpr void setLength(Length length) noexcept { _length = length; }
    /// Get the first index after this range.
    /// If the range is invalid, infinite, or exceeds the representable index space, this returns Index::noIndex().
    [[nodiscard]] constexpr auto endIndex() const noexcept -> Index {
        if (!isValid() || _length.isInfinite()) {
            return Index::noIndex();
        }
        if (_length.toRawValue() >= (Index::cRawNoIndex - _index.toRawValue())) {
            return Index::noIndex();
        }
        return Index{static_cast<Index::Value>(_index.toRawValue() + _length.toRawValue())};
    }

public: // math
    /// Advance the start index by a non-negative length.
    auto advance(const Length length) noexcept -> IntegerUnitRange & {
        _index.advance(length);
        return *this;
    }
    /// Retreat the start index by a non-negative length.
    auto retreat(const Length length) noexcept -> IntegerUnitRange & {
        _index.retreat(length);
        return *this;
    }
    /// Move the start index by a signed offset.
    auto move(const Offset offset) noexcept -> IntegerUnitRange & {
        _index.move(offset);
        return *this;
    }
    /// Create a new range with the start index advanced by a non-negative length.
    [[nodiscard]] constexpr auto advanced(const Length length) const noexcept -> IntegerUnitRange {
        return IntegerUnitRange{_index.advanced(length), _length};
    }
    /// Create a new range with the start index retreated by a non-negative length.
    [[nodiscard]] constexpr auto retreated(const Length length) const noexcept -> IntegerUnitRange {
        return IntegerUnitRange{_index.retreated(length), _length};
    }
    /// Create a new range with the start index moved by a signed offset.
    [[nodiscard]] constexpr auto moved(const Offset offset) const noexcept -> IntegerUnitRange {
        return IntegerUnitRange{_index.moved(offset), _length};
    }
    /// Create a new range by interpreting this range as relative to the given origin index.
    [[nodiscard]] constexpr auto withOrigin(const Index origin) const noexcept -> IntegerUnitRange {
        if (!isValid() || !origin.isValid()) {
            return noRange();
        }
        return IntegerUnitRange{origin.advanced(Length::fromSizeT(_index.toSizeT())), _length};
    }

    /// Swap two ranges.
    friend void swap(IntegerUnitRange &first, IntegerUnitRange &second) noexcept {
        std::swap(first._index, second._index);
        std::swap(first._length, second._length);
    }

public: // iteration
    /// Call a function for each index in this range.
    /// Raw index values are incremented internally, avoiding saturation checks for every step.
    /// @param loopFn The function to call with each index. It may return `void` or `util::LoopStatus`.
    /// @return The result of the loop.
    template <typename Fn>
    auto forEach(Fn &&loopFn) const -> util::LoopResult {
        if (!isValid()) {
            return util::LoopResult::Error;
        }
        if (_length.isZero()) {
            return util::LoopResult::Success;
        }
        const auto end = endIndex().toRawValue();
        for (auto index = _index.toRawValue(); index < end; ++index) {
            const auto status = util::impl::invokeLoopFunction(loopFn, Index{index});
            if (status != util::LoopStatus::Continue) {
                return util::impl::loopStatusToResult(status);
            }
        }
        return util::LoopResult::Success;
    }

public: // factory methods
    /// Return an empty range starting at zero.
    [[nodiscard]] constexpr static auto empty() noexcept -> IntegerUnitRange {
        return IntegerUnitRange{Index::zero(), Length::zero()};
    }
    /// Return an empty range starting at the given index.
    [[nodiscard]] constexpr static auto emptyAt(Index index) noexcept -> IntegerUnitRange {
        return IntegerUnitRange{index, Length::zero()};
    }
    /// Return a range starting at zero with infinite length.
    [[nodiscard]] constexpr static auto all() noexcept -> IntegerUnitRange {
        return IntegerUnitRange{Index::zero(), Length::infinite()};
    }
    /// Return an invalid or absent range.
    [[nodiscard]] constexpr static auto noRange() noexcept -> IntegerUnitRange {
        return IntegerUnitRange{Index::noIndex(), Length::zero()};
    }
    /// Return a range starting at zero with a given length.
    /// @param length The length to use for the range.
    [[nodiscard]] constexpr static auto fromLength(const Length length) noexcept -> IntegerUnitRange {
        return IntegerUnitRange{Index::zero(), length};
    }
    /// Return a range starting at zero with a length from std::size_t.
    /// The length saturates at the maximum value of Length.
    /// @param length The length to use for the range.
    [[nodiscard]] constexpr static auto fromSizeT(const std::size_t length) noexcept -> IntegerUnitRange {
        return IntegerUnitRange{Index::zero(), Length::fromSizeT(length)};
    }
    /// Return a range starting at zero with a length from std::size_t.
    /// @param length The length to use for the range.
    /// @throws err::OverflowError if the length is too large to represent.
    [[nodiscard]] constexpr static auto fromSizeTOrThrow(const std::size_t length) -> IntegerUnitRange {
        return IntegerUnitRange{Index::zero(), Length::fromSizeTOrThrow(length)};
    }

private:
    Index _index{};   ///< The first index in this range.
    Length _length{}; ///< The number of elements in this range.
};

}

template <erbsland::unit::impl::ValidIntegerUnit tIntegerUnit>
struct std::hash<erbsland::unit::IntegerUnitRange<tIntegerUnit>> {
    auto operator()(const erbsland::unit::IntegerUnitRange<tIntegerUnit> &value) const noexcept -> std::size_t {
        return erbsland::util::createHash(value.index(), value.length());
    }
};
