// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventLoopDriver.hpp"

#if defined(_WIN32)
#include "impl/WindowsEventLoopDriver.hpp"
#elif defined(__APPLE__)
#include "impl/KqueueEventLoopDriver.hpp"
#elif defined(__linux__)
#include "impl/EpollEventLoopDriver.hpp"
#else
#error "No native event-loop driver is available for this platform."
#endif

namespace erbsland::event {

auto EventLoopDriver::createDefault() -> EventLoopDriverPtr {
#if defined(_WIN32)
    return std::make_shared<impl::WindowsEventLoopDriver>();
#elif defined(__APPLE__)
    return std::make_shared<impl::KqueueEventLoopDriver>();
#elif defined(__linux__)
    return std::make_shared<impl::EpollEventLoopDriver>();
#endif
}

}
