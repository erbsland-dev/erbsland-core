// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all_log.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogLineFormat.hpp>
#include <erbsland/LogWriter.hpp>
#include <erbsland/OptionValues.hpp>

#include <memory>
#include <utility>

namespace demo {

using namespace el::text::literals;

/// Retain startup entries until the final logging policy is installed after command-line parsing.
///
/// Pausing is the first initialization action, before any startup message is written. After options and configuration
/// are available, the application installs the complete snapshot and resumes delivery through the new writer route.
class DeferredConfiguration final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override {
        log().pause();
        logStream()->info("Application initialization started."_el);
    }

    void handleCommandLineOptions([[maybe_unused]] const el::OptionValuesPtr &values) override {
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("configured: {level} - {message}"_el);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat)).addWriter(el::LogWriter::createForConsole(terminal()));
        log().setConfiguration(std::move(configuration));

        logStream()->info("Logging configuration loaded."_el);
        log().resume();
    }

    [[nodiscard]] auto main() -> el::ExitCode override {
        logStream()->info("Explorer registry opened."_el);
        return el::ExitCode::success();
    }
};

auto runDeferredConfiguration(const int argc, char *argv[]) -> int {
    auto app = DeferredConfiguration{argc, argv};
    return app.run();
}

}
