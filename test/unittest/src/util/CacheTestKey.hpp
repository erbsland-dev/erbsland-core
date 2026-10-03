// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <stdexcept>

/// Key whose index copy can fail after its list entry has been constructed.
struct CacheTestKey {
    int value{};        ///< Identity shared by equality and hashing.
    bool throwOnCopy{}; ///< Whether copying the key fails.
    /// Create a key with optional copy failure.
    /// @param value Key identity.
    /// @param throwOnCopy Whether to fail when copied into an index.
    CacheTestKey(int value, bool throwOnCopy = false) : value{value}, throwOnCopy{throwOnCopy} {}
    /// Copy a key, failing when requested.
    /// @param other Key to copy.
    /// @throws std::runtime_error If copy failure is enabled.
    CacheTestKey(const CacheTestKey &other) : value{other.value}, throwOnCopy{other.throwOnCopy} {
        if (throwOnCopy) {
            throw std::runtime_error{"key copy"};
        }
    }
    // defaults
    CacheTestKey(CacheTestKey &&) noexcept = default;
    auto operator=(const CacheTestKey &) -> CacheTestKey & = default;
    auto operator=(CacheTestKey &&) noexcept -> CacheTestKey & = default;
    auto operator==(const CacheTestKey &other) const noexcept -> bool { return value == other.value; }
    /// Force collisions to exercise equality and insertion rollback.
    struct Hash {
        /// Hash any key into the same bucket.
        /// @return A constant hash value.
        auto operator()(const CacheTestKey &) const noexcept -> std::size_t { return 0; }
    };
};
