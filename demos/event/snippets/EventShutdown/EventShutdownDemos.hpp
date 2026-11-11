// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

namespace demo {

/// Cancel one operation while its loop continues to dispatch unrelated work.
void cooperativeCancellation();
/// Drain accepted observations and reject submissions after quit.
void drainAcceptedWork();

}
