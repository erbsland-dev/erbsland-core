// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TaskProgress.hpp"

#include <erbsland/LogStream.hpp>

namespace tool::frame {

/// Services assigned for one task invocation, also available to standalone task tools.
struct PreCommitContext final {
    el::LogStreamPtr log;  ///< Task-specific diagnostic stream.
    TaskProgress progress; ///< Active during pre-commit, inactive for standalone commands.
};

}
