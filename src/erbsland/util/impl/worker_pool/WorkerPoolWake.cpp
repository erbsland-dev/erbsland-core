// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WorkerPoolWake.hpp"

#include "WorkerPoolState.hpp"

#include <utility>

namespace erbsland::util::impl {

WorkerPoolWake::WorkerPoolWake(std::weak_ptr<WorkerPoolState> state) : _state{std::move(state)} {
}

WorkerPoolWake::~WorkerPoolWake() {
    if (!_invoked) {
        if (const auto state = _state.lock()) {
            state->discardEvents();
        }
    }
}

void WorkerPoolWake::invoke() {
    _invoked = true;
    if (const auto state = _state.lock()) {
        state->dispatch();
    }
}

}
