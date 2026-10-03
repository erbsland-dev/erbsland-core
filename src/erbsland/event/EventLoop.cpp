// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventLoop.hpp"

#include "impl/EventLoop.hpp"

namespace erbsland::event {

auto EventLoop::create(const EventLoopOptions &options) -> EventLoopPtr {
    return std::make_shared<impl::EventLoop>(options);
}

auto EventLoop::create(EventBackendPtr backend, const EventLoopOptions &options) -> EventLoopPtr {
    auto result = std::make_shared<impl::EventLoop>(options);
    result->registerBackend(std::move(backend));
    return result;
}

auto EventLoop::create(EventLoopDriverPtr driver, const EventLoopOptions &options) -> EventLoopPtr {
    return std::make_shared<impl::EventLoop>(std::move(driver), options);
}

}
