// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventThread.hpp"

namespace erbsland::event {

/// A standalone thread that runs an event loop.
/// Unmanaged event threads are not registered with `core::Application` and are not quit by application shutdown.
/// @tested{EventThreadTest ApplicationEventTest}
class UnmanagedEventThread : public EventThread {
public:
    ~UnmanagedEventThread() override = default;

public:
    /// Create an unmanaged event thread.
    [[nodiscard]] static auto create() -> UnmanagedEventThreadPtr;
};

}
