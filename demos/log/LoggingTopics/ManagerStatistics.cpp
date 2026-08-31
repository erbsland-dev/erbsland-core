// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ManagerDemoWriters.hpp"

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Read point-in-time counters without changing manager options.
///
/// While paused, accepted entries remain visible as queued work. After resume and shutdown, the retained writer has
/// delivered both entries and the deliberately failing destination contributes one contained writer failure.
void managerStatistics() {
    auto configuration = el::LogConfiguration{};
    configuration.addWriter(std::make_shared<FailingLogWriter>())
        .addWriter(std::make_shared<el::LastErrorsLogWriter>());
    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));

    manager->pause();
    manager->rootStream()->error("The northern gate could not be opened."_el);
    manager->rootStream()->error("The ridge team returned to the lodge."_el);
    const auto paused = manager->statistics();
    el::io::printLine("While paused:"_el);
    el::io::printLine("  Accepted entries: "_el, paused.acceptedEntries);
    el::io::printLine("  Written entries : "_el, paused.writtenEntries);
    el::io::printLine("  Queued entries  : "_el, paused.queuedEntries);
    el::io::printLine("  Queue has bytes : "_el, paused.queuedBytes.isZero() ? "no"_el : "yes"_el);

    manager->resume();
    manager->shutdown();
    const auto finished = manager->statistics();
    el::io::printLine("After shutdown:"_el);
    el::io::printLine("  Written entries : "_el, finished.writtenEntries);
    el::io::printLine("  Dropped entries : "_el, finished.droppedEntries);
    el::io::printLine("  Writer failures : "_el, finished.writerFailures);
    el::io::printLine("  Queued entries  : "_el, finished.queuedEntries);
}

}
