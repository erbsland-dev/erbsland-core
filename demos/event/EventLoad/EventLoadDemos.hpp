// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

namespace demo {

/// Aggregate independent readings with a bounded number of requests.
/// @notest{Compiled demo verifies the result count and sum.}
void collectReadings();

void replaceableUpdates();

}
