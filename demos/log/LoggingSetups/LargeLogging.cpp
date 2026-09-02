// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>
#include <erbsland/log/all.hpp>
#include <erbsland/path/all.hpp>

#include <memory>
#include <utility>

namespace demo {

using namespace el::text::literals;

/// Give a large or long-running application one deliberate logging policy.
///
/// This simplified service writes its operational history to a file, enables one diagnostic trace section, and
/// retains recent errors. Because `main()` reports failure, the retained error is repeated on the console under a
/// localized heading while the detailed trace remains in the file.
class LargeLoggingApplication final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        auto temporaryOptions = el::PathTempDirectoryOptions{};
        temporaryOptions.setPrefix("explorer-guild-"_el).setRandomLength(el::CpLength{8U});
        _temporary = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);

        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{level} [{name}] {message}"_el);
        auto fileOptions = el::FileLogWriterOptions{_temporary->path() / "service.log"_el};
        fileOptions.setMode(el::LogFileMode::Overwrite);

        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .enableTraceSection(el::LogTraceSection{"route-search"_el})
            .addWriter(el::LogWriter::createForFile(fileOptions));
        log().setConfiguration(std::move(configuration));

        enableLastErrorDump();
        _log = log().createStream("guild/expedition"_el, el::LogTraceSection{"route-search"_el});
    }

    [[nodiscard]] auto main() -> el::ExitCode override {
        if (_log->traceEnabled()) {
            _log->trace("Candidate route: Turku → Jääjärvi → Majakka"_el);
        }
        _log->info("Expedition 'Revontuli' departed."_el);
        _log->error("The northern checkpoint did not answer."_el);
        return el::ExitCode{2};
    }

    void cleanup() noexcept override { log().shutdown(); }

private:
    el::TempDirectoryPtr _temporary;
    el::LogStreamPtr _log;
};

auto runLargeLogging(const int argc, char *argv[]) -> int {
    auto app = LargeLoggingApplication{argc, argv};
    return app.run();
}

}
