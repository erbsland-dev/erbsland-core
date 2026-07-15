// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

namespace demo {

using namespace el::text::literals;

/// Derive from `Application` when command line options belong to the executable lifecycle.
///
/// Register options in `registerCommandLineOptions()`, then read the parsed `OptionValues` in `main()`.
/// `Application` handles `--help`, `-h`, `--version`, option errors, and terminal rendering before your main function
/// is called.
class SpectrometerApp final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        info().setApplicationName("Prism Laboratory"_el);
        info().setApplicationVersion(el::Version{0, 8, 0});
        info().setAuthorName("Erbsland DEV"_el);
        info().setLicenseText("Apache-2.0"_el);
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpTitle("Prism Laboratory"_el);
        options->setHelpDescription("Configures a short measurement with a bench spectrometer."_el);
        options->setHelpEpilog(
            "Please use the correct names from the manual and the research datasheet "
            "to select the instruments and samples with the command line options."_el);
        options->addOption({"-i"_el, "--instrument"_el, "instrument"_el})
            .setType(el::OptionType::Text)
            .setRequired()
            .setHelpDescription("Name of the instrument that will perform the measurement."_el);
        options->addOption({"-g"_el, "--gain"_el, "gain"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(el::OptionInteger{2})
            .setHelpDescription("Detector gain between 1 and 9."_el);
        options->addOption({"-d"_el, "--dark-frame"_el, "dark-frame"_el})
            .setType(el::OptionType::Flag)
            .setHelpDescription("Captures a dark frame before measuring the sample."_el);
        options->addOption("sample"_el)
            .setRequired()
            .setHelpDescription("Sample placed in front of the instrument."_el);
    }
    [[nodiscard]] auto main() -> el::ExitCode override {
        const auto values = optionValues();
        el::io::printLine("instrument: "_el, values->getText("instrument"_el));
        el::io::printLine("sample: "_el, values->getText("sample"_el));
        el::io::printLine("gain: "_el, values->getInteger("gain"_el));
        el::io::printLine("dark frame: "_el, el::BooleanFormat::yesNo(), values->getFlag("dark-frame"_el));
        return el::ExitCode::success();
    }
};

/// This simple main method creates the application and lets it run the full lifecycle.
auto main(const int argc, char *argv[]) -> int {
    auto app = SpectrometerApp{argc, argv};
    app.enableTerminal();
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
