// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/RuntimeError.hpp"

namespace erbsland::util {

/// Work was submitted after the pool or its event delivery closed.
/// @tested{WorkerPoolTest WorkerPoolEventTest}
class WorkerPoolClosedError : public err::RuntimeError {
public:
    using err::RuntimeError::RuntimeError;
};

}
