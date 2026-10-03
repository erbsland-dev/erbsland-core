// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/RuntimeError.hpp"

namespace erbsland::util {

/// A worker operation was cancelled before committing its result.
/// @tested{WorkerPoolTest WorkerPoolEventTest}
class WorkerCancelledError : public err::RuntimeError {
public:
    using err::RuntimeError::RuntimeError;
};

}
