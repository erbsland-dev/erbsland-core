// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>
#include <erbsland/SubprocessExitStatus.hpp>

namespace tool::frame {

/// Complete captured output and native status from a directly launched command.
struct CommandOutput {
    el::SubprocessExitStatus status; ///< Exit code or termination signal.
    el::String output;               ///< Complete standard output.
    el::String error;                ///< Complete standard error.
};

}
