// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// An inherited coroutine operation retains shared ownership until its bounded call finishes.
/// Library streams are therefore factory-created; invoking the same method on a stack-backed custom stream throws
/// `LogicError` before unsafe suspension can occur.
void retainStreamOwnership() {
    auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{55U});
    const auto weak = std::weak_ptr<ScriptedByteInputStream>{input};
    auto task = input->coRead(el::ByteLength{1U});
    input.reset();
    waitForTask(task);

    el::io::printLine("Data overlevede ejerens scope: "_el, task.result().hasData());
    el::io::printLine("Stream frigivet efter arbejdet: "_el, weak.expired());
}

}
