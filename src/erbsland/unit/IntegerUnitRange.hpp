// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitCount.hpp"
#include "IntegerUnitIndex.hpp"
#include "IntegerUnitOffset.hpp"
#include "IntegerUnitRange_fwd.hpp"

#include "impl/TypeTraits.hpp"

#include "../util/HashHelper.hpp"
#include "../util/impl/ComparisonHelper.hpp"
#include "../util/impl/LoopControl.hpp"
#include "../util/LoopResult.hpp"
#include "../util/LoopStatus.hpp"

#include <algorithm>
#include <compare>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace erbsland::unit {

/// A range with a unit tag.
/// @seedoc{/reference/unit/units_and_versions}
/// @tparam tIntegerUnit The unit tag for this range.
/// @tested{IntegerUnitTest IntegerUnitRemainingTest LastIndexTest}
template <impl::ValidIntegerUnit tIntegerUnit>
class IntegerUnitRange {
public:
    /// The unit type.
    using Unit = tIntegerUnit;
    /// The matching index type.
    using Index = IntegerUnitIndex<tIntegerUnit>;
    /// The matching count type.
    using Count = IntegerUnitCount<tIntegerUnit>;
    /// The matching offset type.
    using Offset = IntegerUnitOffset<tIntegerUnit>;

public:
    /// Create an empty range starting at zero.
    constexpr IntegerUnitRange() noexcept = default;
    /// Create a range from a start index and length.
    constexpr IntegerUnitRange(Index index, Count length) noexcept : _index{index}, _length{length} {}
    /// Create a range from a beginning and end index.
    /// Reversed finite endpoints produce an empty range at begin. An absent begin produces noRange().
    /// If the end index is "no index", the length of the range is infinite.
    /// The end index is considered one *after* the last unit.
    /// @param begin The index that marks the first element.
    /// @param end The index after the last element, or no-index for infinite length.
    constexpr IntegerUnitRange(Index begin, Index end) noexcept : _index{begin} {
        if (begin.isNoIndex()) {
            return;
        }
        if (end.isNoIndex()) {
            _length = Count::infinite();
        } else if (end >= begin) {
            _length = Count{end.toRawValue() - begin.toRawValue(), typename Count::Unchecked{}};
        }
    }

    // defaults
    ~IntegerUnitRange() = default;
    IntegerUnitRange(const IntegerUnitRange &) noexcept = default;
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
        if (offset.isMinimum()) {
            return advanced(offset.absoluteLength());
        }
        return moved(offset.negated());
    }
    /// Move this range by the negated offset with saturation.
    auto operator-=(const Offset offset) noexcept -> IntegerUnitRange & {
        *this = *this - offset;
        return *this;
    }

public: // tests
    /// Test whether this range is present and its finite exclusive endpoint is representable.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool {
        return _index.isValid() &&
            (_length.isInfinite() || _length.toRawValue() <= Index::cRawMaximum - _index.toRawValue());
    }
    /// Test whether the beginning is absent.
    [[nodiscard]] constexpr auto isNoRange() const noexcept -> bool { return _index.isNoIndex(); }
    /// Test if this range is empty.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _length.isZero(); }
    /// Test if this range has an infinite length.
    [[nodiscard]] constexpr auto isInfinite() const noexcept -> bool { return _length.isInfinite(); }
    /// Test if this range contains the given index.
    [[nodiscard]] constexpr auto contains(const Index index) const noexcept -> bool {
        return isValid() && index.isValid() && index >= _index &&
            (isInfinite() || index.toRawValue() - _index.toRawValue() < _length.toRawValue());
    }
    /// Test whether another valid range lies wholly within this range, including empty endpoints.
    [[nodiscard]] constexpr auto contains(const IntegerUnitRange other) const noexcept -> bool {
        if (!isValid() || !other.isValid() || other._index < _index) {
            return false;
        }
        if (isInfinite()) {
            return true;
        }
        return !other.isInfinite() &&
            other._index.toRawValue() + other._length.toRawValue() <= _index.toRawValue() + _length.toRawValue();
    }
    /// Return the common range, empty at the later beginning if disjoint, or noRange() for invalid input.
    [[nodiscard]] constexpr auto intersection(const IntegerUnitRange other) const noexcept -> IntegerUnitRange {
        if (!isValid() || !other.isValid()) {
            return noRange();
        }
        const auto begin = std::max(_index, other._index);
        const auto end = std::min(endIndex(), other.endIndex());
        return fromBeginEnd(begin, end);
    }
    /// Test if this whole range fits within the given bounds length.
    [[nodiscard]] constexpr auto isWithin(const Count bounds) const noexcept -> bool {
        if (!isValid()) {
            return false;
        }
        if (bounds.isInfinite()) {
            return true;
        }
        if (isInfinite()) {
            return false;
        }
        return _index.toRawValue() + _length.toRawValue() <= bounds.toRawValue();
    }
    /// Return this range clamped to a sequence with the given bounds length.
    /// Invalid ranges stay invalid. Ranges starting beyond the bounds become empty at the end of the bounds.
    [[nodiscard]] constexpr auto clampedTo(const Count bounds) const noexcept -> IntegerUnitRange {
        if (!isValid()) {
            return noRange();
        }
        if (bounds.isInfinite()) {
            return *this;
        }
        if (!_index.isWithin(bounds)) {
            return emptyAt(Index::end(bounds));
        }

        const auto availableLength = bounds.remainingFrom(_index);
        if (_length.isInfinite() || _length > availableLength) {
            return IntegerUnitRange{_index, availableLength};
        }
        return *this;
    }

public: // accessors and modifiers
    /// Get the start index of this range.
    [[nodiscard]] constexpr auto beginIndex() const noexcept -> Index { return _index; }
    /// Get the length of this range.
    [[nodiscard]] constexpr auto length() const noexcept -> Count { return _length; }
    /// Set the start index of this range.
    constexpr void setBeginIndex(Index index) noexcept { _index = index; }
    /// Set the length of this range.
    constexpr void setLength(Count length) noexcept { _length = length; }
    /// Get the first index after this range.
    /// If the range is invalid, infinite, or exceeds the representable index space, this returns Index::noIndex().
    [[nodiscard]] constexpr auto endIndex() const noexcept -> Index {
        if (!isValid() || _length.isInfinite()) {
            return Index::noIndex();
        }
        return _index.uncheckedAdvanced(_length);
    }
    /// Get the final included index in this range's coordinate system.
    /// @return The last index, or noIndex() if the range is empty, infinite, or invalid.
    [[nodiscard]] constexpr auto lastIndex() const noexcept -> Index {
        if (!isValid() || _length.isZero() || _length.isInfinite()) {
            return Index::noIndex();
        }
        return _index.uncheckedAdvanced(_length.uncheckedSubtracted(Count::one()));
    }

public: // math
    /// Return the suffix beginning at an absolute index, clamped to this range.
    /// Invalid ranges or an absent index produce noRange(); positions beyond a finite end produce emptyAt(end).
    /// @param index The inclusive beginning of the remaining range, in this range's coordinate system.
    /// @return This range before its beginning, its suffix within it, or an empty range at its end.
    [[nodiscard]] constexpr auto remainingFrom(const Index index) const noexcept -> IntegerUnitRange {
        if (!isValid() || index.isNoIndex()) {
            return noRange();
        }
        if (index <= _index) {
            return *this;
        }
        if (isInfinite()) {
            return fromBegin(index);
        }
        const auto end = endIndex();
        if (index >= end) {
            return emptyAt(end);
        }
        return fromBeginEnd(index, end);
    }
    /// Advance the whole range with saturation, preserving its length.
    constexpr auto advance(Count length) noexcept -> IntegerUnitRange & {
        *this = advanced(length);
        return *this;
    }
    /// Return a translated range with saturation, or noRange() for invalid input.
    [[nodiscard]] constexpr auto advanced(Count length) const noexcept -> IntegerUnitRange {
        if (!isValid()) {
            return noRange();
        }
        if (length.toRawValue() > maximumBegin() - _index.toRawValue()) {
            return {Index{maximumBegin(), typename Index::Unchecked{}}, _length};
        }
        return {_index.uncheckedAdvanced(length), _length};
    }
    /// Advance the range without saturation.
    /// @throws err::OverflowError for invalid input or an unrepresentable translation.
    constexpr auto advanceOrThrow(Count length) -> IntegerUnitRange & {
        *this = advancedOrThrow(length);
        return *this;
    }
    /// Return an exactly translated range.
    /// @throws err::OverflowError for invalid input or an unrepresentable translation.
    [[nodiscard]] constexpr auto advancedOrThrow(Count length) const -> IntegerUnitRange {
        if (!isValid() || length.toRawValue() > maximumBegin() - _index.toRawValue()) {
            impl::throwOverflow("Range translation exceeds representable bounds");
        }
        return {_index.uncheckedAdvanced(length), _length};
    }
    /// Retreat the whole range with saturation, preserving its length.
    constexpr auto retreat(Count length) noexcept -> IntegerUnitRange & {
        *this = retreated(length);
        return *this;
    }
    /// Return a translated range with saturation, or noRange() for invalid input.
    [[nodiscard]] constexpr auto retreated(Count length) const noexcept -> IntegerUnitRange {
        if (!isValid()) {
            return noRange();
        }
        if (length.toRawValue() > _index.toRawValue()) {
            return {Index{0U, typename Index::Unchecked{}}, _length};
        }
        return {_index.uncheckedRetreated(length), _length};
    }
    /// Retreat the range without saturation.
    /// @throws err::OverflowError for invalid input or an unrepresentable translation.
    constexpr auto retreatOrThrow(Count length) -> IntegerUnitRange & {
        *this = retreatedOrThrow(length);
        return *this;
    }
    /// Return an exactly translated range.
    /// @throws err::OverflowError for invalid input or an unrepresentable translation.
    [[nodiscard]] constexpr auto retreatedOrThrow(Count length) const -> IntegerUnitRange {
        if (!isValid() || length.toRawValue() > _index.toRawValue()) {
            impl::throwOverflow("Range translation exceeds representable bounds");
        }
        return {_index.uncheckedRetreated(length), _length};
    }
    /// Move the whole range with saturation, preserving its length.
    constexpr auto move(Offset offset) noexcept -> IntegerUnitRange & {
        *this = moved(offset);
        return *this;
    }
    /// Return a translated range with saturation, or noRange() for invalid input.
    [[nodiscard]] constexpr auto moved(Offset offset) const noexcept -> IntegerUnitRange {
        if (offset.isNegative()) {
            return retreated(offset.absoluteLength());
        }
        return advanced(offset.absoluteLength());
    }
    /// Move the range without saturation.
    /// @throws err::OverflowError for invalid input or an unrepresentable translation.
    constexpr auto moveOrThrow(Offset offset) -> IntegerUnitRange & {
        *this = movedOrThrow(offset);
        return *this;
    }
    /// Return an exactly translated range.
    /// @throws err::OverflowError for invalid input or an unrepresentable translation.
    [[nodiscard]] constexpr auto movedOrThrow(Offset offset) const -> IntegerUnitRange {
        if (offset.isNegative()) {
            return retreatedOrThrow(offset.absoluteLength());
        }
        return advancedOrThrow(offset.absoluteLength());
    }
    /// Resolve relative coordinates at origin exactly, or return noRange() on failure.
    [[nodiscard]] constexpr auto toAbsolute(Index origin) const noexcept -> IntegerUnitRange {
        if (!isValid() || !origin.isValid() || origin.toRawValue() > maximumBegin() - _index.toRawValue()) {
            return noRange();
        }
        return {origin.uncheckedAdvanced(_index.distanceFromZero()), _length};
    }
    /// Express absolute coordinates relative to origin exactly, or return noRange() on failure.
    [[nodiscard]] constexpr auto relativeTo(Index origin) const noexcept -> IntegerUnitRange {
        if (!isValid() || !origin.isValid() || origin > _index) {
            return noRange();
        }
        return {_index.uncheckedRetreated(origin.distanceFromZero()), _length};
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
        auto end = Index::cRawNoIndex;
        if (_length.isFinite()) {
            end = _index.toRawValue() + _length.toRawValue();
        }
        for (auto index = _index.toRawValue(); index < end; ++index) {
            auto current = Index{index, typename Index::Unchecked{}};
            if constexpr (std::is_void_v<std::invoke_result_t<Fn &, Index>>) {
                std::invoke(loopFn, std::move(current));
            } else {
                const auto status = util::impl::invokeLoopFunction(loopFn, std::move(current));
                if (status != util::LoopStatus::Continue) {
                    return util::impl::loopStatusToResult(status);
                }
            }
        }
        return util::LoopResult::Success;
    }

public: // factory methods
    /// Construct an open-ended range by preserving the beginning and setting an infinite length.
    /// @param begin The inclusive beginning of the range.
    /// @return The range from begin to infinity; an absent beginning produces an absent range with infinite length.
    [[nodiscard]] constexpr static auto fromBegin(const Index begin) noexcept -> IntegerUnitRange {
        return {begin, Count::infinite()};
    }
    /// Construct a range from inclusive begin and exclusive end, using constructor semantics.
    [[nodiscard]] constexpr static auto fromBeginEnd(Index begin, Index end) noexcept -> IntegerUnitRange {
        return {begin, end};
    }
    /// Construct a range from ordered endpoints, accepting noIndex() only as an open endpoint.
    /// @throws err::OutOfRangeError if begin is absent or finite endpoints are reversed.
    [[nodiscard]] constexpr static auto fromBeginEndOrThrow(Index begin, Index end) -> IntegerUnitRange {
        if (begin.isNoIndex() || end < begin) {
            impl::throwOutOfRange("Invalid range endpoints");
        }
        return {begin, end};
    }
    /// Return an empty range starting at zero.
    [[nodiscard]] constexpr static auto empty() noexcept -> IntegerUnitRange {
        return IntegerUnitRange{Index::zero(), Count::zero()};
    }
    /// Return an empty range starting at the given index.
    [[nodiscard]] constexpr static auto emptyAt(Index index) noexcept -> IntegerUnitRange {
        return IntegerUnitRange{index, Count::zero()};
    }
    /// Return a range starting at zero with infinite length.
    [[nodiscard]] constexpr static auto all() noexcept -> IntegerUnitRange {
        return IntegerUnitRange{Index::zero(), Count::infinite()};
    }
    /// Return an invalid or absent range.
    [[nodiscard]] constexpr static auto noRange() noexcept -> IntegerUnitRange {
        return IntegerUnitRange{Index::noIndex(), Count::zero()};
    }
    /// Return a range starting at zero with a given length.
    /// @param length The length to use for the range.
    [[nodiscard]] constexpr static auto fromLength(const Count length) noexcept -> IntegerUnitRange {
        return IntegerUnitRange{Index::zero(), length};
    }
    /// Return a range starting at zero with a length from std::size_t.
    /// The length saturates at the maximum value of Count.
    /// @param length The length to use for the range.
    [[nodiscard]] constexpr static auto fromSizeT(const std::size_t length) noexcept -> IntegerUnitRange {
        return IntegerUnitRange{Index::zero(), Count::fromSizeT(length)};
    }
    /// Return a range starting at zero with a length from std::size_t.
    /// @param length The length to use for the range.
    /// @throws err::OverflowError if the length is too large to represent.
    [[nodiscard]] constexpr static auto fromSizeTOrThrow(const std::size_t length) -> IntegerUnitRange {
        return IntegerUnitRange{Index::zero(), Count::fromSizeTOrThrow(length)};
    }

private:
    /// Largest beginning preserving this range's length; requires a valid range.
    [[nodiscard]] constexpr auto maximumBegin() const noexcept -> uint64_t {
        if (isInfinite()) {
            return Index::cRawMaximum;
        }
        return Index::cRawMaximum - _length.toRawValue();
    }

    Index _index{};  ///< The first index in this range.
    Count _length{}; ///< The number of elements in this range.
};

}

template <erbsland::unit::impl::ValidIntegerUnit tIntegerUnit>
struct std::hash<erbsland::unit::IntegerUnitRange<tIntegerUnit>> {
    auto operator()(const erbsland::unit::IntegerUnitRange<tIntegerUnit> &value) const noexcept -> std::size_t {
        return erbsland::util::createHash(value.beginIndex(), value.length());
    }
};
