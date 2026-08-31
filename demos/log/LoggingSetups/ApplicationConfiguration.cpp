// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

using namespace el::text::literals;

/// Install a fixed logging policy while the application is being initialized.
///
/// `Application` already owns the log manager and shuts it down at the end of its lifecycle. The application only
/// assembles the line format, enabled trace sections, and writer routes that form its complete logging policy.
class ApplicationConfiguration final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        auto &manager = log();
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{level} [{name}] {message}"_el);

        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .enableTraceSection(el::LogTraceSection{"route-search"_el})
            .addWriter(std::make_shared<el::ConsoleLogWriter>(terminal()));
        manager.setConfiguration(std::move(configuration));

        _log = manager.createStream("guild/routes"_el, el::LogTraceSection{"route-search"_el});
    }

    [[nodiscard]] auto main() -> el::ExitCode override {
        if (_log->traceEnabled()) {
            _log->trace("Compared the ridge and lake routes."_el);
        }
        _log->info("Selected the ridge route for 'Revontuli'."_el);
        return el::ExitCode::success();
    }

private:
    el::LogStreamPtr _log;
};

auto runApplicationConfiguration(const int argc, char *argv[]) -> int {
    auto app = ApplicationConfiguration{argc, argv};
    return app.run();
}

}
