// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/all_log.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogLineFormat.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/LogMessageTruncation.hpp>
#include <erbsland/LogWriter.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

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
            .setMessageLimit(el::CpCount{56U});
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
