// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>
#include <erbsland/log/line/all.hpp>

#include <memory>

namespace demo {

/// Paragraph options control how complete log lines use the available terminal width.
///
/// The console writer accepts the complete `ParagraphOptions` object. Indents distinguish the first physical line
/// from wrapped continuations, while a maximum wrap count and ellipsis keep an unusually long entry bounded.
void consoleWriterParagraphOptions() {
    auto paragraph = el::cterm::ParagraphOptions{};
    paragraph.setLineIndent(2);
    paragraph.setFirstLineIndent(4);
    paragraph.setWrappedLineIndent(8);
    paragraph.setMaximumLineWraps(1);
    paragraph.setParagraphEllipsisMark(el::cterm::BlockStringEditor{"…"_el});

    auto writerOptions = el::ConsoleLogWriterOptions{};
    writerOptions.setParagraphOptions(std::move(paragraph));
    auto format = el::LogLineFormat{};
    format.setPattern("{message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

    auto &manager = el::application().log();
    manager.setConfiguration(std::move(configuration));
    manager.rootStream()->info(
        "The northern survey report contains a long sequence of observations that should remain compact on an "_el,
        "operator's terminal even when the available line width is limited and several route notes follow."_el);
}

/// A base line style establishes the visual foundation shared by every severity and semantic part.
///
/// Level and part styles are overlays, so any foreground, background, or attribute they leave inherited continues to
/// come from this base style.
void consoleWriterBaseStyle() {
    auto writerOptions = el::ConsoleLogWriterOptions{};
    writerOptions.setBaseLineStyle(el::cterm::BlockStyle{el::cterm::fg::BrightWhite, el::cterm::bg::Blue});
    auto format = el::LogLineFormat{};
    format.setPattern("{level} {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

    auto &manager = el::application().log();
    manager.setConfiguration(std::move(configuration));
    manager.rootStream()->info("The base style supplies the blue background."_el);
}

/// Level styles make severities recognizable without changing the text of the formatted log line.
///
/// Each call replaces the complete overlay for one level. Unspecified color components and attributes continue to
/// inherit from the base line style.
void consoleWriterLevelStyles() {
    auto writerOptions = el::ConsoleLogWriterOptions{};
    writerOptions.setLineStyle(el::LogLevel::Information, el::cterm::BlockStyle{el::cterm::fg::BrightGreen})
        .setLineStyle(
            el::LogLevel::Warning, el::cterm::BlockStyle{el::cterm::fg::BrightYellow, el::cterm::BlockAttributes::Bold})
        .setLineStyle(
            el::LogLevel::Error,
            el::cterm::BlockStyle{el::cterm::fg::BrightRed, el::cterm::BlockAttributes::Underline});
    auto format = el::LogLineFormat{};
    format.setPattern("{level} {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

    auto &manager = el::application().log();
    manager.setConfiguration(std::move(configuration));
    const auto log = manager.rootStream();
    log->info("The expedition registry opened."_el);
    log->warn("The western trail report is overdue."_el);
    log->error("The emergency beacon did not answer."_el);
}

/// Part styles emphasize one semantic field across all log levels.
///
/// A line pattern preserves the identity of timestamp, level, name, message, and literal segments. The console writer
/// uses that identity to style fields even though they have already been assembled into one displayed line.
void consoleWriterPartStyles() {
    auto writerOptions = el::ConsoleLogWriterOptions{};
    writerOptions.setPartStyle(el::LogLinePart::Level, el::cterm::BlockStyle{el::cterm::fg::BrightGreen})
        .setPartStyle(el::LogLinePart::Name, el::cterm::BlockStyle{el::cterm::fg::BrightCyan})
        .setPartStyle(el::LogLinePart::Message, el::cterm::BlockStyle{el::cterm::BlockAttributes::Italic});
    auto format = el::LogLineFormat{};
    format.setPattern("{level} [{name}] {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

    auto &manager = el::application().log();
    manager.setConfiguration(std::move(configuration));
    manager.createStream("guild/archive"_el)->info("The route ledger was indexed."_el);
}

/// A part-and-level style is the final overlay and changes one field only for one severity.
///
/// This is useful when the complete error line should remain calm while the actual error message receives the strongest
/// emphasis.
void consoleWriterLevelPartStyles() {
    auto writerOptions = el::ConsoleLogWriterOptions{};
    writerOptions.setLineStyle(el::LogLevel::Error, el::cterm::BlockStyle{el::cterm::fg::BrightWhite})
        .setPartStyle(el::LogLinePart::Name, el::cterm::BlockStyle{el::cterm::fg::BrightCyan})
        .setPartStyle(
            el::LogLinePart::Message,
            el::LogLevel::Error,
            el::cterm::BlockStyle{el::cterm::fg::BrightRed, el::cterm::BlockAttributes::Bold});
    auto format = el::LogLineFormat{};
    format.setPattern("{level} [{name}] {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .addWriter(el::LogWriter::createForConsole(el::application().terminal(), writerOptions));

    auto &manager = el::application().log();
    manager.setConfiguration(std::move(configuration));
    manager.createStream("guild/dispatch"_el)->error("No guide is available for the eastern pass."_el);
}

}
