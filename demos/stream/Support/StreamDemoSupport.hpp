// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ScriptedByteInputStream.hpp"
#include "ScriptedByteOutputStream.hpp"

#include <DemoCommon.hpp>

#include <chrono>
#include <memory>
#include <thread>

namespace demo {

[[nodiscard]] auto createStreamDemoDirectory(const el::StringView &prefix) -> el::TempDirectoryPtr;
[[nodiscard]] auto bytesFromText(const el::StringView &text) -> el::ByteBlock;

template <typename tValue>
void waitForTask(el::CoTask<tValue> &task) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!task.isComplete() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
    if (!task.isComplete()) {
        throw el::LogicError{"The stream demo coroutine did not complete."};
    }
}

}
