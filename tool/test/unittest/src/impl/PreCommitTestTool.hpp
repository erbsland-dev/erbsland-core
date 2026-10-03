// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tool/frame/ToolApplication.hpp>
#include <tool/pre_commit/PreCommitRunner.hpp>

namespace test::impl {

using namespace el::text::literals;

/// Run only fixture tasks, without repository-specific Git tasks.
class PreCommitTestTool final : public tool::frame::Tool {
public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override { return "test_tasks"_el; }
    [[nodiscard]] auto title() const -> el::String override { return "Run fixture tasks"_el; }
    [[nodiscard]] auto description() const -> el::String override { return "Run fixture tasks."_el; }
    void execute() override { _result = tool::pre_commit::PreCommitRunner{}.run(tool::frame::app().preCommitTasks()); }
    [[nodiscard]] auto exitCode() const noexcept -> el::ExitCode override { return _result; }

private:
    el::ExitCode _result; ///< Aggregate task status.
};

}
