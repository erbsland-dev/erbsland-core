// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::time {

/// A well-known time epoch.
enum class TimeEpoch : uint8_t {
    Core,    ///< The Erbsland Core epoch, 0000-01-01 00:00:00 UTC.
    Posix,   ///< The POSIX epoch, 1970-01-01 00:00:00 UTC.
    Windows, ///< The Windows FILETIME epoch, 1601-01-01 00:00:00 UTC.
    Rfc868,  ///< The RFC 868 epoch, 1900-01-01 00:00:00 UTC.
};

}
