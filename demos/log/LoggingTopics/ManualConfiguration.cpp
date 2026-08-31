// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>

namespace demo {

/// Shut down a standalone log manager after its producers have finished.
///
/// Unlike the manager owned by `Application`, a manager created with `LogManager::create()` has no application
/// lifecycle to close it. An explicit shutdown drains its accepted entries and releases its writer resources.
void manualConfiguration() {
    auto lineFormat = el::LogLineFormat{};
    lineFormat.setPattern("{level} [{name}] {message}"_el);

    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(lineFormat));
    configuration.addWriter(
        std::make_shared<el::ConsoleLogWriter>(el::application().terminal()),
        el::LogWriterFilter{el::LogLevels{
            el::LogLevel::Information,
            el::LogLevel::Warning,
            el::LogLevel::Error,
        }});

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->createStream("guild/archive"_el);
    log->info("Map collection 'Järvien maa' was archived."_el);
    log->warn("One map sheet still needs a waterproof cover."_el);

    // Stop the standalone manager after the last producer call.
    manager->shutdown();
}

}
