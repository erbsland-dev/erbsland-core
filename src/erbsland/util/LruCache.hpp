// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/OrderedCache.hpp"

#include "../unit/ItemCount.hpp"

#include <functional>
#include <optional>
#include <utility>

namespace erbsland::util {

/// A bounded cache evicting the least recently used entry.
/// Instances own independent storage and require external synchronization when shared between threads.
/// @seedoc{/reference/util/utilities}
/// @tparam tKey The copyable key type.
/// @tparam tValue The copyable value type.
/// @tparam tHash The default-constructible key hash type.
/// @tparam tEqual The default-constructible key equality type.
/// @tested{LruCacheTest}
template <typename tKey, typename tValue, typename tHash = std::hash<tKey>, typename tEqual = std::equal_to<tKey>>
class LruCache final {
public:
    /// Create an empty cache with a finite capacity.
    /// @param capacity Maximum retained entries; zero disables retention.
    explicit LruCache(unit::ItemCount capacity) : _storage{capacity} {}

    // defaults
    ~LruCache() = default;
    LruCache(const LruCache &) = default;
    LruCache(LruCache &&) noexcept = default;
    auto operator=(const LruCache &) -> LruCache & = default;
    auto operator=(LruCache &&) noexcept -> LruCache & = default;

public: // access
    /// Test whether no entries are retained.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return count().isZero(); }
    /// Test membership without changing eviction order.
    /// @param key Key to find.
    /// @return Whether the key is retained.
    [[nodiscard]] auto contains(const tKey &key) const -> bool { return _storage.contains(key); }
    /// Return the maximum retained entry count.
    [[nodiscard]] auto capacity() const noexcept -> unit::ItemCount { return _storage.capacity(); }
    /// Return the retained entry count.
    [[nodiscard]] auto count() const noexcept -> unit::ItemCount { return _storage.count(); }
    /// Copy a value and refresh its recency.
    /// @param key Key to find.
    /// @return The value, or an empty optional when absent.
    [[nodiscard]] auto get(const tKey &key) -> std::optional<tValue> { return _storage.get(key); }
    /// Copy a value without changing eviction order.
    /// @param key Key to find.
    /// @return The value, or an empty optional when absent.
    [[nodiscard]] auto peek(const tKey &key) const -> std::optional<tValue> { return _storage.peek(key); }

public: // changes
    /// Change the finite capacity, immediately evicting excess entries.
    /// @param capacity Maximum retained entries; zero clears and disables retention.
    /// @return This cache.
    auto setCapacity(unit::ItemCount capacity) -> LruCache & {
        _storage.setCapacity(capacity);
        return *this;
    }
    /// Insert or replace a value; replacement refreshes recency.
    /// @param key Key to retain.
    /// @param value Value to retain.
    /// @return This cache.
    auto set(tKey key, tValue value) -> LruCache & {
        _storage.set(std::move(key), std::move(value), true);
        return *this;
    }
    /// Remove a key if present.
    /// @param key Key to remove.
    /// @return This cache.
    auto remove(const tKey &key) -> LruCache & {
        _storage.remove(key);
        return *this;
    }
    /// Remove all entries, retaining the configured capacity.
    /// @return This cache.
    auto clear() noexcept -> LruCache & {
        _storage.clear();
        return *this;
    }

private:
    impl::OrderedCache<tKey, tValue, tHash, tEqual> _storage; ///< Ordered entry storage.
};

}
