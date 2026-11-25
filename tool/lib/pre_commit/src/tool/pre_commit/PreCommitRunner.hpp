// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ExitCode.hpp>
#include <erbsland/List.hpp>
#include <tool/frame/PreCommitTask.hpp>

namespace tool::pre_commit {

/// Run an ordered task list, retaining every operational failure and its diagnostics.
class PreCommitRunner final {
public: // execution
    /// Execute all tasks through the active tool application's services.
    /// @param tasks Ordered task instances, each invoked once.
    /// @return Success only when all tasks complete without errors.
    [[nodiscard]] auto run(const el::List<frame::PreCommitTaskPtr> &tasks) const -> el::ExitCode;
};

}
