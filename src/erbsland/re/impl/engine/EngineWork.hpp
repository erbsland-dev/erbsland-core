// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupManager.hpp"
#include "EngineThread.hpp"

#include <cstddef>
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
        _workHead = 0;
        _workList.clear();
    }

public: // work list to non-recursively resolve next threads
    /// Test if there are work tasks to handle.
    [[nodiscard]] auto hasWork() const noexcept -> bool { return _workHead < _workList.size(); }

    /// Init the work list with the start thread.
    void initWorkList(EngineThread start) {
        _workList.clear();
        _workHead = 0;
        _workList.emplace_back(std::move(start));
    }

    /// Get the current work thread.
    [[nodiscard]] auto currentWork() noexcept -> EngineThread & { return _workList[_workHead]; }

    /// End the current work thread without a match.
    void endWorkWithoutMatch(const EngineThread &work) {
        // make sure we release the capture group set for the thread.
        _captureGroupManager->release(work.captureGroupSet);
        popWork();
    }

    /// Pop a work thread if the flow ends at this position.
    void popWork() noexcept {
        _workHead += 1;
        if (_workHead >= _workList.size()) {
            // Reset to avoid unbounded growth when this helper list is used repeatedly.
            _workList.clear();
            _workHead = 0;
        }
    }

    /// Add a work thread.
    void addWork(EngineThread thread) noexcept {
        // Insert this work immediately after the current work head.
        // This preserves left-to-right priority rules of the regex engine when building the next thread list.
        const auto insertIndex = _workHead + 1;
        if (insertIndex >= _workList.size()) {
            _workList.emplace_back(std::move(thread));
        } else {
            _workList.insert(
                _workList.begin() + static_cast<EngineThreadList::difference_type>(insertIndex), std::move(thread));
        }
    }

    /// Remove threads in that reference groups in a given list.
    /// Called on STOP ATOMIC to prune the threads.
    void prune(const CaptureGroupSetReferenceList &referencesToPrune) {
        // We only prune elements after the current work head.
        // The current work itself and all elements before it are never pruned.
        const auto firstPrunedWorkIndex = _workHead + 1;

        // Test if there are work threads that need pruning.
        if (referencesToPrune.empty() || firstPrunedWorkIndex >= _workList.size()) {
            return;
        }

        // Prepare the range for pruning tests.
        const auto begin = _workList.begin() + static_cast<EngineThreadList::difference_type>(firstPrunedWorkIndex);
        const auto end = _workList.end();

        // Erase work threads with lower-priority that are affected by the references.
        _workList.erase(
            std::ranges::remove_if(
                begin,
                end,
                [&](const auto &thread) -> bool {
                    const auto doPrune = std::ranges::any_of(
                        referencesToPrune, [&](const auto &ref) -> bool { return thread.captureGroupSet == ref; });
                    if (doPrune) {
                        // make sure we release the capture group set
                        _captureGroupManager->release(thread.captureGroupSet);
                    }
                    return doPrune;
                })
                .begin(),
            end);
    }

private:
    const CaptureGroupManagerPtr &_captureGroupManager; ///< Reference to the capture group manager from state.
    EngineThreadList _workList; ///< Helper for resolving next threads (priority-ordered work list).
    std::size_t _workHead{0};   ///< Head index for the work list in `_workList`.
};

}
