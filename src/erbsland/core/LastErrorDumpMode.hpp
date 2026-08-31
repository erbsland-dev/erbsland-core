// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::core {

/// Select when an enabled retained-error snapshot is displayed during final application cleanup.
enum class LastErrorDumpMode : uint8_t {
    OnFailure, ///< Display retained errors only after a nonzero application exit code.
    Always,    ///< Display retained errors after every application run.
};

}
