// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KqueueEventLoopNativeCalls.hpp"

namespace erbsland::event::impl {

auto KqueueEventLoopNativeCalls::change(const int queue, const struct kevent *changes, const int count) noexcept
    -> int {
    return ::kevent(queue, changes, count, nullptr, 0, nullptr);
}

}
