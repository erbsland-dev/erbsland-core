// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <cstddef>
#include <memory>
#include <utility>

namespace demo {

/// Guard preparation that exists only to produce a trace message.
///
/// `trace()` already suppresses its own formatting while disabled. An explicit `traceEnabled()` guard also avoids the
/// route search performed before the call, so the disabled manager performs no diagnostic preparation at all.
void traceSectionGuard() {
    const auto runSearch = [](const bool enableSection) -> std::size_t {
        const auto section = el::LogTraceSection{"route-search"_el};
        auto format = el::LogLineFormat{};
        format.setPattern("{level}: {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(format))
            .addWriter(
                el::LogWriter::createForConsole(el::application().terminal()),
                el::LogWriterFilter{el::LogLevels{el::LogLevel::Trace}});
        if (enableSection) {
            configuration.enableTraceSection(section);
        }

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->createStream("guild/route-search"_el, section);
        auto preparationCount = std::size_t{};
        if (log->traceEnabled()) {
            ++preparationCount;
            const auto route = el::String::fromJoined({"Kallio"_el, " → "_el, "Jääjärvi"_el, " → "_el, "Majakka"_el});
            log->trace("Candidate route: "_el, route);
        }
        manager->shutdown();
        return preparationCount;
    };

    el::io::printLine("Preparations while disabled: "_el, runSearch(false));
    const auto enabledCount = runSearch(true);
    el::io::printLine("Preparations while enabled : "_el, enabledCount);
}

}
