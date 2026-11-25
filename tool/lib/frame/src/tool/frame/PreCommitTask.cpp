// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PreCommitTask.hpp"

#include <erbsland/LogicFailure.hpp>

namespace tool::frame {

using namespace el::text::literals;

auto PreCommitTask::taskContext() const -> const PreCommitContext & {
    if (!_taskContext) {
        throw el::LogicFailure{"Task services are unavailable before framework preparation."_el};
    }
    return *_taskContext;
}

}
