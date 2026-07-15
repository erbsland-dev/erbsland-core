// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Coroutine operations preserve timeout as a normal stream result.
/// Awaiting moves bounded work away from the caller thread; it does not convert flow control into an exception.
void handleCoroutineTimeout() {
    const auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{9U}, 1U, 1U);
    auto first = input->coRead(el::ByteLength{1U});
    waitForTask(first);
    el::io::printLine("First coroutine timeout: "_el, first.result().isTimeout());

    auto second = input->coRead(el::ByteLength{1U});
    waitForTask(second);
    el::io::printLine("Anden coroutine fik data: "_el, second.result().hasData());
}

}
