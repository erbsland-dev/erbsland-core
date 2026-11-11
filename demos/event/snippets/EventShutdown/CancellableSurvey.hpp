// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/StopToken.hpp>

#include <cstddef>

namespace demo {

/// Survey bounded units of work and report one terminal outcome after cancellation or completion.
class CancellableSurvey final {
public:
    /// Retain the dispatch loop and observe the operation's separate stop request.
    /// @param loop The open loop on which all survey callbacks run.
    /// @param stopToken The cancellation token checked before each habitat.
    CancellableSurvey(el::EventLoopPtr loop, el::StopToken stopToken);

public: // accessors
    /// Test whether cancellation ended the survey.
    [[nodiscard]] auto isCancelled() const noexcept -> bool { return _cancelled; }
    /// Get the number of completed work units.
    [[nodiscard]] auto processedCount() const noexcept -> std::size_t { return _processed; }
    /// Get the number of recorded terminal outcomes.
    [[nodiscard]] auto completionCount() const noexcept -> std::size_t { return _completions; }

public: // operations
    /// Process one habitat and schedule the next step, or record the terminal outcome.
    /// Call initially on the owner loop and retain this object until dispatch ends.
    void advance();

private:
    void finish(bool cancelled);

private:
    el::EventLoopPtr _loop;       ///< Loop that dispatches every survey step.
    el::StopToken _stopToken;     ///< Operation cancellation, independent of loop termination.
    std::size_t _processed = 0;   ///< Habitats already surveyed.
    bool _cancelled = false;      ///< Whether cancellation ended the survey.
    std::size_t _completions = 0; ///< Terminal outcomes recorded on the owner loop.
};

}
