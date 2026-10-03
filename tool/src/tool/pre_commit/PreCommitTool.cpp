// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PreCommitTool.hpp"

#include "CaptureGitStateTask.hpp"
#include "PreCommitRunner.hpp"
#include "VerifyGitStateTask.hpp"

#include "../frame/ToolApplication.hpp"

namespace tool::pre_commit {

using namespace el::text::literals;

auto PreCommitTool::identifier() const -> el::String {
    return "pre_commit"_el;
}

auto PreCommitTool::title() const -> el::String {
    return "Normalize and check the repository before committing."_el;
}

auto PreCommitTool::description() const -> el::String {
    return "Run every maintenance task, report all failures, and require unchanged Git state. Review generated changes and rerun."_el;
}

void PreCommitTool::execute() {
    const auto state = std::make_shared<GitState>(frame::app().files().projectRoot());
    auto tasks = el::List<frame::PreCommitTaskPtr>{std::make_shared<CaptureGitStateTask>(state)};
    tasks.append(frame::app().preCommitTasks());
    tasks.append(std::make_shared<VerifyGitStateTask>(state));
    _exitCode = PreCommitRunner{}.run(tasks);
}

}
