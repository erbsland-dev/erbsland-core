// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all_core.hpp>
#include <erbsland/all_err.hpp>
#include <erbsland/all_event.hpp>
#include <erbsland/all_options.hpp>
#include <erbsland/all_path.hpp>
#include <erbsland/all_stream.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/all_time.hpp>

#include <chrono>
#include <filesystem>

namespace demo {

using namespace el::text::literals;

/// Here we demonstrate how to enter the main loop if you derive from `Application`.
/// The whole logic is in this derived class, which is not recommended for a real application.
class FileTimeMonitorApp : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override { info().setApplicationName("File Time Monitor"_el); }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->addOption("file"_el).setRequired().setHelp("The path to the file to monitor"_el);
        options->addOption({"-t"_el, "--timeout"_el, "timeout"_el})
            .setType(el::OptionType::Integer)
            .setHelp("The timeout in seconds. Zero for no timeout."_el)
            .setDefaultValue(5);
    }
    [[nodiscard]] auto main() -> el::ExitCode override {
        _path = el::Path::fromNativeOrThrow(optionValues()->getText("file"_el));
        _pathInfo = _path.info();
        if (!_path.info().exists()) {
            throw el::ApplicationError{"File does not exist."_el};
        }
        const auto timeout = el::Seconds{optionValues()->getInteger("timeout"_el)};
        if (timeout.isPositive()) {
            events()->invokeAfter(timeout, []() -> void { el::application().quit(); });
        }
        _pollTimer = events()->createTimer([this]() -> void { poll(); });
        _pollTimer->startFixedDelay(el::Seconds{1});
        el::stdOut()->printLine("Monitoring file: ", _path);
        // at this point, enter the main event loop.
        return runEventLoop();
    }
    void poll() {
        _pathInfo.reload(el::PathInfoPart::Times);
        el::stdOut()->printLine("Last Modification: ", _pathInfo.lastModified().toIsoString());
    }

private:
    el::EventTimerPtr _pollTimer;
    el::Path _path;
    el::PathInfo _pathInfo;
};

}
