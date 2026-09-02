// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "List.hpp"
#include "LoopResult.hpp"
#include "Set_fwd.hpp"

#include "../mem/CowManualStorage.hpp"
#include "../unit/ItemCount.hpp"

#include <concepts>
#include <functional>
#include <initializer_list>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace erbsland::util {

/// A copy-on-write ordered key container with Erbsland-style access and set algorithms.
/// @seedoc{/reference/util/utilities}
/// @tparam tKey The key type.
/// @tparam tCompare The key comparison type.
/// @tparam tSelf Internal CRTP type used by derived public set types.
/// @tested{SetTest}
template <typename tKey, typename tCompare, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
class Set {
public:
    using Key = tKey;                           ///< The key type.
    using Compare = tCompare;                   ///< The key comparison type.
    using Raw = std::set<Key, Compare>;         ///< The wrapped standard container.
    using Storage = mem::CowManualStorage<Raw>; ///< The COW storage type.
    using Count = unit::ItemCount;              ///< The element count type.
    using Self = std::conditional_t<std::is_void_v<tSelf>, Set<Key, Compare>, tSelf>; ///< The fluent return type.
    using key_type = Key;                                                             ///< Standard container key type.
    using value_type = Key;                              ///< Standard container value type.
    using const_iterator = typename Raw::const_iterator; ///< Standard const iterator type.

public:
    /// Create an empty set.
    Set();
    /// Create a set from an initializer list.
    /// @param values The keys to insert.
    explicit Set(std::initializer_list<Key> values);
    /// Create a set from a standard set.
    /// @param raw The set to copy.
    explicit Set(const Raw &raw);
    /// Create a set from a standard set, taking ownership.
    /// @param raw The set to move.
    explicit Set(Raw &&raw);

    // defaults
    ~Set() = default;
    Set(const Set &) noexcept = default;
    Set(Set &&) noexcept = default;
    auto operator=(const Set &) noexcept -> Set & = default;
    auto operator=(Set &&) noexcept -> Set & = default;

public: // conversion
    /// Create a set from a list.
    /// @param values The list of keys.
    /// @return A new set with unique keys.
    [[nodiscard]] static auto fromList(const List<Key> &values) -> Self;
    /// Return a reference to the underlying standard set.
    /// @return The wrapped set.
    [[nodiscard]] auto toRawValue() const noexcept -> const Raw &;
    /// Convert to a list.
    /// @return A list with all keys.
    [[nodiscard]] auto toList() const -> List<Key>;
    /// Convert to a standard vector.
    /// @return A vector with all keys.
    [[nodiscard]] auto toStdVector() const -> std::vector<Key>;
    /// Convert to a standard set.
    /// @return A copy of the wrapped set.
    [[nodiscard]] auto toStdSet() const -> Raw;
    /// Convert to a standard unordered set.
    /// @return An unordered set with all keys.
    [[nodiscard]] auto toStdUnorderedSet() const -> std::unordered_set<Key>;

public: // elements
    /// Return the number of elements.
    /// @return The element count.
    [[nodiscard]] auto count() const noexcept -> Count;
    /// Count all keys matching a predicate.
    /// @param function The predicate called for every key.
    /// @return The number of matching keys.
    template <typename Function>
    [[nodiscard]] auto countIf(Function function) const -> Count;
    /// Get the first (smallest) element.
    /// @return A copy of the first element.
    [[nodiscard]] auto first() const -> Key;
    /// Get the last (largest) element.
    /// @return A copy of the last element.
    [[nodiscard]] auto last() const -> Key;

public: // memory
    /// Reserve capacity.
    /// @param count The minimum capacity to reserve.
    /// @return A reference to this set.
    auto reserve(Count count) -> Self &;
    /// Return the current capacity.
    /// @return The current capacity.
    [[nodiscard]] auto capacity() const noexcept -> Count;
    /// Shrink the underlying storage to fit.
    /// @return A reference to this set.
    auto shrinkToFit() -> Self &;
    /// Remove all elements.
    /// @return A reference to this set.
    auto clear() -> Self &;
    /// Swap with another set.
    /// @param other The set to swap with.
    /// @return A reference to this set.
    auto swap(Self &other) noexcept -> Self &;

public: // remove
    /// Remove a key.
    /// @param key The key to remove.
    /// @return A reference to this set.
    auto remove(const Key &key) -> Self &;
    /// Try to remove a key.
    /// @param key The key to remove.
    /// @return `true` if the key was present and removed.
    [[nodiscard]] auto tryRemove(const Key &key) -> bool;
    /// Remove all keys matching a predicate.
    /// @param function The predicate called for every key.
    /// @return A reference to this set.
    template <typename Function>
    auto removeIf(Function function) -> Self &;
    /// Return a set with a key removed.
    /// @param key The key to remove.
    /// @return A new set without the key.
    [[nodiscard]] auto removed(const Key &key) const -> Self;
    /// Return a set without keys matching a predicate.
    /// @param function The predicate called for every key.
    /// @return A new set without matching keys.
    template <typename Function>
    [[nodiscard]] auto removedIf(Function function) const -> Self;

public: // algorithms
    /// Call a function for every key.
    /// @param function The function called for every key.
    /// @return The result of the iteration.
    template <typename Function>
    auto forEach(Function function) const -> LoopResult;
    /// Call a function for every key in reverse order.
    /// @param function The function called for every key.
    /// @return The result of the iteration.
    template <typename Function>
    auto forEachReverse(Function function) const -> LoopResult;

public: // sets
    /// Unite with another set (union).
    /// @param other The set to unite with.
    /// @return A reference to this set.
    auto unite(const Self &other) -> Self &;
    /// Intersect with another set.
    /// @param other The set to intersect with.
    /// @return A reference to this set.
    auto intersect(const Self &other) -> Self &;
    /// Subtract another set (difference).
    /// @param other The set to subtract.
    /// @return A reference to this set.
    auto subtract(const Self &other) -> Self &;
    /// Compute the symmetric difference with another set.
    /// @param other The set for the symmetric difference.
    /// @return A reference to this set.
    auto symmetricDifference(const Self &other) -> Self &;
    /// Return the union of this set and another.
    /// @param other The set to unite with.
    /// @return A new set containing all elements.
    [[nodiscard]] auto unitedWith(const Self &other) const -> Self;
    /// Return the intersection of this set and another.
    /// @param other The set to intersect with.
    /// @return A new set containing common elements.
    [[nodiscard]] auto intersectedWith(const Self &other) const -> Self;
    /// Return the difference of this set from another.
    /// @param other The set to subtract.
    /// @return A new set with elements of `other` removed.
    [[nodiscard]] auto subtractedBy(const Self &other) const -> Self;
    /// Return the symmetric difference with another set.
    /// @param other The set for the symmetric difference.
    /// @return A new set with elements unique to either set.
    [[nodiscard]] auto symmetricDifferenceWith(const Self &other) const -> Self;

public: // change
    /// Insert a key.
    /// @param key The key to insert.
    /// @return A reference to this set.
    auto insert(const Key &key) -> Self &;
    /// Insert a key, taking ownership of the value.
    /// @param key The key to insert.
    /// @return A reference to this set.
    auto insert(Key &&key) -> Self &;
    /// Try to insert a key.
    /// @param key The key to insert.
    /// @return `true` if the key was not already present.
    [[nodiscard]] auto tryInsert(const Key &key) -> bool;
    /// Try to insert a key, taking ownership of the value.
    /// @param key The key to insert.
    /// @return `true` if the key was not already present.
    [[nodiscard]] auto tryInsert(Key &&key) -> bool;

public: // tests
    /// Test if two sets contain the same elements.
    /// @param other The set to compare with.
    /// @return `true` if both sets contain the same keys.
    [[nodiscard]] auto compare(const Self &other) const -> bool;
    /// Test if the set contains a key.
    /// @param key The key to search for.
    /// @return `true` if the key is present.
    [[nodiscard]] auto contains(const Key &key) const -> bool;
    /// Test if all keys match a predicate.
    /// @param function The predicate called for every key.
    /// @return `true` if all keys match.
    template <typename Function>
    [[nodiscard]] auto allOf(Function function) const -> bool;
    /// Test if any key matches a predicate.
    /// @param function The predicate called for every key.
    /// @return `true` if at least one key matches.
    template <typename Function>
    [[nodiscard]] auto anyOf(Function function) const -> bool;
    /// Test if no key matches a predicate.
    /// @param function The predicate called for every key.
    /// @return `true` if no key matches.
    template <typename Function>
    [[nodiscard]] auto noneOf(Function function) const -> bool;
    /// Test if this set is a subset of another.
    /// @param other The superset to compare with.
    /// @return `true` if all elements are in `other`.
    [[nodiscard]] auto isSubsetOf(const Self &other) const -> bool;
    /// Test if this set is a superset of another.
    /// @param other The subset to compare with.
    /// @return `true` if all elements of `other` are in this set.
    [[nodiscard]] auto isSupersetOf(const Self &other) const -> bool;
    /// Test if this set has no elements in common with another.
    /// @param other The set to compare with.
    /// @return `true` if the sets share no elements.
    [[nodiscard]] auto isDisjointWith(const Self &other) const -> bool;
    /// Test if this set shares any elements with another.
    /// @param other The set to compare with.
    /// @return `true` if there is at least one common element.
    [[nodiscard]] auto intersects(const Self &other) const -> bool;

public: // minimal std-library compatibility
    /// Return an iterator to the first key.
    /// @return The begin iterator.
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// Return an iterator past the last key.
    /// @return The end iterator.
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    /// Swap two sets.
    /// @param first The first set.
    /// @param second The second set.
    friend void swap(Set &first, Set &second) noexcept { first._storage.swap(second._storage); }

protected:
    /// Return the shared raw container.
    /// @return The wrapped set.
    [[nodiscard]] auto raw() const noexcept -> const Raw &;
    /// Return a mutable raw container, detaching storage if needed.
    /// @return The detached set.
    [[nodiscard]] auto mutableRaw() -> Raw &;
    /// Return this object as the fluent self type.
    /// @return A mutable reference to this set as `Self`.
    [[nodiscard]] auto self() noexcept -> Self &;
    /// Return this object as the fluent self type.
    /// @return A const reference to this set as `Self`.
    [[nodiscard]] auto self() const noexcept -> const Self &;
    /// Create the fluent self type from raw storage.
    /// @param raw The raw set to wrap.
    /// @return A new set object.
    [[nodiscard]] static auto makeSelf(Raw raw) -> Self;

private:
    /// Create the default set storage.
    [[nodiscard]] static auto defaultStorage() -> Storage;
    /// Convert an item count to a storage size.
    [[nodiscard]] static auto countToSize(Count count) -> std::size_t;

private:
    Storage _storage;
};

}

#include "Set_algorithm.tpp"
#include "Set_change.tpp"
#include "Set_construction.tpp"
#include "Set_elements.tpp"
#include "Set_memory.tpp"
#include "Set_remove.tpp"
#include "Set_sets.tpp"
#include "Set_tests.tpp"
