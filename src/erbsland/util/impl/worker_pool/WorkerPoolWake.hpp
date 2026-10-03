// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WorkerPoolState_fwd.hpp"

#include <memory>

namespace erbsland::util::impl {

/// Release mailbox admission if an owner-loop wake is discarded instead of invoked.
/// @tested{WorkerPoolEventTest}
class WorkerPoolWake final {
public:
    /// Retain only weak scheduler ownership while a wake is queued.
    /// @param state The owning scheduler.
    explicit WorkerPoolWake(std::weak_ptr<WorkerPoolState> state);
    /// Release retained mailbox admission when invocation was discarded.
    ~WorkerPoolWake();

public: // delivery
    /// Dispatch one notification and disarm discard cleanup, including when a handler throws.
    void invoke();

private:
    std::weak_ptr<WorkerPoolState> _state; ///< No retention cycle through a stalled owner loop.
    bool _invoked{false};                  ///< Only accessed by the final owner-loop invocation/destruction.
};

}
