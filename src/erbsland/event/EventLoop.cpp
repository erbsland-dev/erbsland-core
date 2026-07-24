// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventLoop.hpp"

#include "impl/EventLoop.hpp"

namespace erbsland::event {

auto EventLoop::create() -> EventLoopPtr {
    return std::make_shared<impl::EventLoop>();
}

auto EventLoop::create(EventBackendPtr backend) -> EventLoopPtr {
    auto result = std::make_shared<impl::EventLoop>();
    result->registerBackend(std::move(backend));
    return result;
}

auto EventLoop::create(EventLoopDriverPtr driver) -> EventLoopPtr {
    return std::make_shared<impl::EventLoop>(std::move(driver));
}

}
