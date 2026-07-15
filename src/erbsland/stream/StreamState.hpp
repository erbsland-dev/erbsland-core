// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::stream {

/// The lifecycle state of a stream.
enum class StreamState : uint8_t {
    Open,    ///< Operations are accepted.
    Closing, ///< Graceful closing is in progress.
    Closed,  ///< The stream is closed.
    Failed,  ///< A native stream operation failed.
};

}
