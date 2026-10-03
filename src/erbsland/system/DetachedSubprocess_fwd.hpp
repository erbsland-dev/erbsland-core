// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::system {

class DetachedSubprocess;
/// Shared access to a detached child identifier.
using DetachedSubprocessPtr = std::shared_ptr<DetachedSubprocess>;

}
