// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Install one line format as part of a complete logging configuration.
///
/// Create `LogLineFormat` as a value, customize it, and move it into `LogConfiguration`. The manager takes another
/// complete configuration value and applies that same line format to every writer route in the snapshot.
void lineFormats() {
    auto lineFormat = el::LogLineFormat{};
    lineFormat.setPattern("{level} [{name}] {message}"_el);

    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(lineFormat))
        .addWriter(std::make_shared<el::ConsoleLogWriter>(el::application().terminal()));

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->createStream("guild/routes"_el);
    log->info("The route catalog is ready."_el);
    manager->shutdown();
}

}
