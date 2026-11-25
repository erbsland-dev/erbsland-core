// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TaskProgressSnapshot.hpp"

#include <memory>
#include <mutex>

namespace tool::frame {

/// A copyable progress handle; default handles discard updates without locking.
class TaskProgress final {
    /// Mutable counters shared by task workers and the display timer.
    struct State {
        std::mutex mutex;              ///< Protects the phase and its counters.
        TaskProgressSnapshot snapshot; ///< Most recent progress state.
    };

public:
    // defaults
    TaskProgress() = default;

public: // progress
    /// Begin a phase after all workers from the preceding phase have completed.
    /// @param phase Short description of the current operation.
    /// @param total Known number of items, or absent for indeterminate progress.
    void beginPhase(el::String phase, std::optional<el::ItemCount> total = {}) const;
    /// Advance the current phase from any worker thread.
    /// @param count Number of newly completed items.
    void advance(el::ItemCount count = el::ItemCount{1}) const;
    /// Get one consistent snapshot for rendering.
    [[nodiscard]] auto snapshot() const -> TaskProgressSnapshot;
    /// Create an active handle shared by the runner and task workers.
    [[nodiscard]] static auto create() -> TaskProgress;

private:
    std::shared_ptr<State> _state; ///< Shared counters, absent for standalone commands.
};

}
