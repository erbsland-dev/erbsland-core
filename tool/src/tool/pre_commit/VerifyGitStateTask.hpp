// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GitState.hpp"

#include "../frame/PreCommitTask.hpp"

namespace tool::pre_commit {

/// Verify repository stability after all maintenance tasks, including failed checks.
class VerifyGitStateTask final : public frame::PreCommitTask {
public:
    /// Bind the run-owned state shared with initial capture.
    /// @param state Non-null state retained for the run.
    explicit VerifyGitStateTask(std::shared_ptr<GitState> state);

public: // implement PreCommitTask
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    void execute() override;

private:
    std::shared_ptr<GitState> _state; ///< Shared baseline and Git executable.
};

}
