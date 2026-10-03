// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/all_log.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogLevel.hpp>
#include <erbsland/LogLineFormat.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/LogTraceSection.hpp>
#include <erbsland/LogWriter.hpp>
#include <erbsland/LogWriterFilter.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Group diagnostics by investigation rather than by stream path.
///
/// The route-search section spans map and weather modules because both help explain route selection. Radio frames
/// answer a different question and remain silent even though the writer route accepts trace from every path.
void traceSectionGrouping() {
    constexpr auto routeSearch = el::LogTraceSection{"route-search"_el};
    constexpr auto radioFrames = el::LogTraceSection{"radio-frames"_el};
    auto format = el::LogLineFormat{};
    format.setPattern("{level} [{name}] {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .enableTraceSection(routeSearch)
        .addWriter(
            el::LogWriter::createForConsole(el::application().terminal()),
            el::LogWriterFilter{}.setAcceptedLevels({el::LogLevel::Trace}));

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto mapLog = manager->createStream("guild/map"_el, routeSearch);
    const auto weatherLog = manager->createStream("guild/weather"_el, routeSearch);
    const auto radioLog = manager->createStream("guild/radio"_el, radioFrames);
    mapLog->trace("Candidate path crosses the eastern ridge."_el);
    weatherLog->trace("The eastern ridge remains below the cloud line."_el);
    radioLog->trace("Raw frame bytes are available."_el);
    manager->shutdown();
}

}
