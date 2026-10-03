// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/RuntimeError.hpp"

namespace erbsland::util {

/// The configured worker-pool admission limit was reached.
/// @tested{WorkerPoolTest WorkerPoolEventTest}
class WorkerPoolFullError : public err::RuntimeError {
public:
    using err::RuntimeError::RuntimeError;
};

}
