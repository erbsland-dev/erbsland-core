// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SkillTreeStreams.hpp"

#include <StreamDemoSupport.hpp>

namespace demo {

/// Provide a shared factory when callers should use inherited coroutine methods.
/// The coroutine wrapper retains the custom stream while its bounded source operation runs on the worker service.
void useCustomStreamAsynchronously() {
    const auto input = SkillTreeInputStream::create({9U, 8U, 7U, 6U});
    auto task = input->coReadExact(el::ByteLength{4U});
    waitForTask(task);
    el::io::printLine("Asynchronous read completed: "_el, task.result().hasData());
}

}
