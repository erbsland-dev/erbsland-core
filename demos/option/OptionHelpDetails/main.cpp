// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoTerminal.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/BooleanFormat.hpp>
#include <erbsland/ByteIntegerAccess.hpp>
#include <erbsland/conf/Integer.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Option.hpp>
#include <erbsland/OptionChoice.hpp>
#include <erbsland/OptionChoices.hpp>
#include <erbsland/OptionEditor.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionErrorContext.hpp>
#include <erbsland/OptionFlag.hpp>
#include <erbsland/OptionHelp.hpp>
#include <erbsland/OptionHelpVisibility.hpp>
#include <erbsland/OptionInteger.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValue.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/pattern/Text.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/Time.hpp>
#include <erbsland/Version.hpp>

namespace demo {

using namespace el::text::literals;

/// Build a choice list when individual choices need their own help metadata.
///
/// `OptionEditor::addChoice()` is enough for simple choices.
/// Create `OptionChoice` objects when choices need descriptions or visibility settings of their own.
/// Hidden choices remain accepted by the parser, but are omitted from generated help output.
[[nodiscard]] auto activityChoices() -> el::OptionChoicesPtr {
    auto choices = el::OptionChoices::create();

    choices->addChoice(el::OptionChoice::create("dusk"_el, el::OptionHelp{"Activity around dusk."_el}));
    choices->addChoice(el::OptionChoice::create("night"_el, el::OptionHelp{"Activity in full darkness."_el}));
    choices->addChoice(el::OptionChoice::create("dawn"_el, el::OptionHelp{"Activity shortly before sunrise."_el}));

    auto internalHelp = el::OptionHelp{"Internal test choice that does not appear in regular help."_el};
    internalHelp.setVisibility(el::OptionHelpVisibility::Hidden);
    choices->addChoice(el::OptionChoice::create("internal"_el, internalHelp));
    return choices;
}

/// Option definitions combine command-line names, value types, flags, choices, defaults, validation, and help text.
///
/// `OptionEditor` is returned from `addOption()`, so each option can be declared in one fluent expression.
/// Dashed names are accepted on the command line, while dashless names are lookup aliases for `OptionValues`.
/// Choices may carry their own help text, and visibility controls where an enabled option appears in generated help.
class NightWatchApp final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        DemoTerminal::enable(*this);
        info().setApplicationName("Night Watch"_el);
        info().setApplicationVersion(el::Version{0, 8, 0});
    }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpTitle("Night Watch"_el);
        options->setHelpDescription("Records short observations of nocturnal animal behavior."_el);
        options->setHelpEpilog("Hidden options remain usable, but do not appear in this help."_el);
        options->addOption({"-s"_el, "--species"_el, "species"_el})
            .setType(el::OptionType::Text)
            .setValueName("animal"_el)
            .setRequired()
            .setHelpTitle("Animal species tracked during the round."_el)
            .setHelpVisibility(el::OptionHelpVisibility::Usage);
        options->addOption({"-f"_el, "--phase"_el, "phase"_el})
            .setChoices(activityChoices())
            .setDefaultValue("night"_el)
            .setHelpTitle("Time window in which the observation occurs."_el);
        options->addOption({"-r"_el, "--round"_el, "round"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(el::OptionInteger{2})
            .setValidateFn([](const el::OptionValuePtr &value, el::OptionValuesPtr) -> void {
                if (value->getInteger() < 1 || value->getInteger() > 6) {
                    auto context = el::OptionErrorContext{};
                    context.setTitle("Invalid round count"_el)
                        .setDescription("The round count must be between 1 and 6."_el);
                    throw el::OptionError{context};
                }
            })
            .setHelpTitle("Number of observation rounds in this area."_el);
        options->addOption({"-q"_el, "--quiet"_el, "quiet"_el})
            .setHelpTitle("Writes only the final result."_el)
            .setHelpVisibility(el::OptionHelpVisibility::Hidden);
        options->addOption({"--old-log"_el, "old-log"_el})
            .setFlag(el::OptionFlag::Disabled)
            .setHelpTitle("Old logging option that is no longer accepted."_el);
        options->addOption("area"_el).setRequired().setHelpTitle("Area where the observation takes place."_el);
    }
    [[nodiscard]] auto main() -> el::ExitCode override {
        const auto values = optionValues();
        el::io::printLine("species: "_el, values->getText("species"_el));
        el::io::printLine("area: "_el, values->getText("area"_el));
        el::io::printLine("phase: "_el, values->getText("phase"_el));
        el::io::printLine("rounds: "_el, values->getInteger("round"_el));
        el::io::printLine("quiet: "_el, el::BooleanFormat::yesNo(), values->getFlag("quiet"_el));
        return el::ExitCode::success();
    }
};

}

auto main(const int argc, char *argv[]) -> int {
    auto app = demo::NightWatchApp{argc, argv};
    return app.run();
}
