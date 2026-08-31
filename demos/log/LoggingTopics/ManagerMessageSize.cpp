// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Bound one producer message before its entry enters the queue.
///
/// `setMaximumMessageBytes()` counts encoded bytes, preserves a valid character boundary, and adds an ellipsis when the
/// message is shortened. The immutable entry records that producer-side truncation occurred.
void managerMessageSize() {
    auto options = el::LogManagerOptions{};
    options.setMaximumMessageBytes(el::ByteLength{32U});
    const auto retained = std::make_shared<el::LastErrorsLogWriter>();
    auto configuration = el::LogConfiguration{};
    configuration.setManagerOptions(options).addWriter(retained);

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    manager->rootStream()->error("Aurora observation continues beyond midnight."_el);
    manager->shutdown();

    const auto entry = retained->snapshot().front();
    el::io::printLine("Retained message: "_el, entry->message());
    el::io::printLine("Marked as truncated: "_el, entry->isTruncated() ? "yes"_el : "no"_el);
}

}
