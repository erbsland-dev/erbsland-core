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

/// Choose the visible mark that tells readers text was shortened.
///
/// `setTruncationMark()` replaces the default ellipsis for first-line, character-count, total-line, and left-truncated
/// name formatting. The mark occupies part of a character-count budget and may also be empty.
void lineTruncationMarks() {
    const auto showMark = [](const el::String &label, el::String mark) -> void {
        el::io::printLine(label, ":"_el);
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{message}"_el)
            .setMessageTruncation(el::LogMessageTruncation::CharacterCount)
            .setMessageLimit(el::CpCount{36U})
            .setTruncationMark(std::move(mark));
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        manager->rootStream()->info("The northern observation platform is closing for the night."_el);
        manager->shutdown();
    };

    showMark("Ellipsis"_el, "…"_el);
    showMark("Text marker"_el, " [more]"_el);
    showMark("No marker"_el, ""_el);
}

}
