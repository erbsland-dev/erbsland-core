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

#include <memory>
#include <utility>

namespace demo {

/// Show the synchronous flag refresh caused by configuration replacement.
///
/// Runtime replacement is possible, but each call installs a complete manager policy. Immediately after
/// `setConfiguration()` returns, existing streams expose the new section decision through `traceEnabled()`.
void traceSectionReplacement() {
    constexpr auto section = el::LogTraceSection{"route-search"_el};
    const auto traceFilter = el::LogWriterFilter{}.setAcceptedLevels({el::LogLevel::Trace});
    const auto manager = el::LogManager::create();

    auto disabled = el::LogConfiguration{};
    disabled.addWriter(el::LogWriter::createForConsole(el::application().terminal()), traceFilter);
    manager->setConfiguration(std::move(disabled));
    const auto log = manager->createStream("guild/route-search"_el, section);
    el::io::printLine("Initial state          : "_el, log->traceEnabled() ? "enabled"_el : "disabled"_el);

    auto enabled = el::LogConfiguration{};
    enabled.enableTraceSection(section).addWriter(
        el::LogWriter::createForConsole(el::application().terminal()), traceFilter);
    manager->setConfiguration(std::move(enabled));
    el::io::printLine("After enabling section : "_el, log->traceEnabled() ? "enabled"_el : "disabled"_el);

    disabled = el::LogConfiguration{};
    disabled.addWriter(el::LogWriter::createForConsole(el::application().terminal()), traceFilter);
    manager->setConfiguration(std::move(disabled));
    el::io::printLine("After replacing policy : "_el, log->traceEnabled() ? "enabled"_el : "disabled"_el);
    manager->shutdown();
}

}
