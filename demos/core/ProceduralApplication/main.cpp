// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

namespace demo {

using namespace el::text::literals;

/// Override `Application::main()` for a synchronous workflow with one clear result.
///
/// `Application` initializes the process, parses the command line, and handles framework errors before and after this
/// method. The overridden main method reads the validated values and returns the process exit code.
class ReportApplication final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override {
        info().setApplicationName("Deployment Report"_el);
        info().setApplicationVersion(el::Version{1, 0, 0});
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->addOption("environment"_el)
            .setRequired()
            .setHelpDescription("Environment summarized by the report."_el);
        options->addOption({"-d"_el, "--dry-run"_el, "dry-run"_el})
            .setHelpDescription("Marks the report as a simulation."_el);
    }
    [[nodiscard]] auto main() -> el::ExitCode override {
        el::io::printLine("environment: "_el, optionValues()->getText("environment"_el));
        el::io::printLine("mode: "_el, optionValues()->getFlag("dry-run"_el) ? "simulation"_el : "deployment"_el);
        return el::ExitCode::success();
    }
};

/// Create the procedural application and run its complete lifecycle.
auto main(const int argc, char *argv[]) -> int {
    auto app = ReportApplication{argc, argv};
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
