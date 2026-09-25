// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ManagerDemoWriters.hpp"

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Keep part of the queue's byte budget available for warning and error entries.
///
/// `setReservedErrorBytes()` protects 512 bytes within a two-kibibyte total. A large information entry consumes most of
/// the ordinary budget, so another is dropped while a warning of the same size can use the reservation.
void managerErrorByteReserve() {
    auto options = el::LogManagerOptions{};
    options.setMaximumBytes(el::ByteLength{2048U}).setReservedErrorBytes(el::ByteLength{512U});
    const auto writer = std::make_shared<BlockingLogWriter>();
    auto configuration = el::LogConfiguration{};
    configuration.setManagerOptions(options).addWriter(writer);

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->rootStream();
    log->info("The journal destination is occupied."_el);
    writer->waitUntilWriting();

    log->info(el::String::fromCharacter(el::Char{U'i'}, el::CpLength{1200U}));
    const auto shortReport = el::String::fromCharacter(el::Char{U'w'}, el::CpLength{300U});
    log->info(shortReport);
    log->warn(shortReport);
    const auto full = manager->statistics();
    el::io::printLine("Queued entries: "_el, full.queuedEntries);
    el::io::printLine("Dropped entries: "_el, full.droppedEntries);

    writer->release();
    manager->shutdown();
}

}
