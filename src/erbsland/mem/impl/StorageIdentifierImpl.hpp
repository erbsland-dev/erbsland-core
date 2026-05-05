// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <cstdint>

namespace erbsland::mem::impl {

/// Create the storage identifier using two memory pointers.
/// Adding a simple, but reversible bitmask to the pointers so they don't match existing pointers in memory.
class StorageIdentifierMix final {
private:
    static constexpr auto mask1 = 0x26235704d7fc994e; // fixed random number with no meaning.
    static constexpr auto mask2 = 0x1a0ad4762b12f62f; // fixed random number with no meaning.

public:
    constexpr StorageIdentifierMix(const void *begin, const void *end) : _begin{begin}, _end{end} {}
    ~StorageIdentifierMix() = default;

    StorageIdentifierMix(const StorageIdentifierMix &) = delete;
    StorageIdentifierMix(StorageIdentifierMix &&) = delete;
    auto operator=(const StorageIdentifierMix &) -> StorageIdentifierMix & = delete;
    auto operator=(StorageIdentifierMix &&) -> StorageIdentifierMix & = delete;

public:
    [[nodiscard]] auto toValues() const noexcept -> std::array<uint64_t, 2> {
        const auto beginId = pointerToId(_begin);
        const auto endId = pointerToId(_end);
        return std::array{beginId ^ mask1, endId ^ mask2};
    }

private:
    [[nodiscard]] static auto pointerToId(const void *ptr) noexcept -> std::uint64_t {
        return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ptr));
    }

private:
    const void *_begin;
    const void *_end;
};

}
