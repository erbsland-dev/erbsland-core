// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Limit the complete rendered line by giving the message the space left by fixed fields.
///
/// `TotalLineLength` counts pattern literals, the level, the rendered name, and every message occurrence against the
/// `setMessageLimit()` budget. A richer prefix therefore leaves fewer characters for the message.
void lineTotalLimits() {
    const auto showPattern = [](const el::String &label, el::String pattern) -> void {
        el::io::printLine(label, ":"_el);
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern(std::move(pattern))
            .setMessageTruncation(el::LogMessageTruncation::TotalLineLength)
            .setMessageLimit(el::CpLength{56U});
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        manager->createStream("guild/weather"_el)
            ->warn("A snow front will reach the northern platform before dusk."_el);
        manager->shutdown();
    };

    showPattern("Message-only pattern"_el, "{message}"_el);
    showPattern("Level-and-name pattern"_el, "{level} [{name}] {message}"_el);
}

}
