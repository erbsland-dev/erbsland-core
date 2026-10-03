// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/Tool.hpp"

namespace tool::pre_commit {

/// Run registered maintenance tasks and verify that normalization left the repository unchanged.
class PreCommitTool final : public frame::Tool {
public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    void execute() override;
    [[nodiscard]] auto exitCode() const noexcept -> el::ExitCode override { return _exitCode; }

private:
    el::ExitCode _exitCode; ///< Aggregate result after all tasks have completed.
};

}
