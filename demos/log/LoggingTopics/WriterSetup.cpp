// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>

namespace demo {

/// A writer becomes part of the log system when it is added to a complete `LogConfiguration` snapshot.
///
/// With no explicit filter, the route accepts every log level and stream path. Named trace streams still require an
/// enabled trace section. A regular application installs the snapshot in its existing log manager and lets the
/// application shut that manager down automatically when it exits.
void writerSetup() {
    auto format = el::LogLineFormat{};
    format.setPattern("{level} [{name}] {message}"_el);

    const auto consoleWriter = std::make_shared<el::ConsoleLogWriter>(el::application().terminal());
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format)).addWriter(consoleWriter);

    auto &manager = el::application().log();
    manager.setConfiguration(std::move(configuration));
    manager.createStream("guild/dispatch"_el)->info("The western trail report is ready."_el);
}

}
