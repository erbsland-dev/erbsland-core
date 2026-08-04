// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventThread.hpp"

namespace erbsland::event {

/// An application-managed thread that runs an event loop.
/// Managed event threads are created by `core::Application::createEventThread()` and are quit together with the
/// application event loop.
/// @tested{ApplicationEventTest}
class ManagedEventThread : public EventThread {
public:
    // defaults
    ~ManagedEventThread() override = default;
};

}
