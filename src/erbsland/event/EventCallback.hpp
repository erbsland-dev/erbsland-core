// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::event {

/// A callback that is executed by an event loop.
using EventCallback = std::function<void()>;

}
