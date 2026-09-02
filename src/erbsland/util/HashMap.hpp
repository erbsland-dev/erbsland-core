// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashSet.hpp"
#include "List.hpp"
#include "LoopResult.hpp"
#include "Set.hpp"

#include "../mem/CowManualStorage.hpp"
#include "../unit/ItemCount.hpp"

#include <concepts>
#include <functional>
#include <initializer_list>
#include <map>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace erbsland::util {

/// A copy-on-write unordered key/value container with Erbsland-style access and algorithms.
/// @seedoc{/reference/util/utilities}
/// @tparam tKey The key type.
/// @tparam tValue The value type.
/// @tparam tHash The key hash type.
/// @tparam tEqual The key equality type.
/// @tparam tSelf Internal CRTP type used by derived public hash map types.
/// @tested{HashMapTest}
template <
    typename tKey,
    typename tValue,
    typename tHash = std::hash<tKey>,
    typename tEqual = std::equal_to<tKey>,
    typename tSelf = void>
    requires std::default_initializable<tKey> && std::default_initializable<tValue> && std::copyable<tKey> &&
    std::copyable<tValue>
class HashMap {
public:
    using Key = tKey;                                        ///< The key type.
    using Value = tValue;                                    ///< The value type.
    using Entry = std::pair<Key, Value>;                     ///< One map entry.
    using Hash = tHash;                                      ///< The key hash type.
    using Equal = tEqual;                                    ///< The key equality type.
    using Raw = std::unordered_map<Key, Value, Hash, Equal>; ///< The wrapped standard container.
    using Storage = mem::CowManualStorage<Raw>;              ///< The COW storage type.
    using Count = unit::ItemCount;                           ///< The element count type.
    using Self =
        std::conditional_t<std::is_void_v<tSelf>, HashMap<Key, Value, Hash, Equal>, tSelf>; ///< The fluent return type.
    using key_type = Key;                       ///< Standard container key type.
    using mapped_type = Value;                  ///< Standard mapped value type.
    using value_type = Entry;                   ///< Standard container value type.
    using const_iterator = Raw::const_iterator; ///< Standard const iterator type.

public:
    /// Create an empty hash map.
    HashMap();
    /// Create a hash map from an initializer list.
    /// @param values The key-value pairs to insert.
    explicit HashMap(std::initializer_list<Entry> values);
    /// Create a hash map from a standard unordered map.
    /// @param raw The map to copy.
    explicit HashMap(const Raw &raw);
    /// Create a hash map from a standard unordered map, taking ownership.
    /// @param raw The map to move.
    explicit HashMap(Raw &&raw);

    // defaults
    ~HashMap() = default;
    HashMap(const HashMap &) noexcept = default;
    HashMap(HashMap &&) noexcept = default;
    auto operator=(const HashMap &) noexcept -> HashMap & = default;
    auto operator=(HashMap &&) noexcept -> HashMap & = default;

public: // conversion
    /// Return a reference to the underlying standard unordered map.
    /// @return The wrapped map.
    [[nodiscard]] auto toRawValue() const noexcept -> const Raw &;
    /// Convert to a standard ordered map.
    /// @return An ordered map with all entries.
    [[nodiscard]] auto toStdMap() const -> std::map<Key, Value>;
    /// Convert to a standard unordered map.
    /// @return A copy of the wrapped map.
    [[nodiscard]] auto toStdUnorderedMap() const -> Raw;
    /// Convert keys to a standard vector.
    /// @return A vector with all keys.
    [[nodiscard]] auto toStdKeyVector() const -> std::vector<Key>;
    /// Convert entries to a standard vector.
    /// @return A vector with all entries.
    [[nodiscard]] auto toStdVector() const -> std::vector<Entry>;
    /// Convert keys to an ordered set.
    /// @return A set with all keys.
    [[nodiscard]] auto toKeySet() const -> Set<Key>;
    /// Convert keys to a hash set.
    /// @return A hash set with all keys.
    [[nodiscard]] auto toKeyHashSet() const -> HashSet<Key, Hash, Equal>;
    /// Convert values to an ordered set.
    /// @return A set with all values.
    [[nodiscard]] auto toValueSet() const -> Set<Value>;
    /// Convert values to a hash set.
    /// @return A hash set with all values.
    [[nodiscard]] auto toValueHashSet() const -> HashSet<Value>;

public: // elements
    /// Return the number of entries.
    /// @return The entry count.
    [[nodiscard]] auto count() const noexcept -> Count;
    /// Count all entries matching a predicate.
    /// @param function The predicate called with each entry.
    /// @return The number of matching entries.
    template <typename Function>
    [[nodiscard]] auto countIf(Function function) const -> Count;
    /// Count all keys matching a predicate.
    /// @param function The predicate called with each key.
    /// @return The number of matching keys.
    template <typename Function>
    [[nodiscard]] auto countIfKey(Function function) const -> Count;
    /// Count all values matching a predicate.
    /// @param function The predicate called with each value.
    /// @return The number of matching values.
    template <typename Function>
    [[nodiscard]] auto countIfValue(Function function) const -> Count;
    /// Get a value by key.
    /// @param key The key to look up.
    /// @return The value, or `std::nullopt` if not found.
    [[nodiscard]] auto get(const Key &key) const -> std::optional<Value>;
    /// Get a value by key, with a default.
    /// @param key The key to look up.
    /// @param defaultValue The value to return if not found.
    /// @return The value, or the default.
    [[nodiscard]] auto get(const Key &key, const Value &defaultValue) const -> Value;
    /// Get an arbitrary entry.
    /// @return A copy of an entry.
    [[nodiscard]] auto first() const -> Entry;
    /// Get an arbitrary entry.
    /// @return A copy of an entry.
    [[nodiscard]] auto last() const -> Entry;
    /// Convert keys to a list.
    /// @return A list with all keys.
    [[nodiscard]] auto toKeyList() const -> List<Key>;
    /// Convert values to a list.
    /// @return A list with all values.
    [[nodiscard]] auto toValueList() const -> List<Value>;
    /// Convert entries to a list.
    /// @return A list with all entries.
    [[nodiscard]] auto toList() const -> List<Entry>;

public: // memory
    /// Reserve capacity.
    /// @param count The minimum capacity to reserve.
    /// @return A reference to this map.
    auto reserve(Count count) -> Self &;
    /// Return the current capacity.
    /// @return The current capacity.
    [[nodiscard]] auto capacity() const noexcept -> Count;
    /// Shrink the underlying storage to fit.
    /// @return A reference to this map.
    auto shrinkToFit() -> Self &;
    /// Remove all entries.
    /// @return A reference to this map.
    auto clear() -> Self &;
    /// Swap with another map.
    /// @param other The map to swap with.
    /// @return A reference to this map.
    auto swap(Self &other) noexcept -> Self &;

public: // remove
    /// Remove an entry by key.
    /// @param key The key to remove.
    /// @return A reference to this map.
    auto remove(const Key &key) -> Self &;
    /// Remove all entries matching a predicate.
    /// @param function The predicate called with each entry.
    /// @return A reference to this map.
    template <typename Function>
    auto removeIf(Function function) -> Self &;
    /// Remove all entries whose key matches a predicate.
    /// @param function The predicate called with each key.
    /// @return A reference to this map.
    template <typename Function>
    auto removeIfKey(Function function) -> Self &;
    /// Remove all entries whose value matches a predicate.
    /// @param function The predicate called with each value.
    /// @return A reference to this map.
    template <typename Function>
    auto removeIfValue(Function function) -> Self &;
    /// Return a hash map without one key.
    /// @param key The key to remove.
    /// @return A new hash map without the key.
    [[nodiscard]] auto removed(const Key &key) const -> Self;
    /// Return a hash map without entries matching a predicate.
    /// @param function The predicate called with each entry.
    /// @return A new hash map without matching entries.
    template <typename Function>
    [[nodiscard]] auto removedIf(Function function) const -> Self;
    /// Return a hash map without entries whose key matches a predicate.
    /// @param function The predicate called with each key.
    /// @return A new hash map without matching keys.
    template <typename Function>
    [[nodiscard]] auto removedIfKey(Function function) const -> Self;
    /// Return a hash map without entries whose value matches a predicate.
    /// @param function The predicate called with each value.
    /// @return A new hash map without matching values.
    template <typename Function>
    [[nodiscard]] auto removedIfValue(Function function) const -> Self;

public: // take
    /// Take a value by key.
    /// @param key The key to take.
    /// @return The value associated with the key.
    auto take(const Key &key) -> Value;
    /// Take all entries matching a predicate.
    /// @param function The predicate called with each entry.
    /// @return A hash map containing the taken entries.
    template <typename Function>
    auto takeIf(Function function) -> Self;
    /// Take all entries whose key matches a predicate.
    /// @param function The predicate called with each key.
    /// @return A hash map containing the taken entries.
    template <typename Function>
    auto takeIfKey(Function function) -> Self;
    /// Take all entries whose value matches a predicate.
    /// @param function The predicate called with each value.
    /// @return A hash map containing the taken entries.
    template <typename Function>
    auto takeIfValue(Function function) -> Self;

public: // algorithms
    /// Call a function for every entry.
    /// @param function The function called with each entry.
    /// @return The result of the iteration.
    template <typename Function>
    auto forEach(Function function) const -> LoopResult;
    /// Call a function for every key.
    /// @param function The function called with each key.
    /// @return The result of the iteration.
    template <typename Function>
    auto forEachKey(Function function) const -> LoopResult;
    /// Call a function for every value.
    /// @param function The function called with each value.
    /// @return The result of the iteration.
    template <typename Function>
    auto forEachValue(Function function) const -> LoopResult;
    /// Replace every value with the result of a function.
    /// @param function The function used to map values.
    /// @return A reference to this map.
    template <typename Function>
    auto mapValue(Function function) -> Self &;
    /// Return a copy with every value mapped through a function.
    /// @param function The function used to map values.
    /// @return A mapped copy of the map.
    template <typename Function>
    [[nodiscard]] auto mappedValues(Function function) const -> Self;

public: // change
    /// Set a key-value pair.
    /// @param key The key.
    /// @param value The value.
    /// @return A reference to this map.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    auto set(const Key &key, tValueFwd &&value) -> Self &;
    /// Set a key-value pair, taking ownership of the key.
    /// @param key The key.
    /// @param value The value.
    /// @return A reference to this map.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    auto set(Key &&key, tValueFwd &&value) -> Self &;
    /// Try to replace an existing value.
    /// @param key The key to replace.
    /// @param value The new value.
    /// @return `true` if the key existed and was replaced.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    [[nodiscard]] auto tryReplace(const Key &key, tValueFwd &&value) -> bool;
    /// Try to replace an existing value, taking ownership of the key when replaced.
    /// @param key The key to replace.
    /// @param value The new value.
    /// @return `true` if the key existed and was replaced.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    [[nodiscard]] auto tryReplace(Key &&key, tValueFwd &&value) -> bool;
    /// Try to insert a new entry.
    /// @param key The key.
    /// @param value The value.
    /// @return `true` if the key was not already present.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    [[nodiscard]] auto tryInsert(const Key &key, tValueFwd &&value) -> bool;
    /// Try to insert a new entry, taking ownership of the key when inserted.
    /// @param key The key.
    /// @param value The value.
    /// @return `true` if the key was not already present.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    [[nodiscard]] auto tryInsert(Key &&key, tValueFwd &&value) -> bool;

public: // tests
    /// Test if two maps contain the same entries.
    /// @param other The map to compare with.
    /// @return `true` if both maps have the same key-value pairs.
    [[nodiscard]] auto compare(const Self &other) const -> bool;
    /// Test if two maps contain the same keys.
    /// @param other The map to compare with.
    /// @return `true` if both maps have the same keys.
    [[nodiscard]] auto compareKeys(const Self &other) const -> bool;
    /// Test if the map contains a key.
    /// @param key The key to search for.
    /// @return `true` if the key is present.
    [[nodiscard]] auto contains(const Key &key) const -> bool;
    /// Test if all entries match a predicate.
    /// @param function The predicate called with each entry.
    /// @return `true` if all entries match.
    template <typename Function>
    [[nodiscard]] auto allOf(Function function) const -> bool;
    /// Test if any entry matches a predicate.
    /// @param function The predicate called with each entry.
    /// @return `true` if at least one entry matches.
    template <typename Function>
    [[nodiscard]] auto anyOf(Function function) const -> bool;
    /// Test if no entry matches a predicate.
    /// @param function The predicate called with each entry.
    /// @return `true` if no entry matches.
    template <typename Function>
    [[nodiscard]] auto noneOf(Function function) const -> bool;

public: // minimal std-library compatibility
    /// Return an iterator to the first entry.
    /// @return The begin iterator.
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// Return an iterator past the last entry.
    /// @return The end iterator.
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    /// Swap two hash maps.
    /// @param first The first hash map.
    /// @param second The second hash map.
    friend void swap(HashMap &first, HashMap &second) noexcept { first._storage.swap(second._storage); }

protected:
    /// Return the shared raw container.
    /// @return The wrapped unordered map.
    [[nodiscard]] auto raw() const noexcept -> const Raw &;
    /// Return a mutable raw container, detaching storage if needed.
    /// @return The detached unordered map.
    [[nodiscard]] auto mutableRaw() -> Raw &;
    /// Return this object as the fluent self type.
    /// @return A mutable reference to this hash map as `Self`.
    [[nodiscard]] auto self() noexcept -> Self &;
    /// Return this object as the fluent self type.
    /// @return A const reference to this hash map as `Self`.
    [[nodiscard]] auto self() const noexcept -> const Self &;
    /// Create the fluent self type from raw storage.
    /// @param raw The raw unordered map to wrap.
    /// @return A new hash map object.
    [[nodiscard]] static auto makeSelf(Raw raw) -> Self;

private:
    /// Create the default hash-map storage.
    [[nodiscard]] static auto defaultStorage() -> Storage;
    /// Convert an item count to a storage size.
    [[nodiscard]] static auto countToSize(Count count) -> std::size_t;

private:
    Storage _storage;
};

}

#include "HashMap_algorithm.tpp"
#include "HashMap_change.tpp"
#include "HashMap_construction.tpp"
#include "HashMap_elements.tpp"
#include "HashMap_memory.tpp"
#include "HashMap_remove.tpp"
#include "HashMap_set.tpp"
#include "HashMap_take.tpp"
#include "HashMap_tests.tpp"
