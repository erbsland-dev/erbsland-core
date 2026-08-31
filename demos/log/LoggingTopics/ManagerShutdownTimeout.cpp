// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ManagerDemoWriters.hpp"

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <chrono>
#include <memory>
#include <thread>
#include <utility>

namespace demo {

/// Limit how long shutdown keeps entries waiting behind an active writer call.
///
/// `setShutdownTimeout()` gives the queue 20 milliseconds to drain. The simulated destination remains occupied for
/// longer, so the two entries still waiting at the deadline are dropped and recorded in the statistics.
void managerShutdownTimeout() {
    auto options = el::LogManagerOptions{};
    options.setShutdownTimeout(el::Milliseconds{20});
    const auto writer = std::make_shared<BlockingLogWriter>();
    auto configuration = el::LogConfiguration{};
    configuration.setManagerOptions(options).addWriter(writer);

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->rootStream();
    log->info("The destination is writing this entry."_el);
    writer->waitUntilWriting();
    log->info("This entry is waiting in the queue."_el);
    log->info("This entry is also waiting in the queue."_el);

    auto releaseThread = std::thread{[writer]() -> void {
        std::this_thread::sleep_for(std::chrono::milliseconds{40});
        writer->release();
    }};
    manager->shutdown();
    releaseThread.join();

    const auto finished = manager->statistics();
    el::io::printLine("Written entries: "_el, finished.writtenEntries);
    el::io::printLine("Dropped entries: "_el, finished.droppedEntries);
}

}
