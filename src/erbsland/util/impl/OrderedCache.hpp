// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../err/ParameterFailure.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ItemCount.hpp"

#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

namespace erbsland::util::impl {

/// Independently owned ordering and hash index shared by the bounded cache implementations.
/// The storage pointer keeps the linked entries and their iterator index together during moves and assignments.
/// @tparam tKey The key type.
/// @tparam tValue The copied value type.
/// @tparam tHash The hash function type.
/// @tparam tEqual The key equality function type.
/// @tested{FifoCacheTest LruCacheTest}
template <typename tKey, typename tValue, typename tHash, typename tEqual>
class OrderedCache {
    using Entries = std::list<std::pair<tKey, tValue>>;
    using Index = std::unordered_map<tKey, typename Entries::iterator, tHash, tEqual>;

    /// One inseparable entry sequence and its lookup index.
    struct State {
        /// Construct an empty index with the selected hash and equality state.
        /// @param hash Hash function state.
        /// @param equal Equality function state.
        State(tHash hash = {}, tEqual equal = {}) : index{0, std::move(hash), std::move(equal)} {}

        Entries entries; ///< Entries from oldest to newest.
        Index index;     ///< Lookup index into entries.
    };

public:
    /// Create an empty cache with a finite capacity.
    /// @param capacity Maximum retained entries; zero disables retention.
    explicit OrderedCache(unit::ItemCount capacity) : _capacity{checkedCapacity(capacity)} {}
    /// Copy entries and rebuild their independent lookup index.
    /// @param other Cache to copy.
    OrderedCache(const OrderedCache &other) : _capacity{other._capacity} {
        if (other._state) {
            _state = std::make_unique<State>(other._state->index.hash_function(), other._state->index.key_eq());
            for (const auto &entry : other._state->entries) {
                set(entry.first, entry.second, false);
            }
        }
    }
    auto operator=(const OrderedCache &other) -> OrderedCache & {
        auto copy = OrderedCache{other};
        _state.swap(copy._state);
        std::swap(_capacity, copy._capacity);
        return *this;
    }

    // defaults
    ~OrderedCache() = default;
    OrderedCache(OrderedCache &&) noexcept = default;
    auto operator=(OrderedCache &&) noexcept -> OrderedCache & = default;

public: // access
    /// Return the capacity.
    [[nodiscard]] auto capacity() const noexcept -> unit::ItemCount { return _capacity; }
    /// Return the retained entry count.
    [[nodiscard]] auto count() const noexcept -> unit::ItemCount {
        if (!_state) {
            return {};
        }
        return unit::ItemCount::fromSizeT(_state->index.size());
    }
    /// Test membership without changing ordering.
    /// @param key Key to find.
    /// @return Whether the key is retained.
    [[nodiscard]] auto contains(const tKey &key) const -> bool { return _state && _state->index.contains(key); }
    /// Copy a retained value without changing ordering.
    /// @param key Key to find.
    /// @return The value or an empty optional.
    [[nodiscard]] auto peek(const tKey &key) const -> std::optional<tValue> {
        if (_state) {
            const auto found = _state->index.find(key);
            if (found != _state->index.end()) {
                return found->second->second;
            }
        }
        return {};
    }
    /// Copy a retained value and make it newest after a successful copy.
    /// @param key Key to find.
    /// @return The value or an empty optional.
    [[nodiscard]] auto get(const tKey &key) -> std::optional<tValue> {
        if (_state) {
            const auto found = _state->index.find(key);
            if (found != _state->index.end()) {
                auto result = std::optional<tValue>{found->second->second};
                _state->entries.splice(_state->entries.end(), _state->entries, found->second);
                return result;
            }
        }
        return {};
    }

public: // changes
    /// Change capacity and evict oldest entries as needed.
    /// @param capacity Finite maximum entry count.
    void setCapacity(unit::ItemCount capacity) {
        capacity = checkedCapacity(capacity);
        while (count() > capacity) {
            evict();
        }
        _capacity = capacity;
    }
    /// Insert or replace a value, publishing only completely constructed entries.
    /// @param key Key to insert.
    /// @param value Value to retain.
    /// @param touch Whether replacement changes ordering.
    void set(tKey key, tValue value, bool touch) {
        if (_capacity.isZero()) {
            return;
        }
        if (!_state) {
            _state = std::make_unique<State>();
        }
        const auto found = _state->index.find(key);
        if (found != _state->index.end()) {
            replace(found, std::move(key), std::move(value), touch);
        } else {
            insert(std::move(key), std::move(value));
        }
    }
    /// Remove one key without affecting other entries.
    /// @param key Key to remove.
    void remove(const tKey &key) {
        if (!_state) {
            return;
        }
        const auto found = _state->index.find(key);
        if (found != _state->index.end()) {
            _state->entries.erase(found->second);
            _state->index.erase(found);
        }
    }
    /// Remove all retained entries.
    void clear() noexcept { _state.reset(); }

private:
    /// Replace a fully constructed entry while preserving or refreshing its order.
    void replace(typename Index::iterator found, tKey &&key, tValue &&value, bool touch) {
        auto &entries = _state->entries;
        const auto old = found->second;
        auto position = old;
        if (touch) {
            position = entries.end();
        }
        const auto replacement = entries.emplace(position, std::move(key), std::move(value));
        found->second = replacement;
        entries.erase(old);
    }
    /// Publish a new entry before evicting its predecessor, rolling back an incomplete index insertion.
    void insert(tKey &&key, tValue &&value) {
        auto &entries = _state->entries;
        auto &index = _state->index;
        // Locate the victim before insertion so a throwing hash cannot leave an over-capacity cache.
        auto victim = entries.end();
        if (count() == _capacity) {
            victim = entries.begin();
        }
        // Reserve first; the victim's hash-index iterator then survives insertion.
        // The private index retains its default maximum load factor of one.
        if (index.size() >= index.bucket_count()) {
            index.reserve((index.size() + 1) * 2);
        }
        auto victimIndex = index.end();
        if (victim != entries.end()) {
            victimIndex = index.find(victim->first);
        }
        entries.emplace_back(std::move(key), std::move(value));
        const auto added = std::prev(entries.end());
        try {
            index.emplace(added->first, added);
        } catch (...) {
            entries.erase(added);
            throw;
        }
        if (victim != entries.end()) {
            index.erase(victimIndex);
            entries.erase(victim);
        }
    }
    /// Validate the finite-capacity precondition.
    /// @param capacity Requested retention limit.
    /// @return The validated capacity.
    static auto checkedCapacity(unit::ItemCount capacity) -> unit::ItemCount {
        using namespace text::literals;
        if (capacity.isInfinite()) {
            throw err::ParameterFailure{"Cache capacity must be finite."_el, "capacity"_el};
        }
        return capacity;
    }
    /// Remove the oldest entry from a nonempty cache.
    void evict() {
        const auto found = _state->index.find(_state->entries.front().first);
        _state->index.erase(found);
        _state->entries.pop_front();
    }

private:
    unit::ItemCount _capacity;     ///< Finite retention limit.
    std::unique_ptr<State> _state; ///< Coherent, independently owned entry/index storage.
};

}
