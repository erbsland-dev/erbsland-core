// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

[[nodiscard]] auto countRiverBlocks(const std::shared_ptr<ScriptedByteInputStream> &input) -> el::CoTask<std::size_t> {
    auto generator = input->coReadBlocks(el::ByteLength{2U});
    auto count = std::size_t{0U};
    while (auto next = co_await generator.next()) {
        if (next->hasData()) {
            ++count;
        }
    }
    co_return count;
}

/// `coReadBlocks()` is a lazy, single-pass sequence of owned blocks.
/// Advancing it is asynchronous; data and timeout results are yielded, while end-of-stream completes the generator.
void processBlockGenerator() {
    const auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{1U, 1U, 2U, 3U, 5U}, 2U);
    auto task = countRiverBlocks(input);
    waitForTask(task);
    el::io::printLine("Data blocks from the stream: "_el, task.result());
}

}
