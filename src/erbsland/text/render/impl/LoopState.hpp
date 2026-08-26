// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LoopState_fwd.hpp"

#include "../Value.hpp"

namespace erbsland::text::render::impl {

/// Private state for one active list or ordered-map iteration.
/// @tested{RenderIterationTest RenderProgramTest}
class LoopState final {
public:
    /// Create state for one resolved list or map value.
    explicit LoopState(Value iterable);

    // defaults
    ~LoopState() = default;
    LoopState(const LoopState &) = default;
    LoopState(LoopState &&) noexcept = default;
    auto operator=(const LoopState &) -> LoopState & = default;
    auto operator=(LoopState &&) noexcept -> LoopState & = default;

public:
    /// Advance to the next item, returning false after the final item.
    [[nodiscard]] auto advance() -> bool;
    /// Test if this state traverses a list.
    [[nodiscard]] auto isList() const noexcept -> bool { return _iterable.isList(); }
    /// Test if this state traverses a map.
    [[nodiscard]] auto isMap() const noexcept -> bool { return _iterable.isMap(); }
    /// Get the current list item.
    [[nodiscard]] auto listValue() const -> Value;
    /// Get the current map key.
    [[nodiscard]] auto mapKey() const -> String;
    /// Get the current map value.
    [[nodiscard]] auto mapValue() const -> Value;
    /// Build the public loop metadata value for the current item.
    [[nodiscard]] auto metadata() const -> Value;
    /// Test whether an execution scope is active for the current item.
    [[nodiscard]] auto hasActiveScope() const noexcept -> bool { return _hasActiveScope; }
    /// Test whether at least one item was produced.
    [[nodiscard]] auto hadItems() const noexcept -> bool { return _started && !_iterable.itemCount().isZero(); }
    /// Mark whether an execution scope is active for the current item.
    void setActiveScope(const bool active) noexcept { _hasActiveScope = active; }

private:
    Value _iterable;                         ///< Retained immutable collection.
    ValueMap::const_iterator _mapIterator{}; ///< Current ordered-map iterator.
    std::size_t _index{};                    ///< Zero-based current index.
    bool _started{false};                    ///< Whether the first advance occurred.
    bool _hasActiveScope{false};             ///< Whether the engine installed this item's scope.
};

}
