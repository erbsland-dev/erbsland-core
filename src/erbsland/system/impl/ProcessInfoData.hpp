// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProcessInfoError.hpp"

#include "../ProcessId.hpp"
#include "../UserId.hpp"

#include "../../path/Path.hpp"
#include "../../time/DateTime.hpp"

namespace erbsland::system::impl {

/// One coherent process-information snapshot.
/// @tested{ProcessInfoTest SubprocessInteropTest}
struct ProcessInfoData final {
    bool exists{};                         ///< Whether the platform reports this process.
    path::Path executablePath;             ///< Absolute executable image path, when available.
    ProcessId parentProcessId;             ///< Parent process identifier, when available.
    time::DateTime startTime;              ///< Process start time in UTC, when available.
    UserId ownerId;                        ///< Platform owner identifier, when available.
    ProcessInfoError lookupError;          ///< Failure to determine the process snapshot.
    ProcessInfoError processError;         ///< Reason attributes are unavailable for an absent process.
    ProcessInfoError executablePathError;  ///< Executable-path query failure.
    ProcessInfoError parentProcessIdError; ///< Parent-process query failure.
    ProcessInfoError startTimeError;       ///< Start-time query failure.
    ProcessInfoError ownerIdError;         ///< Owner query failure.
};

}
