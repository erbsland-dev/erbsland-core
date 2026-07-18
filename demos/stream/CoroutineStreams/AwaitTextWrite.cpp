// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Asynchronous text output owns its string until the complete atomic request is accepted.
/// This is useful when a producer coroutine must not wait for output back pressure on its current thread.
void awaitTextWrite() {
    const auto output = el::AnyStringBuilderStream::create();
    auto task = output->coWriteLine(el::String{"Flodprofil: rolig strøm ved østbredden"_el});
    waitForTask(task);

    el::io::printLine("Skrivning accepteret: "_el, task.result().isSuccess());
    el::io::print(output->takeString());
}

}
