// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/conf/all.hpp>
#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/cterm/all.hpp>
#include <erbsland/stream/StandardStreams.hpp>

#include <optional>

namespace demo {

/// Interactive developer utility for experimenting with every `ReadLineOptions` setting.
///
/// The application combines Erbsland Core command-line options, ELCL configuration, the event scheduler, and the
/// non-blocking terminal line editor. A live clock above the input area demonstrates that polling the editor does not
/// block the application event loop.
class ReadLineApp final : public el::Application {
public:
    using Application::Application;

protected:
    /// Configure application metadata and enable advanced terminal output.
    void initialize() override;
    /// Register the complete command-line interface.
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    /// Start the non-blocking line editor.
    [[nodiscard]] auto main() -> el::ExitCode override;
    /// Stop active timers and input safely.
    void cleanup() noexcept override;

private:
    [[nodiscard]] auto hasCommandLineValue(const el::String &name) const -> bool;
    void applyConfiguration(el::cterm::ReadLineOptions &settings);
    void applyCommandLine(el::cterm::ReadLineOptions &settings);
    void dumpConfiguration(const el::cterm::ReadLineOptions &settings);
    void validateConfigurationKeys(const el::conf::DocumentPtr &document) const;
    [[nodiscard]] static auto configurationValue(const el::conf::DocumentPtr &document, const el::String &path)
        -> el::conf::ValuePtr;
    [[noreturn]] static void throwConfigurationError(
        el::String title,
        el::String description,
        const el::conf::ValuePtr &value = {},
        const std::exception_ptr &cause = {});

private: // value parsing
    [[nodiscard]] static auto equalsCI(const el::String &left, const el::String &right) noexcept -> bool;
    [[nodiscard]] static auto parseDisplayStyle(const el::String &text) -> el::cterm::ReadLineDisplayStyle;
    [[nodiscard]] static auto parseFrameStyle(const el::String &text) -> el::cterm::FrameStyle;
    [[nodiscard]] static auto parseBlock(const el::String &text, el::cterm::BlockStyle style = {}) -> el::cterm::Block;
    [[nodiscard]] static auto parseKey(const el::String &text) -> el::cterm::Key;
    static void applyFrameStyle(
        el::cterm::FrameBorder &border, el::cterm::FrameBorder::Element element, const el::String &text);
    static void applyUniformFrameStyle(el::cterm::FrameBorder &border, el::cterm::FrameStyle style);
    static void applyUniformFrameColor(el::cterm::FrameBorder &border, el::cterm::Color color);

private: // event loop
    void startReadLine(el::cterm::ReadLineOptions settings);
    void updateReadLine();
    void writeClock(bool initial);
    void finishReadLine(const el::cterm::ReadLineResult &result);
    [[nodiscard]] static auto statusText(const el::cterm::ReadLineStatus &status) -> el::String;

private:
    el::cterm::ReadLinePtr _readLine;
    el::EventTimerPtr _updateTimer;
    el::String _lastClockText;
};

}
