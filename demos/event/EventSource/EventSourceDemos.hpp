// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

namespace demo {

/// Demonstrate owner-loop setup and replaceable source handlers.
void handlerSetup();
/// Demonstrate retained, moved, and cancelled observer subscriptions.
void observerLifetime();
/// Demonstrate value, strong, and weak callback captures.
void weakController();

}
