// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ManagerDemoWriters.hpp"

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Keep queue slots available for warning and error entries.
///
/// The total capacity is four entries and `setReservedErrorEntries()` protects two of them. Information fills the two
/// ordinary slots, another information entry is dropped, and warning plus error can still use the protected slots.
void managerErrorEntryReserve() {
    auto options = el::LogManagerOptions{};
    options.setMaximumEntries(4U).setReservedErrorEntries(2U);
    const auto writer = std::make_shared<BlockingLogWriter>();
    auto configuration = el::LogConfiguration{};
    configuration.setManagerOptions(options).addWriter(writer);

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->rootStream();
    log->info("The journal destination is occupied."_el);
    writer->waitUntilWriting();

    log->info("Ordinary slot one."_el);
    log->info("Ordinary slot two."_el);
    log->info("No ordinary slot remains."_el);
    log->warn("The warning uses a protected slot."_el);
    log->error("The error uses the final protected slot."_el);
    log->error("The complete queue is now full."_el);
    const auto full = manager->statistics();
    el::io::printLine("Queued entries: "_el, full.queuedEntries);
    el::io::printLine("Dropped entries: "_el, full.droppedEntries);

    writer->release();
    manager->shutdown();
}

}
