// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PatternNodeArena.hpp"

#include <cstddef>
#include <utility>

namespace erbsland::re::impl {

/// Allocator that keeps the pattern-node arena alive through the shared-pointer control block.
template <typename T>
class PatternNodeAllocator final {
public:
    using value_type = T;

    /// Create an allocator retaining a pattern-node arena.
    /// @param arena The arena that backs allocated nodes.
    explicit PatternNodeAllocator(PatternNodeArenaPtr arena) noexcept : _arena{std::move(arena)} {}

    /// Copy an allocator while retaining its arena.
    /// @tparam U The source allocator value type.
    /// @param other The allocator whose arena is retained.
    template <typename U>
    PatternNodeAllocator(const PatternNodeAllocator<U> &other) noexcept : _arena{other.arena()} {}

    /// Allocate pattern-node storage from the retained arena.
    [[nodiscard]] auto allocate(const std::size_t count) -> T * {
        return static_cast<T *>(_arena->resource()->allocate(sizeof(T) * count, alignof(T)));
    }

    /// Return pattern-node storage to the arena resource.
    void deallocate(T *data, const std::size_t count) noexcept {
        _arena->resource()->deallocate(data, sizeof(T) * count, alignof(T));
    }

    /// Access the arena retained by this allocator.
    [[nodiscard]] auto arena() const noexcept -> const PatternNodeArenaPtr & { return _arena; }

    /// Test whether two allocators retain the same arena.
    /// @tparam U The other allocator value type.
    /// @param other The allocator to compare.
    template <typename U>
    [[nodiscard]] auto operator==(const PatternNodeAllocator<U> &other) const noexcept -> bool {
        return _arena == other.arena();
    }

private:
    PatternNodeArenaPtr _arena;
};

}
