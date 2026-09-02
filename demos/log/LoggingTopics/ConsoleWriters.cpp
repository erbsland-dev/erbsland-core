// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>

namespace demo {

/// A console writer renders formatted log lines as terminal paragraphs.
///
/// Create its options, pass them to the writer together with the application terminal, and add the writer to the
/// complete log configuration. The application owns and shuts down its log manager automatically.
void consoleWriters() {
    auto paragraph = el::cterm::ParagraphOptions{};
    paragraph.setWrappedLineIndent(4);

    auto writerOptions = el::ConsoleLogWriterOptions{};
    writerOptions.setParagraphOptions(std::move(paragraph));

    auto format = el::LogLineFormat{};
    format.setPattern("{level} [{name}] {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

    auto &manager = el::application().log();
    manager.setConfiguration(std::move(configuration));
    const auto log = manager.createStream("guild/weather"_el);
    log->info("Northern ridge observation opened."_el);
    log->warn("A snow squall is crossing the ridge."_el);
}

}
