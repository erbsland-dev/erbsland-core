// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Choose a compact or descriptive spelling for severity levels.
///
/// `setLevelFormat()` affects only the text inserted by `{level}`. The entry keeps the same warning level, so routing
/// and console styling remain unchanged across all four representations.
void lineLevelFormats() {
    const auto showLevel = [](const el::String &label, const el::LogLevelFormat levelFormat) -> void {
        el::io::printLine(label, ":"_el);
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{level}: {message}"_el).setLevelFormat(levelFormat);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(std::make_shared<el::ConsoleLogWriter>(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        manager->rootStream()->warn("One route marker needs repainting."_el);
        manager->shutdown();
    };

    showLevel("Three uppercase letters"_el, el::LogLevelFormat::ThreeLetterUpper);
    showLevel("Three lowercase letters"_el, el::LogLevelFormat::ShortLower);
    showLevel("Full lowercase name"_el, el::LogLevelFormat::FullLower);
    showLevel("Full uppercase name"_el, el::LogLevelFormat::FullUpper);
}

}
