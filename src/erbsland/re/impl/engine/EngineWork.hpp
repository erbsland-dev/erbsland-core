// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupManager.hpp"
#include "EngineThread.hpp"

#include <ranges>
#include <utility>

namespace erbsland::re::impl {

/// Helper for resolving next threads (priority-ordered work list).
class EngineWork final {
public:
    /// Create a new instance.
    explicit EngineWork(const CaptureGroupManagerPtr &_captureGroupManager) :
        _captureGroupManager{_captureGroupManager} {}

    /// Prepare this helper for repeated use.
    void initialize() { _workList.reserve(32); }

    /// Reset this helper for the next *find first*.
    void resetForNextFind() {
        _workList.clear();
        _hasCurrentWork = false;
    }

public: // work list to non-recursively resolve next threads
    /// Test if there are work tasks to handle.
    [[nodiscard]] auto hasWork() const noexcept -> bool { return _hasCurrentWork; }

    /// Init the work list with the start thread.
    void initWorkList(EngineThread start) {
        _workList.clear();
        _currentWork = std::move(start);
        _hasCurrentWork = true;
    }

    /// Get the current work thread.
    [[nodiscard]] auto currentWork() noexcept -> EngineThread & { return _currentWork; }

    /// End the current work thread without a match.
    void endWorkWithoutMatch(const EngineThread &work) {
        // make sure we release the capture group set for the thread.
        if (_captureGroupManager) {
            _captureGroupManager->release(work.captureGroupSet);
        }
        popWork();
    }

    /// Pop a work thread if the flow ends at this position.
    void popWork() noexcept {
        if (_workList.empty()) {
            _hasCurrentWork = false;
            return;
        }
        _currentWork = std::move(_workList.back());
        _workList.pop_back();
    }

    /// Add a work thread.
    void addWork(EngineThread thread) {
        // New alternatives have priority over older pending alternatives, so a stack preserves depth-first order.
        _workList.emplace_back(std::move(thread));
    }

    /// Remove threads in that reference groups in a given list.
    /// Called on STOP ATOMIC to prune the threads.
    void prune(const CaptureGroupSetReferenceList &referencesToPrune) {
        // The current work itself is never pruned; all pending alternatives have lower priority.
        if (referencesToPrune.empty() || _workList.empty()) {
            return;
        }

        // Erase work threads with lower-priority that are affected by the references.
        _workList.erase(
            std::ranges::remove_if(
                _workList,
                [&](const auto &thread) -> bool {
                    const auto doPrune = std::ranges::any_of(
                        referencesToPrune, [&](const auto &ref) -> bool { return thread.captureGroupSet == ref; });
                    if (doPrune) {
                        // make sure we release the capture group set
                        if (_captureGroupManager) {
                            _captureGroupManager->release(thread.captureGroupSet);
                        }
                    }
                    return doPrune;
                })
                .begin(),
            _workList.end());
    }

private:
    const CaptureGroupManagerPtr &_captureGroupManager; ///< Reference to the capture group manager from state.
    EngineThread _currentWork;                          ///< The work item currently being processed.
    EngineThreadList _workList;                         ///< Pending work, with the highest-priority item at the end.
    bool _hasCurrentWork{false};                        ///< If `_currentWork` contains an active work item.
};

}
