// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VerifyGitStateTask.hpp"

namespace tool::pre_commit {

using namespace el::text::literals;

VerifyGitStateTask::VerifyGitStateTask(std::shared_ptr<GitState> state) : _state{std::move(state)} {
}

auto VerifyGitStateTask::identifier() const -> el::String {
    return "verify_git_state"_el;
}

auto VerifyGitStateTask::title() const -> el::String {
    return "Verify Git state"_el;
}

void VerifyGitStateTask::execute() {
    _state->verify(taskContext().progress);
}

}
