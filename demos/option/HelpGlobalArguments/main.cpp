// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

namespace demo {

using namespace el::text::literals;

/// This small executable only registers a handful of global options for help-output testing.
class HelpGlobalArgumentsApp final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        enableTerminal();
        info().setApplicationName("Prism Bench"_el);
        info().setApplicationVersion(el::Version{0, 8, 0});
        info().setAuthorName("Erbsland DEV"_el);
        info().setLicenseText("Apache-2.0"_el);
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpTitle("Prism Bench"_el);
        options->setHelpDescription("Prepares a short measurement with bench optical instruments."_el);
        options->addOption({"-q"_el, "--quiet"_el, "quiet"_el})
            .setHelpDescription("Reduces output to essential values."_el);
        options->addOption({"-c"_el, "--count"_el, "count"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(el::OptionInteger{3})
            .setHelpDescription("Number of readings to take."_el);
        options->addOption({"-m"_el, "--mode"_el, "mode"_el})
            .addChoice("fast"_el)
            .addChoice("precise"_el)
            .addChoice("night"_el)
            .setDefaultValue(el::String{"precise"_el})
            .setHelpDescription("Acquisition mode for the optical bench."_el);
        options->addOption("sample"_el).setRequired().setHelpDescription("Sample placed in front of the prism."_el);
    }
    [[nodiscard]] auto main() -> el::ExitCode override { return el::ExitCode::success(); }
};

}

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::HelpGlobalArgumentsApp{argc, argv};
    return app.run();
}
