// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/all_log.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogLineFormat.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/LogWriter.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Install one line format as part of a complete logging configuration.
///
/// Create `LogLineFormat` as a value, customize it, and move it into `LogConfiguration`. The manager takes another
/// complete configuration value and uses this default for routes without their own line-format override.
void lineFormats() {
    auto lineFormat = el::LogLineFormat{};
    lineFormat.setPattern("{level} [{name}] {message}"_el);

    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(lineFormat))
        .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->createStream("guild/routes"_el);
    log->info("The route catalog is ready."_el);
    manager->shutdown();
}

}
