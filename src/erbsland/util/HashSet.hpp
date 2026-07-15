// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashSet_fwd.hpp"
#include "List.hpp"
#include "LoopResult.hpp"

#include "../mem/CowManualStorage.hpp"
#include "../unit/ElementCount.hpp"

#include <cmath>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace erbsland::util {

/// A copy-on-write unordered key container with Erbsland-style access and set algorithms.
/// @seedoc{/reference/util/collections}
/// @tparam tKey The key type.
/// @tparam tHash The key hash type.
/// @tparam tEqual The key equality type.
/// @tparam tSelf Internal CRTP type used by derived public hash set types.
/// @tested{HashSetTest}
template <typename tKey, typename tHash, typename tEqual, typename tSelf>
    requires std::default_initializable<tKey> && std::copyable<tKey>
class HashSet {
public:
    using Key = tKey;                                 ///< The key type.
    using Hash = tHash;                               ///< The key hash type.
    using Equal = tEqual;                             ///< The key equality type.
    using Raw = std::unordered_set<Key, Hash, Equal>; ///< The wrapped standard container.
    using Storage = mem::CowManualStorage<Raw>;       ///< The COW storage type.
    using Count = unit::ElementCount;                 ///< The element count type.
    using Self =
        std::conditional_t<std::is_void_v<tSelf>, HashSet<Key, Hash, Equal>, tSelf>; ///< The fluent return type.
    using key_type = Key;                                                            ///< Standard container key type.
    using value_type = Key;                                                          ///< Standard container value type.
    using const_iterator = typename Raw::const_iterator;                             ///< Standard const iterator type.

public:
    /// Create an empty hash set.
    HashSet();
    /// Create a hash set from an initializer list.
    /// @param values The keys to insert.
    explicit HashSet(std::initializer_list<Key> values);
    /// Create a hash set from a standard unordered set.
    /// @param raw The set to copy.
    explicit HashSet(const Raw &raw);
    /// Create a hash set from a standard unordered set, taking ownership.
    /// @param raw The set to move.
    explicit HashSet(Raw &&raw);

    // defaults
    ~HashSet() = default;
    HashSet(const HashSet &) noexcept = default;
    HashSet(HashSet &&) noexcept = default;
    auto operator=(const HashSet &) noexcept -> HashSet & = default;
    auto operator=(HashSet &&) noexcept -> HashSet & = default;

public: // conversion
    /// Create a hash set from a list.
    /// @param values The list of keys.
    /// @return A new hash set with unique keys.
    [[nodiscard]] static auto fromList(const List<Key> &values) -> Self;
    /// Return a reference to the underlying standard unordered set.
    /// @return The wrapped set.
    [[nodiscard]] auto toRawValue() const noexcept -> const Raw &;
    /// Convert to a list.
    /// @return A list with all keys.
    [[nodiscard]] auto toList() const -> List<Key>;
    /// Convert to a standard vector.
    /// @return A vector with all keys.
    [[nodiscard]] auto toStdVector() const -> std::vector<Key>;
    /// Convert to a standard set.
    /// @return An ordered set with all keys.
    [[nodiscard]] auto toStdSet() const -> std::set<Key>;
    /// Convert to a standard unordered set.
    /// @return A copy of the wrapped set.
    [[nodiscard]] auto toStdUnorderedSet() const -> Raw;

public: // elements
    /// Return the number of elements.
    /// @return The element count.
    [[nodiscard]] auto count() const noexcept -> Count;
    /// Count all keys matching a predicate.
    /// @param function The predicate called for every key.
    /// @return The number of matching keys.
    template <typename Function>
    [[nodiscard]] auto countIf(Function function) const -> Count;
    /// Get an arbitrary element.
    /// @return A copy of an element.
    [[nodiscard]] auto first() const -> Key;
    /// Get an arbitrary element.
    /// @return A copy of an element.
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
    /// Swap two hash sets.
    /// @param first The first hash set.
    /// @param second The second hash set.
    friend void swap(HashSet &first, HashSet &second) noexcept { first._storage.swap(second._storage); }

protected:
    /// Return the shared raw container.
    /// @return The wrapped unordered set.
    [[nodiscard]] auto raw() const noexcept -> const Raw &;
    /// Return a mutable raw container, detaching storage if needed.
    /// @return The detached unordered set.
    [[nodiscard]] auto mutableRaw() -> Raw &;
    /// Return this object as the fluent self type.
    /// @return A mutable reference to this hash set as `Self`.
    [[nodiscard]] auto self() noexcept -> Self &;
    /// Return this object as the fluent self type.
    /// @return A const reference to this hash set as `Self`.
    [[nodiscard]] auto self() const noexcept -> const Self &;
    /// Create the fluent self type from raw storage.
    /// @param raw The raw unordered set to wrap.
    /// @return A new hash set object.
    [[nodiscard]] static auto makeSelf(Raw raw) -> Self;

private:
    [[nodiscard]] static auto countToSize(Count count) -> std::size_t;

private:
    Storage _storage;
};

}

#include "HashSet_algorithm.tpp"
#include "HashSet_change.tpp"
#include "HashSet_construction.tpp"
#include "HashSet_elements.tpp"
#include "HashSet_memory.tpp"
#include "HashSet_remove.tpp"
#include "HashSet_sets.tpp"
#include "HashSet_tests.tpp"
