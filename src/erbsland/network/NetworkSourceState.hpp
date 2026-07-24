// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::network {

/// The lifecycle state of a network event source.
enum class NetworkSourceState : uint8_t {
    Inactive, ///< Configurable and not yet started.
    Starting, ///< Started but not yet ready for regular operation.
    Active,   ///< Ready for regular operation.
    Closing,  ///< Graceful closure is draining accepted output.
    Closed,   ///< Terminal successful or cancelled state.
    Failed,   ///< Terminal state after an operational error.
};

}
