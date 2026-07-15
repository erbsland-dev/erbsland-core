// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <chrono>
#include <thread>

namespace demo {

/// Destroying or cancelling an incomplete `CoTask` suppresses its continuation.
/// Already-running bounded stream work may finish, and its retained stream ownership is released afterward.
void cancelPendingTask() {
    auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{8U}, 1U, 0U, true, true);
    auto weak = std::weak_ptr<ScriptedByteInputStream>{input};
    auto task = input->coRead(el::ByteLength{1U});
    while (input->readCount() == 0U) {
        std::this_thread::yield();
    }
    task.cancel();
    input->release();
    input.reset();

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!weak.expired() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    el::io::printLine("Annulleret opgave frigav streamen: "_el, weak.expired());
}

}
