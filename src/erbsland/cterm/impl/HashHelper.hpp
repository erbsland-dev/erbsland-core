// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace erbsland::cterm::impl {

/// Combine an existing hash with a new input value with 32bit precision.
/// @param hash The existing hash.
/// @param inputValue The new input value.
/// @return The combined hash.
template <typename InputValue>
    requires(std::is_integral_v<InputValue>)
[[nodiscard]] constexpr auto hashCombineInput32(const uint32_t hash, const InputValue inputValue) noexcept -> uint32_t {
    return static_cast<uint32_t>(inputValue) * 0xd8163841U + (hash << 6) + (hash >> 2);
}

/// Create a 32bit hash for one or more integral values.
/// @param inputValues The input values.
/// @return The combined hash.
template <typename... InputValues>
    requires(sizeof...(InputValues) > 0 && (std::is_integral_v<InputValues> && ...))
[[nodiscard]] constexpr auto hashCreate32(InputValues... inputValues) noexcept -> uint32_t {
    auto hash = uint32_t{0};
    ((hash = hashCombineInput32(hash, inputValues)), ...);
    hash ^= (hash >> 13);
    hash *= 0x85ebca6bU;
    hash ^= (hash >> 16);
    return hash;
}

/// Combine an existing hash with a new input value with 64bit precision.
/// @param hash The existing hash.
/// @param inputValue The new input value.
/// @return The combined hash.
template <typename InputValue>
    requires(std::is_integral_v<InputValue>)
[[nodiscard]] constexpr auto hashCombineInput64(const uint64_t hash, const InputValue inputValue) noexcept -> uint64_t {
    return static_cast<uint64_t>(inputValue) + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
}

/// Create a 64bit hash for one or more integral values.
/// @param inputValues The input values.
/// @return The combined hash.
template <typename... InputValues>
    requires(sizeof...(InputValues) > 0 && (std::is_integral_v<InputValues> && ...))
[[nodiscard]] constexpr auto hashCreate64(InputValues... inputValues) noexcept -> uint64_t {
    auto hash = uint64_t{0};
    ((hash = hashCombineInput64(hash, inputValues)), ...);
    hash ^= (hash >> 30);
    hash *= 0xbf58476d1ce4e5b9ULL;
    hash ^= (hash >> 27);
    hash *= 0x94d049bb133111ebULL;
    hash ^= (hash >> 31);
    return hash;
}

/// Create an `std::size_t` hash for one or more integral values.
/// This automatically chooses `hash32` or `hash64` depending on the platform.
template <typename... InputValues>
    requires(sizeof...(InputValues) > 0 && (std::is_integral_v<InputValues> && ...))
[[nodiscard]] constexpr auto hashCreate(InputValues... inputValues) noexcept -> std::size_t {
    if constexpr (sizeof(std::size_t) == sizeof(uint64_t)) {
        return hashCreate64(inputValues...);
    } else {
        return hashCreate32(inputValues...);
    }
}

}
