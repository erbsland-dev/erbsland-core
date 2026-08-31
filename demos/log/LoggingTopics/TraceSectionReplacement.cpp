// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Show the synchronous flag refresh caused by configuration replacement.
///
/// Runtime replacement is possible, but each call installs a complete manager policy. Immediately after
/// `setConfiguration()` returns, existing streams expose the new section-and-route decision through `traceEnabled()`.
void traceSectionReplacement() {
    const auto section = el::LogTraceSection{"route-search"_el};
    const auto writer = std::make_shared<el::LastErrorsLogWriter>();
    const auto traceFilter = el::LogWriterFilter{el::LogLevels{el::LogLevel::Trace}};
    const auto manager = el::LogManager::create();

    auto disabled = el::LogConfiguration{};
    disabled.addWriter(writer, traceFilter);
    manager->setConfiguration(std::move(disabled));
    const auto log = manager->createStream("guild/route-search"_el, section);
    el::io::printLine("Initial state          : "_el, log->traceEnabled() ? "enabled"_el : "disabled"_el);

    auto enabled = el::LogConfiguration{};
    enabled.enableTraceSection(section).addWriter(writer, traceFilter);
    manager->setConfiguration(std::move(enabled));
    el::io::printLine("After enabling section : "_el, log->traceEnabled() ? "enabled"_el : "disabled"_el);

    disabled = el::LogConfiguration{};
    disabled.addWriter(writer, traceFilter);
    manager->setConfiguration(std::move(disabled));
    el::io::printLine("After replacing policy : "_el, log->traceEnabled() ? "enabled"_el : "disabled"_el);
    manager->shutdown();
}

}
