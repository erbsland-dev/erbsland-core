// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PreCommitProbe.hpp"

namespace test::impl {

using namespace el::text::literals;

/// First registered task.
class FirstPreCommitProbe final : public PreCommitProbe {
public: // implement Tool and PreCommitTask
    [[nodiscard]] auto identifier() const -> el::String override { return "first_task"_el; }
};

/// Second registered task.
class MiddlePreCommitProbe final : public PreCommitProbe {
public: // implement Tool and PreCommitTask
    [[nodiscard]] auto identifier() const -> el::String override { return "middle_task"_el; }
};

/// Final registered task.
class LastPreCommitProbe final : public PreCommitProbe {
public: // implement Tool and PreCommitTask
    [[nodiscard]] auto identifier() const -> el::String override { return "last_task"_el; }
};

/// A registered task with no command interface.
class TaskOnlyPreCommitProbe final : public tool::frame::PreCommitTask {
public: // implement PreCommitTask
    [[nodiscard]] auto identifier() const -> el::String override { return "task_only"_el; }
    [[nodiscard]] auto title() const -> el::String override { return "Task only"_el; }
    void execute() override { taskContext().log->warn("task-only warning"_el); }
};

}
