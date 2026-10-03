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

/// Limit the rendered message while leaving the other pattern fields outside the budget.
///
/// Select `CharacterCount` with `setMessageTruncation()`, then pass the code-point budget to `setMessageLimit()`. A
/// zero message limit disables this shortening rule.
void lineMessageLimits() {
    const auto showLimit = [](const el::String &label, const el::CpCount limit) -> void {
        el::io::printLine(label, ":"_el);
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{level} [{name}] {message}"_el)
            .setMessageTruncation(el::LogMessageTruncation::CharacterCount)
            .setMessageLimit(limit);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        manager->createStream("guild/weather"_el)
            ->warn("A snow front will reach the northern platform before dusk."_el);
        manager->shutdown();
    };

    showLimit("48 message characters"_el, el::CpCount{48U});
    showLimit("24 message characters"_el, el::CpCount{24U});
    showLimit("Unlimited message"_el, el::CpCount::zero());
}

}
