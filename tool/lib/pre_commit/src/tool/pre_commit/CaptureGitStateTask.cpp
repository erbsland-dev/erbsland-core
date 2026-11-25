// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CaptureGitStateTask.hpp"

namespace tool::pre_commit {

using namespace el::text::literals;

CaptureGitStateTask::CaptureGitStateTask(std::shared_ptr<GitState> state) : _state{std::move(state)} {
}

auto CaptureGitStateTask::identifier() const -> el::String {
    return "capture_git_state"_el;
}

auto CaptureGitStateTask::title() const -> el::String {
    return "Capture Git state"_el;
}

void CaptureGitStateTask::execute() {
    _state->capture(taskContext().progress);
}

}
