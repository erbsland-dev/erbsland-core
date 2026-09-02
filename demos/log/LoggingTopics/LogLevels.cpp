// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Choose a level by the consequence an entry has for its reader.
///
/// Trace explains an enabled diagnostic path, information records ordinary progress, a warning describes work that
/// can continue in a degraded state, and an error marks an operation that failed.
void logLevels() {
    auto format = el::LogLineFormat{};
    format.setPattern("{level}: {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .enableTraceSection(el::LogTraceSection{"route-search"_el})
        .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->createStream("guild/routes"_el, el::LogTraceSection{"route-search"_el});
    log->trace("Compared the lake and ridge routes."_el);
    log->info("Selected the ridge route."_el);
    log->warn("The last marker is weathered, but still readable."_el);
    log->error("The northern checkpoint did not answer."_el);
    manager->shutdown();
}

}
