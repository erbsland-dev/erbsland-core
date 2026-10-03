// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventLoopRunScope.hpp"

#include "EventLoop.hpp"

namespace erbsland::event::impl {

EventLoopRunScope::EventLoopRunScope(EventLoop &loop) : _loop{loop}, _eventsScope{loop.beginDispatch()} {
}

EventLoopRunScope::~EventLoopRunScope() noexcept {
    _loop.finishDispatch();
}

}
