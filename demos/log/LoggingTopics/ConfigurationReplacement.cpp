// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Replace the active logging policy with one complete configuration snapshot.
///
/// Existing streams remain valid. `setConfiguration()` returns only after their cached trace flags reflect the new
/// trace sections and routes, so guarded diagnostic preparation can use the new policy immediately.
void configurationReplacement() {
    auto &manager = el::application().log();
    const auto log = manager.createStream("guild/routes"_el, el::LogTraceSection{"route-search"_el});
    el::io::printLine("Trace before replacement: "_el, el::BooleanFormat::yesNo(), log->traceEnabled());

    auto lineFormat = el::LogLineFormat{};
    lineFormat.setPattern("{level} [{name}] {message}"_el);
    auto replacement = el::LogConfiguration{};
    replacement.setLineFormat(std::move(lineFormat))
        .enableTraceSection(el::LogTraceSection{"route-search"_el})
        .addWriter(std::make_shared<el::ConsoleLogWriter>(el::application().terminal()));
    manager.setConfiguration(std::move(replacement));

    el::io::printLine("Trace after replacement: "_el, el::BooleanFormat::yesNo(), log->traceEnabled());
    log->trace("Prepared the detailed ridge-route comparison."_el);
}

}
