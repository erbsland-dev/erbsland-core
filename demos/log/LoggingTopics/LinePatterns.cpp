// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Arrange semantic log fields with a validated placeholder pattern.
///
/// `setPattern()` accepts time, level, name, and message placeholders. A compact command can show only the message,
/// while larger applications often add level and name. Doubled braces produce literal braces in the output.
void linePatterns() {
    const auto showPattern = [](const el::String &label, el::String pattern) -> void {
        el::io::printLine(label, ":"_el);
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern(std::move(pattern));
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        manager->createStream("guild/routes"_el)->info("Selected the ridge route."_el);
        manager->shutdown();
    };

    showPattern("Message only"_el, "{message}"_el);
    showPattern("Level and name"_el, "{level} [{name}] {message}"_el);
    showPattern("Literal braces"_el, "{{guild}} {level}: {message}"_el);
}

}
