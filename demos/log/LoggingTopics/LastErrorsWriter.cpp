// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>

namespace demo {

/// A last-errors writer keeps a bounded, thread-safe snapshot of the most recent error entries.
///
/// Information and warning entries are ignored. When the capacity is reached, the oldest retained error is removed,
/// which makes the snapshot suitable for a concise failure report at the application boundary.
void lastErrorsWriter() {
    const auto lastErrors = std::make_shared<el::LastErrorsLogWriter>(2U);
    auto configuration = el::LogConfiguration{};
    configuration.addWriter(lastErrors);

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->createStream("guild/expedition"_el);
    log->info("The expedition entered the northern pass."_el);
    log->error("The bridge marker could not be found."_el);
    log->warn("Snowfall is reducing visibility."_el);
    log->error("The reserve compass failed its check."_el);
    log->error("The return route is blocked by ice."_el);
    manager->shutdown();

    el::io::printLine("Retained errors:"_el);
    for (const auto &entry : lastErrors->snapshot()) {
        el::io::printLine("  "_el, entry->message());
    }
}

}
