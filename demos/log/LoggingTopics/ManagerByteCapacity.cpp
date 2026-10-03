// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ManagerDemoWriters.hpp"

#include <DemoCommon.hpp>
#include <erbsland/all_log.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/LogManagerOptions.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Bound the memory retained by entries waiting in the queue.
///
/// `setMaximumBytes()` applies even when many entry slots remain. Four large warnings fit below this one-mebibyte byte
/// budget; another large warning is dropped while the producer continues immediately.
void managerByteCapacity() {
    auto options = el::LogManagerOptions{};
    options.setMaximumBytes(el::ByteCount{1024U * 1024U});
    const auto writer = std::make_shared<BlockingLogWriter>();
    auto configuration = el::LogConfiguration{};
    configuration.setManagerOptions(options).addWriter(writer);

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->rootStream();
    log->warn("The journal destination is occupied."_el);
    writer->waitUntilWriting();

    const auto largeReport = el::String::fromCharacter(el::Char{U'x'}, el::CpCount{250U * 1024U});
    for (auto index = 0U; index < 4U; ++index) {
        log->warn(largeReport);
    }
    log->warn(largeReport);
    const auto full = manager->statistics();
    el::io::printLine("Queued large entries: "_el, full.queuedEntries);
    el::io::printLine("Dropped entries: "_el, full.droppedEntries);

    writer->release();
    manager->shutdown();
}

}
