// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/ConsoleLogWriterOptions.hpp>
#include <erbsland/cterm/BlockStyle.hpp>
#include <erbsland/FileLogWriterOptions.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogFileMode.hpp>
#include <erbsland/LogLevel.hpp>
#include <erbsland/LogLineFormat.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/LogStream.hpp>
#include <erbsland/LogTraceSection.hpp>
#include <erbsland/LogWriter.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringJoins.hpp>

namespace demo {

using namespace el::text::literals;

/// Common terminal, logging, and command-line setup for the two mail applications.
class MailApplication : public el::Application {
public:
    using Application::Application;

protected: // application support
    /// Register the configuration and diagnostic options shared by both applications.
    /// @param options Application option collection.
    void addCommonOptions(const el::OptionsPtr &options) {
        addTextOption(options, "config"_el, "Read this configuration file instead of the executable's default."_el);
        options->addOption({"-v"_el, "--verbose"_el, "verbose"_el})
            .setType(el::OptionType::Flag)
            .setHelpTitle("Show TLS checkpoints and SMTP conversation."_el);
    }
    /// Register a text override without a default, preserving configuration precedence.
    /// @param options Application option collection.
    /// @param name Stable option identifier, with underscores converted to CLI hyphens.
    /// @param title Help text explaining the value.
    void addTextOption(const el::OptionsPtr &options, const el::String &name, const el::String &title) {
        options->addOption({el::join("--"_el, name.replacedAll("_"_el, "-"_el)), name})
            .setType(el::OptionType::Text)
            .setValueName("value"_el)
            .setHelpTitle(title);
    }
    /// Configure colored console logging and an optional append-only log file.
    /// @param values Parsed command-line options.
    /// @param file Optional resolved log path.
    void setupLogging(const el::OptionValuesPtr &values, const el::Path &file = {}) {
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(el::LogLineFormat{}.setPattern("{message}"_el));
        auto consoleOptions = el::ConsoleLogWriterOptions{};
        consoleOptions.setLineStyle(el::LogLevel::Trace, el::cterm::BlockStyle{el::cterm::fg::Cyan});
        configuration.addWriter(el::LogWriter::createForConsole(terminal(), consoleOptions));
        if (!file.isEmpty()) {
            configuration.addWriter(
                el::LogWriter::createForFile(el::FileLogWriterOptions{file}.setMode(el::LogFileMode::Append)));
        }
        if (values->getFlag("verbose"_el)) {
            configuration.enableTraceSection(el::LogTraceSection{"mail"_el});
        }
        log().setConfiguration(std::move(configuration));
        _mailLog = log().createStream("mail"_el, el::LogTraceSection{"mail"_el});
    }

protected:
    el::LogStreamPtr _mailLog; ///< Shared log stream with an opt-in trace section.
};

}
