// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Select how much of a hierarchical stream path appears in `{name}`.
///
/// `setNameFormat()` can preserve the full path, keep its leaf, combine its first and final segments, or retain the
/// right side of a long path within the default 40-character name limit.
void lineNameFormats() {
    const auto showName = [](const el::String &label, const el::LogNameFormat nameFormat) -> void {
        el::io::printLine(label, ":"_el);
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{name}: {message}"_el).setNameFormat(nameFormat);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        manager->createStream("guild/exploration/finland/lapland/northern-lights"_el)->info("Survey opened."_el);
        manager->shutdown();
    };

    showName("Full path"_el, el::LogNameFormat::Full);
    showName("Leaf segment"_el, el::LogNameFormat::Leaf);
    showName("First and final segments"_el, el::LogNameFormat::HeadAndLeaf);
    showName("Left-truncated path"_el, el::LogNameFormat::LeftTruncated);
}

}
