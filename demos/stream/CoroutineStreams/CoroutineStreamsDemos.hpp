// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

namespace demo {

void awaitByteRead();
void awaitTextWrite();
void cancelPendingTask();
void handleCoroutineTimeout();
void processBlockGenerator();
void processLineGenerator();
void retainStreamOwnership();

}
