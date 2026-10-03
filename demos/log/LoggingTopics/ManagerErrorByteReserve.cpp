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

/// Keep part of the queue's byte budget available for warning and error entries.
///
/// `setReservedErrorBytes()` protects 512 bytes within a two-kibibyte total. A large information entry consumes most of
/// the ordinary budget, so another is dropped while a warning of the same size can use the reservation.
void managerErrorByteReserve() {
    auto options = el::LogManagerOptions{};
    options.setMaximumBytes(el::ByteCount{2048U}).setReservedErrorBytes(el::ByteCount{512U});
    const auto writer = std::make_shared<BlockingLogWriter>();
    auto configuration = el::LogConfiguration{};
    configuration.setManagerOptions(options).addWriter(writer);

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->rootStream();
    log->info("The journal destination is occupied."_el);
    writer->waitUntilWriting();

    log->info(el::String::fromCharacter(el::Char{U'i'}, el::CpCount{1200U}));
    const auto shortReport = el::String::fromCharacter(el::Char{U'w'}, el::CpCount{300U});
    log->info(shortReport);
    log->warn(shortReport);
    const auto full = manager->statistics();
    el::io::printLine("Queued entries: "_el, full.queuedEntries);
    el::io::printLine("Dropped entries: "_el, full.droppedEntries);

    writer->release();
    manager->shutdown();
}

}
