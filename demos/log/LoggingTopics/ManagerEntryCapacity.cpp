// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ManagerDemoWriters.hpp"

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Bound the number of entries waiting behind a slow writer.
///
/// `setMaximumEntries()` limits queued entries, including warning and error reservations. The entry already inside the
/// writer is no longer in that queue, so 260 further warnings fit and the next one is dropped without blocking.
void managerEntryCapacity() {
    auto options = el::LogManagerOptions{};
    options.setMaximumEntries(260U);
    const auto writer = std::make_shared<BlockingLogWriter>();
    auto configuration = el::LogConfiguration{};
    configuration.setManagerOptions(options).addWriter(writer);

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->rootStream();
    log->warn("The journal destination is occupied."_el);
    writer->waitUntilWriting();

    for (auto index = 0U; index < 260U; ++index) {
        log->warn("A route report is waiting."_el);
    }
    log->warn("This report exceeds the queue-entry limit."_el);
    const auto full = manager->statistics();
    el::io::printLine("Queued entries: "_el, full.queuedEntries);
    el::io::printLine("Dropped entries: "_el, full.droppedEntries);

    writer->release();
    manager->shutdown();
}

}
