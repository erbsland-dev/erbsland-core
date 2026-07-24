// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstddef>
#include <functional>

namespace erbsland::util {

/// Combines two hash values into a single hash value.
///
/// This function uses a common hash combination algorithm (often attributed to Boost)
/// to combine two `std::size_t` hash values into one.
/// @param hash1 The first hash value.
/// @param hash2 The second hash value.
/// @return The combined hash value.
[[nodiscard]] constexpr auto combineHash(const std::size_t hash1, const std::size_t hash2) noexcept -> std::size_t {
    return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
}

/// Advances an existing hash value by hashing a new argument and combining it.
///
/// This function hashes the provided argument using `std::hash` and then combines
/// it with the current hash value using `combineHash`.
/// @tparam T The type of the argument to hash.
/// @param hash The current hash value, which will be updated.
/// @param arg The argument to be hashed and combined.
template <typename T>
void advanceHash(std::size_t &hash, const T &arg) noexcept {
    hash = combineHash(hash, std::hash<T>{}(arg));
}

/// Creates a new hash value from one or more arguments.
///
/// Combines hashes of all arguments into a single value.
/// @seedoc{/reference/util/supporting_utilities}
/// @tparam T1 The type of the first argument.
/// @tparam Rest The types of the remaining arguments.
/// @param arg1 The first argument.
/// @param rest The remaining arguments.
/// @return The combined hash value of all arguments.
template <typename T1, typename... Rest>
[[nodiscard]] auto createHash(const T1 &arg1, const Rest &...rest) noexcept -> std::size_t {
    std::size_t hash = 0;
    advanceHash(hash, arg1);
    if constexpr (sizeof...(rest) > 0) {
        (advanceHash(hash, rest), ...);
    }
    return hash;
}

}
