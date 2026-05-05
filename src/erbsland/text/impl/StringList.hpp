// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Char.hpp"
#include "../CharCompareFn.hpp"
#include "../CharSet.hpp"

#include "../../unit/ElementCount.hpp"
#include "../../unit/IntegerUnitRange.hpp"
#include "../../util/List.hpp"

#include <compare>
#include <utility>

namespace erbsland::text::impl {

/// A string-specific list container with character-comparison operations and joining.
/// @tparam tString The string or string view type stored in this list.
/// @tested{StringListTest}
template <typename tString>
class StringList : public util::List<tString, StringList<tString>> {
public:
    using Base = util::List<tString, StringList<tString>>;
    using Element = Base::Element;
    using Index = Base::Index;
    using View = Element::View;
    using Count = unit::ElementCount;
    using NativeIndex = decltype(std::declval<View>().findFirstOf(std::declval<const CharSet &>()));
    using NativeLength = typename NativeIndex::Length;
    using NativeRange = unit::IntegerUnitRange<typename NativeIndex::Unit>;

public:
    /// Inherit the standard list constructors.
    using Base::Base;
    /// Create an empty string list.
    StringList() = default;
    /// Destroy the string list.
    ~StringList() = default;
    /// Create a copy that shares storage until one copy is modified.
    StringList(const StringList &) noexcept = default;
    /// Move a string list.
    StringList(StringList &&) noexcept = default;
    /// Assign a string list, sharing storage until one copy is modified.
    auto operator=(const StringList &) noexcept -> StringList & = default;
    /// Move-assign a string list.
    auto operator=(StringList &&) noexcept -> StringList & = default;

public:
    using Base::compare;
    using Base::contains;
    using Base::findFirst;
    using Base::findLast;
    using Base::sort;
    using Base::sorted;
    /// Sort the list by decoded code point.
    /// @return A reference to this list.
    auto sort() -> StringList &;
    /// Return a sorted copy, ordered by decoded code point.
    /// @return A sorted copy of the list.
    [[nodiscard]] auto sorted() const -> StringList;
    /// Compare this list with another list, using a character comparison function for each string.
    /// @param other The list to compare with.
    /// @param compareFn The character comparison function.
    /// @return A three-way comparison result.
    [[nodiscard]] auto compare(const StringList &other, CharCompareFn compareFn) const noexcept -> std::strong_ordering;
    /// Sort the list using a character comparison function for strings.
    /// @param compareFn The character comparison function.
    /// @return A reference to this list.
    auto sort(CharCompareFn compareFn) -> StringList &;
    /// Return a sorted copy using a character comparison function for strings.
    /// @param compareFn The character comparison function.
    /// @return A sorted copy of the list.
    [[nodiscard]] auto sorted(CharCompareFn compareFn) const -> StringList;
    /// Find the first string that matches using a character comparison function.
    /// @param value The value to search for.
    /// @param compareFn The character comparison function.
    /// @return The index of the first matching value.
    [[nodiscard]] auto findFirst(const Element &value, CharCompareFn compareFn) const noexcept -> Index;
    /// Find the first string at or after `start` that matches using a character comparison function.
    /// @param value The value to search for.
    /// @param start The index to start searching from.
    /// @param compareFn The character comparison function.
    /// @return The index of the first matching value.
    [[nodiscard]] auto findFirst(const Element &value, Index start, CharCompareFn compareFn) const noexcept -> Index;
    /// Find the last string that matches using a character comparison function.
    /// @param value The value to search for.
    /// @param compareFn The character comparison function.
    /// @return The index of the last matching value.
    [[nodiscard]] auto findLast(const Element &value, CharCompareFn compareFn) const noexcept -> Index;
    /// Find the last string at or before `start` that matches using a character comparison function.
    /// @param value The value to search for.
    /// @param start The index to start searching from.
    /// @param compareFn The character comparison function.
    /// @return The index of the last matching value.
    [[nodiscard]] auto findLast(const Element &value, Index start, CharCompareFn compareFn) const noexcept -> Index;
    /// Test if a matching string exists using a character comparison function.
    /// @param value The value to search for.
    /// @param compareFn The character comparison function.
    /// @return `true` if a matching value is present.
    [[nodiscard]] auto contains(const Element &value, CharCompareFn compareFn) const noexcept -> bool;
    /// Split a string view into this list type.
    /// @param text The text to split.
    /// @param separators The separator characters.
    /// @param maximumSplits The maximum number of split points to apply.
    /// @param keepEmpty Whether empty parts are kept.
    /// @return A list with the split parts.
    [[nodiscard]] static auto fromSplit(
        const View &text, const CharSet &separators, Count maximumSplits = Count::infinite(), bool keepEmpty = false)
        -> StringList;
    /// Join all strings in this list.
    /// @param separator The separator inserted between elements.
    /// @return The joined string.
    [[nodiscard]] auto join(const View &separator = {}) const -> Element;
    /// Remove all empty strings from this list.
    /// @return A reference to this list.
    auto removeEmpty() -> StringList &;
    /// Return a copy with all empty strings removed.
    /// @return A new list without empty strings.
    [[nodiscard]] auto removedEmpty() const -> StringList;

private:
    /// Estimate the capacity for a split result.
    [[nodiscard]] static auto estimatedSplitCapacity(const View &text, const CharSet &separators, Count maximumSplits)
        -> std::size_t;
    /// Return a safe list capacity for a finite split count.
    [[nodiscard]] static auto capacityForSplitLimit(std::size_t maximumSplits) noexcept -> std::size_t;
};

}

#include "StringList.tpp"
