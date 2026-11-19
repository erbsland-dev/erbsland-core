// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"
#include "../i18n/DisplayTextTestLayer.hpp"

#include <erbsland/DisplayText.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionChoices.hpp>
#include <erbsland/OptionFlag.hpp>
#include <erbsland/OptionHelpVisibility.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/OptionModule.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionSet.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/TextNode.hpp>
#include <erbsland/TextNodeType.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::options;
using namespace el::text::literals;
using el::text::String;
using NodeType = el::text::TextNodeType;

TESTED_TARGETS(OptionDocumentBuilder OptionDisplayModel)
class OptionHelpResultsTest final : public el::UnitTest {
private:
    static constexpr auto cCOption = "-c"_el;

public:
    void testPartialOptionsAreGroupedByScopeWithoutCommands() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        options->addOption({"--config"_el, "config"_el})
            .setType(OptionType::Text)
            .setValueName("file"_el)
            .setDefaultValue("settings.conf"_el)
            .setHelp("Read configuration"_el);
        auto other = OptionModule::create("generate-other"_el);
        other->addOption({cCOption, "--check"_el, "check"_el}).setHelp("Check other output"_el);
        options->addModule(other);
        auto selected = OptionModule::create("generate-data"_el);
        selected->addOption({cCOption, "--check"_el, "check"_el}).setHelp("Check generated output"_el);
        options->addModule(selected);
        auto manager = OptionManager{options};
        const auto document = manager.detailedHelpDocument("generate-data"_el, "c"_el);
        const auto text = document.toString();
        REQUIRE(text.startsWith("\nMatching help for \"c\"\n\nOptions for generate-data:\n  -c, --check"_el));
        REQUIRE(text.contains("Options:\n      --config <file>"_el));
        REQUIRE(text.contains("Options for generate-other:\n  -c, --check"_el));
        REQUIRE(text.find("Options for generate-data:"_el) < text.find("Options:"_el));
        REQUIRE(text.find("Options:"_el) < text.find("Options for generate-other:"_el));
        REQUIRE(!text.contains("tool"_el));
        REQUIRE(!text.contains("--help"_el));
        REQUIRE(!text.contains("settings.conf"_el));
        REQUIRE(document.root()->contains(NodeType::OptionShort));
        REQUIRE(document.root()->contains(NodeType::OptionLong));
        REQUIRE(document.root()->contains(NodeType::OptionMeta));
        REQUIRE(!document.root()->contains(NodeType::OptionExecutable));
        REQUIRE(!document.root()->contains(NodeType::OptionDetails));
    }

    void testChoicesUseTheirOwnersAndOnlyMatchedValues() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        auto choices = OptionChoices::create();
        choices->addChoice("Blue!"_el).setHelp("Use blue"_el).setHelpNames({"probe-blue"_el});
        choices->addChoice("Red!"_el).setHelpNames({"probe-red"_el});
        choices->addChoice("green"_el).setHelp("Unmatched green"_el);
        options->addOption({"-C"_el, "--color"_el}).setChoices(choices);
        auto localChoices = OptionChoices::create();
        localChoices->addChoice("Blue!"_el).setHelp("Local blue"_el).setHelpNames({"probe-blue"_el});
        auto module = OptionModule::create("paint"_el);
        module->addOption("-m"_el).setChoices(localChoices);
        options->addModule(module);
        auto manager = OptionManager{options};
        const auto document = manager.detailedHelpDocument("paint"_el, "probe"_el);
        const auto text = document.toString();
        REQUIRE(text.contains("Choices for paint -m:\n  Blue!"_el));
        REQUIRE(text.contains("Choices for --color:\n  Blue!"_el));
        REQUIRE(text.contains("Local blue"_el));
        REQUIRE(text.contains("Use blue"_el));
        REQUIRE(text.endsWith("Red!"_el));
        REQUIRE(text.find("Choices for paint -m:"_el) < text.find("Choices for --color:"_el));
        REQUIRE(!text.contains("Options:"_el));
        REQUIRE(!text.contains("green"_el));
        REQUIRE(!text.contains("probe-blue"_el));
        REQUIRE(!text.contains("--help"_el));
        REQUIRE(document.root()->contains(NodeType::OptionValue));
        const auto qualified = manager.detailedHelpDocument({}, "--color/probe"_el).toString();
        REQUIRE(qualified.contains("Choices for --color:"_el));
        REQUIRE(!qualified.contains("paint"_el));
        const auto owner = manager.detailedHelpDocument({}, "--colo"_el).toString();
        REQUIRE(owner.contains("Options:\n  -C, --color <choice>"_el));
        REQUIRE(!owner.contains("Use blue"_el));
        REQUIRE(!owner.contains("Choices for"_el));
    }

    void testLimitIsAppliedBeforeGroupingByKindAndScope() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        for (auto index = 0; index < 9; ++index) {
            options->addOption(
                {el::join("--probe-option-"_el, String::fromInteger(index)),
                    el::join("probe-option-"_el, String::fromInteger(index))});
        }
        auto selected = OptionModule::create("active"_el);
        selected->addOption({"--probe-local-one"_el, "probe-local-one"_el});
        selected->addOption({"--probe-local-two"_el, "probe-local-two"_el});
        options->addModule(selected);
        options->addModule(OptionModule::create("probe-module"_el));
        auto manager = OptionManager{options};
        const auto text = manager.detailedHelpDocument("active"_el, "probe"_el).toString();
        REQUIRE(text.contains("Options for active:"_el));
        REQUIRE(text.contains("--probe-option-7"_el));
        REQUIRE(!text.contains("--probe-option-8"_el));
        REQUIRE(!text.contains("probe-module"_el));
        REQUIRE(!text.contains("Modules:"_el));
        REQUIRE(text.contains("2 additional matches."_el));
    }

    void testLocalizedHeadingsAndOptionalValuePlaceholder() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        options->addOption({"--probe-global"_el, "probe-global"_el})
            .setType(OptionType::Text)
            .setValueName("path"_el)
            .setFlag(OptionFlag::AcceptAsFlag);
        auto module = OptionModule::create("probe-module"_el);
        module->addOption({"--probe-option"_el, "probe-option"_el});
        auto choices = OptionChoices::create();
        choices->addChoice("probe-value"_el);
        module->addOption("--mode"_el).setChoices(choices);
        options->addModule(module);
        auto globalChoices = OptionChoices::create();
        globalChoices->addChoice("probe-value"_el);
        options->addOption("--color"_el).setChoices(globalChoices);
        auto wording = DisplayTextTestLayer{
            {"options"_el, "help_results_title"_el, "Found: {}"_el},
            {"options"_el, "modules_heading"_el, "Commands"_el},
            {"options"_el, "options_heading"_el, "Global flags"_el},
            {"options"_el, "options_for_module_heading"_el, "Flags in {}"_el},
            {"options"_el, "choices_for_option_heading"_el, "Values of {}"_el},
            {"options"_el, "choices_for_module_option_heading"_el, "Values in {1} of {0}"_el},
        };
        auto manager = OptionManager{options};
        const auto text = manager.detailedHelpDocument({}, "PROBE"_el).toString();
        REQUIRE(text.startsWith("\nFound: PROBE\n\nCommands:\n  probe-module"_el));
        REQUIRE(text.contains("Global flags:\n      --probe-global[=<path>]"_el));
        REQUIRE(text.contains("Flags in probe-module:\n  --probe-option"_el));
        REQUIRE(text.contains("Values of --color:\n  probe-value"_el));
        REQUIRE(text.contains("Values in --mode of probe-module:\n  probe-value"_el));
        REQUIRE(text.find("Commands:"_el) < text.find("Global flags:"_el));
        REQUIRE(text.find("Flags in probe-module:"_el) < text.find("Values of --color:"_el));
    }

    void testFilteredSetsDoNotContributeHeadingsOrDescriptions() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        auto reports = OptionSet::create();
        reports->setHelpTitle("Reports"_el);
        reports->setHelpDescription("Report introduction."_el);
        reports->addOption("--report"_el).setHelp("Build report"_el);
        options->addSet(reports);
        auto empty = OptionSet::create();
        empty->setHelpTitle("Empty"_el);
        empty->setHelpDescription("Empty introduction."_el);
        options->addSet(empty);
        for (const auto disabled : {false, true}) {
            auto set = OptionSet::create();
            set->setHelpTitle("Execution"_el);
            set->setHelpDescription("Filtered introduction."_el);
            if (disabled) {
                set->setFlags(OptionFlag::Disabled);
                set->addOption("--disabled"_el);
            } else {
                set->addOption("--hidden"_el).setHelpVisibility(OptionHelpVisibility::Hidden);
            }
            options->addSet(set);
        }
        auto visible = OptionSet::create();
        visible->setHelpTitle("Execution"_el);
        visible->setHelpDescription("Visible introduction."_el);
        visible->addOption("--verbose"_el).setHelpVisibility(OptionHelpVisibility::Overview);
        options->addSet(visible);
        options->addModule(OptionModule::create("run"_el));
        auto manager = OptionManager{options};
        const auto overview = manager.helpDocument({}).toString();
        REQUIRE(!overview.contains("Reports:"_el));
        REQUIRE(!overview.contains("Report introduction."_el));
        REQUIRE(!overview.contains("Empty:"_el));
        REQUIRE(!overview.contains("Empty introduction."_el));
        REQUIRE(!overview.contains("Filtered introduction."_el));
        REQUIRE(overview.contains("Execution:\nVisible introduction."_el));
        const auto full = manager.helpDocument("run"_el).toString();
        REQUIRE(full.contains("Reports:\nReport introduction."_el));
        REQUIRE(full.contains("--report"_el));
        REQUIRE(!full.contains("Empty:"_el));
        REQUIRE(!full.contains("Filtered introduction."_el));
    }

private:
    [[nodiscard]] static auto makeOptions() -> OptionsPtr {
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        return options;
    }
};
