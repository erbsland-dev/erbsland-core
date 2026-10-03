// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoTerminal.hpp>
#include <erbsland/all_core.hpp>
#include <erbsland/all_err.hpp>
#include <erbsland/all_event.hpp>
#include <erbsland/all_options.hpp>
#include <erbsland/all_path.hpp>
#include <erbsland/all_stream.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/all_time.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathResolveMode.hpp>

namespace demo {

using namespace el::text::literals;

/// Initialize resources, enter the main event loop, and release resources when it returns.
/// `PathInfo` reloads filesystem timestamps for each timer callback.
/// @notest{Verified by running the file_time_monitor demo.}
class FileTimeMonitorApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override {
        DemoTerminal::enable(*this);
        info().setApplicationName("File Time Monitor"_el);
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->addOption("file"_el).setRequired().setHelp("The path to the file to monitor"_el);
        options->addOption({"-t"_el, "--timeout"_el, "timeout"_el})
            .setType(el::OptionType::Integer)
            .setHelp("The timeout in seconds. Zero for no timeout."_el)
            .setDefaultValue(5);
    }
    [[nodiscard]] auto main() -> el::ExitCode override {
        _path = el::Path::fromNativeOrThrow(optionValues()->getText("file"_el))
                    .resolveOrThrow(el::PathResolveMode::Physical);
        _pathInfo = _path.info(el::PathInfoPart::Type);
        if (!_pathInfo.isRegularFile()) {
            throw el::ApplicationError{"The path must name a regular file."_el};
        }
        const auto timeout = el::Seconds{optionValues()->getInteger("timeout"_el)};
        if (timeout.isNegative()) {
            throw el::ApplicationError{"The timeout must be non-negative."_el};
        }
        if (timeout.isPositive()) {
            events()->invokeAfter(timeout, []() -> void { el::application().quit(); });
        }
        _pollTimer = events()->createTimer([this]() -> void { poll(); });
        _pollTimer->startFixedDelay(el::Seconds{1});
        el::stdOut()->printLine("Monitoring file: "_el, _path);
        // Enter the event loop after setup; execution resumes here when it exits.
        const auto exitCode = runEventLoop();
        _pollTimer->stop();
        el::stdOut()->printLine("Monitoring stopped."_el);
        return exitCode;
    }
    void cleanup() noexcept override { _pollTimer.reset(); }

private:
    void poll() {
        _pathInfo.reload(el::PathInfoPart::Times);
        el::stdOut()->printLine("Last modification: "_el, _pathInfo.lastModified().toString());
    }

private:
    el::EventTimerPtr _pollTimer; ///< Retains the repeating timer.
    el::Path _path;               ///< File being monitored.
    el::PathInfo _pathInfo;       ///< Reloadable filesystem information.
};

}
