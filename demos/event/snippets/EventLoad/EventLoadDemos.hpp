// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

namespace demo {

/// Divide a calculation into bounded callbacks that share loop time.
void chunkedWork();
/// Deliver the latest value of each burst with one waiting notification.
void coalescedUpdates();
/// Aggregate independent readings with a bounded number of requests.
void collectReadings();
/// Discard incoming progress samples when a display queue reaches capacity.
void replaceableUpdates();

}
