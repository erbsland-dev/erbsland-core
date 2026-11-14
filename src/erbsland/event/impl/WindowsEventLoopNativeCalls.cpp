// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsEventLoopNativeCalls.hpp"

namespace erbsland::event::impl {

auto WindowsEventLoopNativeCalls::postWake(HANDLE port, const ULONG_PTR key) noexcept -> BOOL {
    return PostQueuedCompletionStatus(port, 0, key, nullptr);
}

}
