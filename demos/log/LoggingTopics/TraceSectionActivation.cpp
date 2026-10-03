// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/all_log.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogLevel.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/LogTraceSection.hpp>
#include <erbsland/LogWriter.hpp>
#include <erbsland/LogWriterFilter.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Separate producer trace activation from writer delivery.
///
/// These three independent managers show that the section controls trace production. Writer filters independently
/// decide whether queued trace entries reach a destination.
void traceSectionActivation() {
    const auto showState = [](const el::String &label, const bool enableSection, const bool acceptTrace) -> void {
        constexpr auto section = el::LogTraceSection{"route-search"_el};
        auto configuration = el::LogConfiguration{};
        if (enableSection) {
            configuration.enableTraceSection(section);
        }
        const auto levels = acceptTrace ? el::LogLevels{el::LogLevel::Trace} : el::LogLevels{el::LogLevel::Information};
        configuration.addWriter(
            el::LogWriter::createForConsole(el::application().terminal()),
            el::LogWriterFilter{}.setAcceptedLevels(levels));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->createStream("guild/route-search"_el, section);
        el::io::printLine(label, log->traceEnabled() ? "enabled"_el : "disabled"_el);
        manager->shutdown();
    };

    showState("Section only : "_el, true, false);
    showState("Route only   : "_el, false, true);
    showState("Both together: "_el, true, true);
}

}
