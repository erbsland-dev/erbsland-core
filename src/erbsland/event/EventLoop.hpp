// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventBackend.hpp"
#include "EventBackend_fwd.hpp"
#include "EventLoop_fwd.hpp"
#include "EventTarget.hpp"

namespace erbsland::event {

/// An event loop.
/// Each event loop owns its queue, scheduler and backend and source registry.
class EventLoop : public EventTarget {
public:
    ~EventLoop() override = default;

public: // factory methods
    [[nodiscard]] static auto create() -> EventLoopPtr;
    [[nodiscard]] static auto create(const EventBackendPtr &backend) -> EventLoopPtr;

public:
};

}
