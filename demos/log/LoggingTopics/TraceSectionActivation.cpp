// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Require an enabled section and a matching trace route.
///
/// These three independent managers show the two-part rule without changing configuration at runtime. A stream becomes
/// trace-enabled only when the section is listed and a writer accepts trace entries from its path.
void traceSectionActivation() {
    const auto showState = [](const el::String &label, const bool enableSection, const bool acceptTrace) -> void {
        const auto section = el::LogTraceSection{"route-search"_el};
        auto configuration = el::LogConfiguration{};
        if (enableSection) {
            configuration.enableTraceSection(section);
        }
        const auto levels = acceptTrace ? el::LogLevels{el::LogLevel::Trace} : el::LogLevels{el::LogLevel::Information};
        configuration.addWriter(std::make_shared<el::LastErrorsLogWriter>(), el::LogWriterFilter{levels});

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
