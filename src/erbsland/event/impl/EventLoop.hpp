// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../EventLoop.hpp"

namespace erbsland::event::impl {

/// The implementation of the event loop.
class EventLoop : public event::EventLoop {
public:
    EventLoop() = default;
};

}
