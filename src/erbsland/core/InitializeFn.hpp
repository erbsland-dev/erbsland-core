// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Definitions.hpp"

#include <functional>

namespace erbsland::core {

/// A initialize function override for an application.
using InitializeFn = std::function<void()>;

}
