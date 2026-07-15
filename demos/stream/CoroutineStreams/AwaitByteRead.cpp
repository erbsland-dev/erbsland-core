// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

[[nodiscard]] auto readRiverGauge(const std::shared_ptr<ScriptedByteInputStream> &input) -> el::CoTask<uint8_t> {
    const auto result = co_await input->coRead(el::ByteLength{1U});
    co_return result.data().get(el::ByteIndex{0U}).toUInt8();
}

/// Stream `CoTask` operations start eagerly and run the matching bounded synchronous call on the worker service.
/// Awaiting preserves the normal result status and rethrows stream errors at the observation point.
void awaitByteRead() {
    const auto input = std::make_shared<ScriptedByteInputStream>(std::vector<uint8_t>{37U});
    auto task = readRiverGauge(input);
    waitForTask(task);
    el::io::printLine("Vandstand: "_el, task.result(), " cm"_el);
}

}
