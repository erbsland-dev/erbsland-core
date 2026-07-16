// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "List_fwd.hpp"
#include "LoopResult.hpp"

#include "../mem/CowManualStorage.hpp"
#include "../unit/ElementCount.hpp"
#include "../unit/ElementIndex.hpp"
#include "../unit/ElementRange.hpp"

#include <compare>
#include <concepts>
#include <initializer_list>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace erbsland::util {

/// A copy-on-write list container with Erbsland-style element access and algorithms.
/// @seedoc{/reference/util/collections}
/// @tparam tElement The element type stored in this list.
/// @tparam tSelf Internal CRTP type used by derived public list types.
/// @tested{ListTest}
template <typename tElement, typename tSelf>
class List {
public:
    using Element = tElement;                                                     ///< The stored element type.
    using Raw = std::vector<Element>;                                             ///< The wrapped standard container.
    using Storage = mem::CowManualStorage<Raw>;                                   ///< The COW storage type.
    using Index = unit::ElementIndex;                                             ///< The element index type.
    using Count = unit::ElementCount;                                             ///< The element count type.
    using Range = unit::ElementRange;                                             ///< The element range type.
    using Self = std::conditional_t<std::is_void_v<tSelf>, List<Element>, tSelf>; ///< The fluent return type.
    using value_type = Element;                                                   ///< Standard container value type.
    using const_iterator = Raw::const_iterator;                                   ///< Standard const iterator type.

public:
    /// Create an empty list.
    List();
    /// Create a list from an initializer list.
    /// @param values The elements to copy.
    explicit List(std::initializer_list<Element> values);
    /// Create a list with a single element.
    /// @param value The element to store.
    explicit List(Element value);
    /// Create a list with repeated elements.
    /// @param count The number of elements.
    /// @param value The element value to repeat.
    List(Count count, const Element &value);
    /// Create a list from a standard vector.
    /// @param raw The vector to copy.
    explicit List(const Raw &raw);
    /// Create a list from a standard vector, taking ownership.
    /// @param raw The vector to move.
    explicit List(Raw &&raw);

    // defaults
    ~List() = default;
    List(const List &) noexcept = default;
    List(List &&) noexcept = default;
    auto operator=(const List &) noexcept -> List & = default;
    auto operator=(List &&) noexcept -> List & = default;

public: // operators
    /// Concatenate two lists.
    /// @param other The list to append.
    /// @return A new list containing all elements.
    [[nodiscard]] auto operator+(const Self &other) const -> Self;
    /// Append an element to a list.
    /// @param value The element to append.
    /// @return A new list with the element added.
    [[nodiscard]] auto operator+(const Element &value) const -> Self;
    /// Append an element to a list, taking ownership of the value.
    /// @param value The element to append.
    /// @return A new list with the element added.
    [[nodiscard]] auto operator+(Element &&value) const -> Self;
    /// Append another list to this list.
    /// @param other The list to append.
    /// @return A reference to this list.
    auto operator+=(const Self &other) -> Self &;
    /// Append an element to this list.
    /// @param value The element to append.
    /// @return A reference to this list.
    auto operator+=(const Element &value) -> Self &;
    /// Append an element to this list, taking ownership of the value.
    /// @param value The element to append.
    /// @return A reference to this list.
    auto operator+=(Element &&value) -> Self &;
    /// Access an element by index.
    /// @param index The element index.
    /// @return A copy of the element at the given index.
    [[nodiscard]] auto operator[](Index index) const -> Element;
    /// Compare two lists lexicographically.
    /// @param other The list to compare with.
    /// @return A three-way comparison result.
    [[nodiscard]] auto operator<=>(const Self &other) const -> std::strong_ordering;
    /// Test two lists for equality.
    /// @param other The list to compare with.
    /// @return `true` if both lists have the same elements in the same order.
    [[nodiscard]] auto operator==(const Self &other) const -> bool;
    /// Prepend an element to a list.
    /// @param value The element to prepend.
    /// @param list The list to prepend to.
    /// @return A new list with the element added at the front.
    friend auto operator+(const Element &value, const Self &list) -> Self {
        auto result = Self{value};
        result.append(list);
        return result;
    }
    /// Prepend an element to a list, taking ownership of the value.
    /// @param value The element to prepend.
    /// @param list The list to prepend to.
    /// @return A new list with the element added at the front.
    friend auto operator+(Element &&value, const Self &list) -> Self {
        auto result = Self{};
        result.append(std::move(value));
        result.append(list);
        return result;
    }

public: // conversion
    /// Return a reference to the underlying standard vector.
    /// @return The wrapped vector.
    [[nodiscard]] auto toRawValue() const noexcept -> const Raw &;
    /// Convert to a standard vector.
    /// @return A copy of the list as a `std::vector`.
    [[nodiscard]] auto toStdVector() const -> Raw;
    /// Convert to a standard set.
    /// @return A copy of the list as a `std::set`.
    [[nodiscard]] auto toStdSet() const -> std::set<Element>;

public: // elements
    /// Return the number of elements.
    /// @return The element count.
    [[nodiscard]] auto count() const noexcept -> Count;
    /// Count all elements matching a predicate.
    /// @param function The predicate called for every element.
    /// @return The number of matching elements.
    template <typename Function>
    [[nodiscard]] auto countIf(Function function) const -> Count;
    /// Get an element by index.
    /// @param index The element index.
    /// @return A copy of the element.
    [[nodiscard]] auto get(Index index) const -> Element;
    /// Get an element by index, with a default value.
    /// @param index The element index.
    /// @param defaultValue The value to return if the index is out of range.
    /// @return The element, or the default value.
    [[nodiscard]] auto get(Index index, const Element &defaultValue) const -> Element;
    /// Get the first element.
    /// @return A copy of the first element.
    [[nodiscard]] auto first() const -> Element;
    /// Get the last element.
    /// @return A copy of the last element.
    [[nodiscard]] auto last() const -> Element;
    /// Set an element at a given index.
    /// @param index The element index.
    /// @param value The new element value.
    /// @return A reference to this list.
    auto set(Index index, const Element &value) -> Self &;
    /// Set an element at a given index, taking ownership of the value.
    /// @param index The element index.
    /// @param value The new element value.
    /// @return A reference to this list.
    auto set(Index index, Element &&value) -> Self &;

public: // memory
    /// Resize the list.
    /// @param count The new element count.
    /// @return A reference to this list.
    auto resize(Count count) -> Self &;
    /// Resize the list, filling new elements with a value.
    /// @param count The new element count.
    /// @param value The value for new elements.
    /// @return A reference to this list.
    auto resize(Count count, const Element &value) -> Self &;
    /// Reserve capacity.
    /// @param count The minimum capacity to reserve.
    /// @return A reference to this list.
    auto reserve(Count count) -> Self &;
    /// Return the current capacity.
    /// @return The current capacity.
    [[nodiscard]] auto capacity() const noexcept -> Count;
    /// Shrink the underlying storage to fit.
    /// @return A reference to this list.
    auto shrinkToFit() -> Self &;
    /// Remove all elements.
    /// @return A reference to this list.
    auto clear() -> Self &;
    /// Swap with another list.
    /// @param other The list to swap with.
    /// @return A reference to this list.
    auto swap(Self &other) noexcept -> Self &;

public: // slice
    /// Split the list into first element and the rest.
    /// @return A pair of the first element and the remaining list.
    [[nodiscard]] auto sliceFirst() const -> std::pair<Element, Self>;
    /// Split the list into the last element and the rest.
    /// @return A pair of the last element and the remaining list.
    [[nodiscard]] auto sliceLast() const -> std::pair<Element, Self>;
    /// Extract a sub-list from a range.
    /// @param range The element range to extract.
    /// @return A new list containing the sliced elements.
    [[nodiscard]] auto slice(Range range) const -> Self;
    /// Get the first `count` elements.
    /// @param count The number of elements to take.
    /// @return A new list with the prefix.
    [[nodiscard]] auto prefix(Count count) const -> Self;
    /// Get the last `count` elements.
    /// @param count The number of elements to take.
    /// @return A new list with the suffix.
    [[nodiscard]] auto suffix(Count count) const -> Self;

public: // remove
    /// Remove an element at a given index.
    /// @param index The index to remove.
    /// @return A reference to this list.
    auto remove(Index index) -> Self &;
    /// Remove elements in a range.
    /// @param range The element range to remove.
    /// @return A reference to this list.
    auto remove(Range range) -> Self &;
    /// Remove all elements matching a predicate.
    /// @param function The predicate called for every element.
    /// @return A reference to this list.
    template <typename Function>
    auto removeIf(Function function) -> Self &;
    /// Return a copy with one element removed.
    /// @param index The index to remove.
    /// @return A new list without the element.
    [[nodiscard]] auto removed(Index index) const -> Self;
    /// Return a copy with a range removed.
    /// @param range The element range to remove.
    /// @return A new list without the range.
    [[nodiscard]] auto removed(Range range) const -> Self;
    /// Return a copy without elements matching a predicate.
    /// @param function The predicate called for every element.
    /// @return A new list without matching elements.
    template <typename Function>
    [[nodiscard]] auto removedIf(Function function) const -> Self;
    /// Remove the first element.
    /// @return A reference to this list.
    auto removeFirst() -> Self &;
    /// Remove the last element.
    /// @return A reference to this list.
    auto removeLast() -> Self &;

public: // take
    /// Take an element at a given index.
    /// @param index The index to take from.
    /// @return The element at the index.
    auto take(Index index) -> Element;
    /// Take all elements in a range.
    /// @param range The range to remove and return.
    /// @return A list containing the taken elements.
    auto take(Range range) -> Self;
    /// Take all elements matching a predicate.
    /// @param function The predicate called for every element.
    /// @return A list containing the taken elements.
    template <typename Function>
    auto takeIf(Function function) -> Self;
    /// Take the first element.
    /// @return The first element.
    auto takeFirst() -> Element;
    /// Take the last element.
    /// @return The last element.
    auto takeLast() -> Element;

public: // algorithms
    /// Call a function for every element.
    /// @param function The function called for every element.
    /// @return The result of the iteration.
    template <typename Function>
    auto forEach(Function function) const -> LoopResult;
    /// Call a function for every element in reverse order.
    /// @param function The function called for every element.
    /// @return The result of the iteration.
    template <typename Function>
    auto forEachReverse(Function function) const -> LoopResult;
    /// Replace every element with the result of a function.
    /// @param function The function used to map elements.
    /// @return A reference to this list.
    template <typename Function>
    auto map(Function function) -> Self &;
    /// Return a copy with every element mapped through a function.
    /// @param function The function used to map elements.
    /// @return A mapped copy of the list.
    template <typename Function>
    [[nodiscard]] auto mapped(Function function) const -> Self;
    /// Reverse the list in place.
    /// @return A reference to this list.
    auto reverse() -> Self &;
    /// Return a reversed copy.
    /// @return A new list in reverse order.
    [[nodiscard]] auto reversed() const -> Self;
    /// Collapse consecutive duplicate elements.
    /// @return A reference to this list.
    auto collapse() -> Self &;
    /// Return a copy with consecutive duplicate elements collapsed.
    /// @return A new list with duplicate runs collapsed.
    [[nodiscard]] auto collapsed() const -> Self;

public: // sort
    /// Sort the list in place.
    /// @return A reference to this list.
    auto sort() -> Self &;
    /// Sort the list in place using a comparison function.
    /// @param function The comparison function.
    /// @return A reference to this list.
    template <typename Function>
    auto sort(Function function) -> Self &;
    /// Return a sorted copy of the list.
    /// @return A new sorted list.
    [[nodiscard]] auto sorted() const -> Self;
    /// Return a sorted copy using a comparison function.
    /// @param function The comparison function.
    /// @return A new sorted list.
    template <typename Function>
    [[nodiscard]] auto sorted(Function function) const -> Self;

public: // search
    /// Find the first occurrence of a value.
    /// @param value The value to search for.
    /// @return The index of the first occurrence.
    [[nodiscard]] auto findFirst(const Element &value) const -> Index;
    /// Find the first occurrence of a value starting from an index.
    /// @param value The value to search for.
    /// @param start The index to start searching from.
    /// @return The index of the first occurrence.
    [[nodiscard]] auto findFirst(const Element &value, Index start) const -> Index;
    /// Find the first element matching a predicate.
    /// @param function The predicate called for every element.
    /// @return The index of the first matching element.
    template <typename Function>
    [[nodiscard]] auto findFirstIf(Function function) const -> Index;
    /// Find the first element matching a predicate starting from an index.
    /// @param function The predicate called for every element.
    /// @param start The index to start searching from.
    /// @return The index of the first matching element.
    template <typename Function>
    [[nodiscard]] auto findFirstIf(Function function, Index start) const -> Index;
    /// Find the last occurrence of a value.
    /// @param value The value to search for.
    /// @return The index of the last occurrence.
    [[nodiscard]] auto findLast(const Element &value) const -> Index;
    /// Find the last occurrence of a value starting backwards from an index.
    /// @param value The value to search for.
    /// @param start The index to start searching from.
    /// @return The index of the last occurrence.
    [[nodiscard]] auto findLast(const Element &value, Index start) const -> Index;
    /// Find the last element matching a predicate.
    /// @param function The predicate called for every element.
    /// @return The index of the last matching element.
    template <typename Function>
    [[nodiscard]] auto findLastIf(Function function) const -> Index;
    /// Find the last element matching a predicate starting backwards from an index.
    /// @param function The predicate called for every element.
    /// @param start The index to start searching from.
    /// @return The index of the last matching element.
    template <typename Function>
    [[nodiscard]] auto findLastIf(Function function, Index start) const -> Index;

public: // change
    /// Insert an element at a given index.
    /// @param index The index to insert at.
    /// @param value The element to insert.
    /// @return A reference to this list.
    auto insert(Index index, const Element &value) -> Self &;
    /// Insert an element at a given index, taking ownership of the value.
    /// @param index The index to insert at.
    /// @param value The element to insert.
    /// @return A reference to this list.
    auto insert(Index index, Element &&value) -> Self &;
    /// Insert another list at a given index.
    /// @param index The index to insert at.
    /// @param other The list to insert.
    /// @return A reference to this list.
    auto insert(Index index, const Self &other) -> Self &;
    /// Append an element.
    /// @param value The element to append.
    /// @return A reference to this list.
    auto append(const Element &value) -> Self &;
    /// Append an element, taking ownership of the value.
    /// @param value The element to append.
    /// @return A reference to this list.
    auto append(Element &&value) -> Self &;
    /// Append another list.
    /// @param other The list to append.
    /// @return A reference to this list.
    auto append(const Self &other) -> Self &;
    /// Prepend an element.
    /// @param value The element to prepend.
    /// @return A reference to this list.
    auto prepend(const Element &value) -> Self &;
    /// Prepend an element, taking ownership of the value.
    /// @param value The element to prepend.
    /// @return A reference to this list.
    auto prepend(Element &&value) -> Self &;
    /// Prepend another list.
    /// @param other The list to prepend.
    /// @return A reference to this list.
    auto prepend(const Self &other) -> Self &;

public: // tests
    /// Test if this list is empty.
    /// @return `true` if the list has no elements.
    [[nodiscard]] auto isEmpty() const -> bool;
    /// Compare two lists lexicographically.
    /// @param other The list to compare with.
    /// @return A three-way comparison result.
    [[nodiscard]] auto compare(const Self &other) const -> std::strong_ordering;
    /// Test if the list contains a value.
    /// @param value The value to search for.
    /// @return `true` if the value is present.
    [[nodiscard]] auto contains(const Element &value) const -> bool;
    /// Test if all elements match a predicate.
    /// @param function The predicate called for every element.
    /// @return `true` if all elements match.
    template <typename Function>
    [[nodiscard]] auto allOf(Function function) const -> bool;
    /// Test if any element matches a predicate.
    /// @param function The predicate called for every element.
    /// @return `true` if at least one element matches.
    template <typename Function>
    [[nodiscard]] auto anyOf(Function function) const -> bool;
    /// Test if no element matches a predicate.
    /// @param function The predicate called for every element.
    /// @return `true` if no element matches.
    template <typename Function>
    [[nodiscard]] auto noneOf(Function function) const -> bool;

public: // minimal std-library compatibility
    /// Return an iterator to the first element.
    /// @return The begin iterator.
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// Return an iterator past the last element.
    /// @return The end iterator.
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    /// Swap two lists.
    /// @param first The first list.
    /// @param second The second list.
    friend void swap(List &first, List &second) noexcept { first._storage.swap(second._storage); }

protected:
    /// Return the shared raw container.
    /// @return The wrapped vector.
    [[nodiscard]] auto raw() const noexcept -> const Raw &;
    /// Return a mutable raw container, detaching storage if needed.
    /// @return The detached vector.
    [[nodiscard]] auto mutableRaw() -> Raw &;
    /// Return this object as the fluent self type.
    /// @return A mutable reference to this list as `Self`.
    [[nodiscard]] auto self() noexcept -> Self &;
    /// Return this object as the fluent self type.
    /// @return A const reference to this list as `Self`.
    [[nodiscard]] auto self() const noexcept -> const Self &;
    /// Create the fluent self type from raw storage.
    /// @param raw The raw vector to wrap.
    /// @return A new list object.
    [[nodiscard]] static auto makeSelf(Raw raw) -> Self;

private:
    [[nodiscard]] static auto countToSize(Count count) -> std::size_t;
    [[nodiscard]] static auto validIndex(Index index, std::size_t size) noexcept -> bool;
    [[nodiscard]] static auto clampedRange(Range range, Count bounds) noexcept -> Range;

private:
    Storage _storage;
};

}

#include "List_algorithm.tpp"
#include "List_change.tpp"
#include "List_construction.tpp"
#include "List_elements.tpp"
#include "List_memory.tpp"
#include "List_remove.tpp"
#include "List_search.tpp"
#include "List_slice.tpp"
#include "List_sort.tpp"
#include "List_take.tpp"
#include "List_tests.tpp"
