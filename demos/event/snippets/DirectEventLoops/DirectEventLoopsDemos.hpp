// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

namespace demo {

/// Run a loop in short turns until a delayed entry arrives.
void delayedEntry();
/// Drain the journal entries that are ready to dispatch.
void readyEntries();

}
