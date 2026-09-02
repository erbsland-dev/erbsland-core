// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Use one trace section as a stable startup choice for related diagnostics.
///
/// Create the section once, enable it in the complete configuration, and attach it when creating the stream. The same
/// stream continues to carry information messages whether or not its trace section is enabled.
void traceSections() {
    const auto routeSearchTrace = el::LogTraceSection{"route-search"_el};
    auto format = el::LogLineFormat{};
    format.setPattern("{level} [{name}] {message}"_el);

    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .enableTraceSection(routeSearchTrace)
        .addWriter(
            el::LogWriter::createForConsole(el::application().terminal()),
            el::LogWriterFilter{el::LogLevels{el::LogLevel::Trace, el::LogLevel::Information}});

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto routeLog = manager->createStream("guild/route-search"_el, routeSearchTrace);

    // Only build a detailed route description when tracing is active.
    if (routeLog->traceEnabled()) {
        const auto route = el::String::fromJoined({"Kallio"_el, " → "_el, "Jääjärvi"_el, " → "_el, "Majakka"_el});
        routeLog->trace("Candidate route: "_el, route);
    }
    routeLog->info("The route search selected three waypoints."_el);
    manager->shutdown();
}

}
