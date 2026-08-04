// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>
#include <memory_resource>

namespace erbsland::re::impl {

/// Monotonic storage used for the short-lived nodes of one parsed pattern.
/// @notest{Allocation strategy; covered by parser and compiler tests.}
class PatternNodeArena final {
public:
    /// Access the allocator resource.
    [[nodiscard]] auto resource() noexcept -> std::pmr::memory_resource * { return &_resource; }

private:
    std::pmr::monotonic_buffer_resource _resource;
};

using PatternNodeArenaPtr = std::shared_ptr<PatternNodeArena>;

}
