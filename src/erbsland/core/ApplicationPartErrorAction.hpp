// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::core {

/// The manager action after an application-part failure.
enum class ApplicationPartErrorAction : uint8_t {
    Continue, ///< Stop the failed branch and keep unrelated parts running.
    StopAll,  ///< Stop every part and finish the manager in `Failed` state.
};

}
