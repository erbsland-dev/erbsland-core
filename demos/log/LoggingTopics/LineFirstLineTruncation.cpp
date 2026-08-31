// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Choose whether multiline messages remain complete or stop after their first line.
///
/// `LogMessageTruncation::None` is the default and preserves line breaks. `FirstLine` keeps the text before the first
/// newline and appends the configured truncation mark without consulting the message limit.
void lineFirstLineTruncation() {
    const auto showMessage = [](const el::String &label, const el::LogMessageTruncation truncation) -> void {
        el::io::printLine(label, ":"_el);
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{level}: {message}"_el).setMessageTruncation(truncation);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(std::make_shared<el::ConsoleLogWriter>(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        manager->rootStream()->info("Cloud cover is increasing.\nThe ridge team will wait below the summit."_el);
        manager->shutdown();
    };

    showMessage("Complete message"_el, el::LogMessageTruncation::None);
    showMessage("First line only"_el, el::LogMessageTruncation::FirstLine);
}

}
