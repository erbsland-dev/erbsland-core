// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"
#include "../i18n/DisplayTextTestLayer.hpp"

#include <erbsland/DisplayText.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Option.hpp>
#include <erbsland/OptionChoice.hpp>
#include <erbsland/OptionChoices.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionHelp.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/OptionModule.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/options/impl/HelpName.hpp>
#include <erbsland/OptionSet.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/TextNode.hpp>
#include <erbsland/TextNodeType.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <utility>
#include <vector>

using namespace el::options;
using namespace el::text::literals;
using el::text::String;
using el::text::StringList;
using NodeType = el::text::TextNodeType;

TESTED_TARGETS(OptionHelp OptionChoice OptionChoiceEditor OptionHelpSearch OptionHelpTarget OptionDocumentBuilder)
class OptionHelpDocumentTest final : public el::UnitTest {
private:
    static constexpr auto cCheckOption = "--check"_el;
    static constexpr auto cColorOption = "--color"_el;
    static constexpr auto cModeOption = "--mode"_el;

public:
    void testBriefAndDetailsAreSeparate() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        options->addOption({"-c"_el, cCheckOption, "check"_el})
            .setHelp("Check generated output"_el)
            .setHelpDescription("Fail without modifying the generated file."_el)
            .setHelpExample("tool --check"_el)
            .setHelpEpilog("Compare the output before committing."_el);
        options->addOption("--brief-only"_el).setHelp("Brief only"_el);
        options->addOption("--details-only"_el).setHelpDescription("Only visible on its detail page."_el);
        auto manager = OptionManager{options};
        const auto overview = manager.helpDocument({}).toString();
        REQUIRE(overview.contains("Check generated output"_el));
        REQUIRE(!overview.contains("Fail without"_el));
        REQUIRE(!overview.contains("Only visible on"_el));
        const auto detail = manager.detailedHelpDocument({}, "check"_el).toString();
        REQUIRE(detail.startsWith("\nCheck generated output\n-c, --check\n"_el));
        REQUIRE(detail.contains("Description:\nFail without"_el));
        REQUIRE(detail.contains("Example:\ntool --check"_el));
        REQUIRE(detail.contains("Compare the output before committing."_el));
        REQUIRE(!manager.detailedHelpDocument({}, "--brief-only"_el).toString().contains("Description:"_el));
        const auto untitled = manager.detailedHelpDocument({}, "--details-only"_el).toString();
        REQUIRE(untitled.startsWith("\nHelp for --details-only\n--details-only\n"_el));
        REQUIRE(untitled.contains("Only visible on its detail page."_el));
        const auto help = OptionHelp{"A brief"_el};
        REQUIRE_EQUAL(help.title(), "A brief"_el);
        REQUIRE(help.description().isEmpty());
    }

    void testMergedSetDescriptionsRequireVisibleRows() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        auto index = 0;
        for (
            const auto &description : {"First introduction."_el, "Second introduction."_el, "First introduction."_el}) {
            auto set = OptionSet::create();
            set->setHelpTitle("Shared section"_el);
            set->setHelpDescription(description);
            set->addOption(el::join("--entry-"_el, String::fromInteger(index++)));
            options->addSet(set);
        }
        auto manager = OptionManager{options};
        const auto text = manager.helpDocument({}).toString();
        REQUIRE(text.contains("Shared section:"_el));
        REQUIRE(text.contains("First introduction."_el));
        REQUIRE(text.find("First introduction."_el) < text.find("Second introduction."_el));
        REQUIRE_EQUAL(text.count("First introduction."_el), el::unit::ItemCount::one());
    }

    void testModuleDetailsReuseFullModuleHelp() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        auto module = OptionModule::create("generate_data"_el);
        module->setHelpTitle("Generate reference data"_el);
        module->setHelpDescription("Read the configured source and generate reference tables."_el);
        auto help = module->help();
        help.setExample("tool generate-data --check"_el);
        module->setHelp(help);
        module->setHelpEpilog("Keep the source data up to date."_el);
        module->addOption(cCheckOption).setHelp("Check generated output"_el);
        options->addModule(module);
        auto manager = OptionManager{options};
        const auto overview = manager.helpDocument({}).toString();
        REQUIRE(overview.contains("Generate reference data"_el));
        REQUIRE(!overview.contains("Read the configured source"_el));
        const auto document = manager.detailedHelpDocument({}, "GENERATE_DATA"_el);
        const auto text = document.toString();
        REQUIRE(text.startsWith("\nGenerate reference data\ngenerate-data\n\nUsage:"_el));
        REQUIRE(text.contains("tool generate-data [options]"_el));
        REQUIRE(text.contains("Description:\nRead the configured source"_el));
        REQUIRE(text.contains(cCheckOption));
        REQUIRE(text.contains("Keep the source data up to date."_el));
        REQUIRE_EQUAL(text, manager.helpDocument("generate-data"_el).toString());
        REQUIRE(document.root()->contains(NodeType::OptionModule));
        REQUIRE(document.root()->contains(NodeType::OptionExecutable));
    }

    void testChoiceAliasesDoNotChangeAcceptedValues() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        auto choices = OptionChoices::create();
        auto editor = choices->addChoice("Deep Blue!"_el);
        editor.setHelp("Use deep blue"_el)
            .setHelpDescription("Choose this shade for the presentation."_el)
            .setHelpNames({"DEEP_BLUE"_el, "blue"_el, "deep-blue"_el});
        const auto choice = editor.choice();
        REQUIRE_EQUAL(choice->helpNames().count(), el::unit::ItemCount{2U});
        REQUIRE_EQUAL(choice->helpNames().first(), "deep-blue"_el);
        REQUIRE_THROWS_AS(el::err::ParameterFailure, choice->setHelpNames({"valid"_el, "bad/name"_el}));
        REQUIRE_EQUAL(choice->helpNames().first(), "deep-blue"_el);
        const auto tooLong = String{el::text::StringEditor{}.append(U'a', el::unit::CpCount{101U})};
        REQUIRE_THROWS_AS(el::err::ParameterFailure, editor.setHelpNames({tooLong}));
        REQUIRE_EQUAL(choice->helpNames().first(), "deep-blue"_el);
        for (const auto &bad : {"--blue"_el, "-b"_el, "_blue"_el, "blue__shade"_el, "blué"_el}) {
            REQUIRE_THROWS_AS(el::err::ParameterFailure, editor.setHelpNames({bad}));
        }
        options->addOption({"-C"_el, cColorOption, "color"_el}).setChoices(choices).setHelp("Choose a color"_el);
        auto manager = OptionManager{options};
        const auto overview = manager.helpDocument({}).toString();
        REQUIRE(overview.contains("Use deep blue"_el));
        REQUIRE(!overview.contains("Choose this shade"_el));
        const auto document = manager.detailedHelpDocument({}, "-C/DEEP_BLUE"_el);
        REQUIRE_EQUAL(document.toString(), manager.detailedHelpDocument({}, "blue"_el).toString());
        REQUIRE(document.toString().contains("Use deep blue\n-C, --color=Deep Blue!"_el));
        REQUIRE(document.toString().contains("Choose this shade for the presentation."_el));
        REQUIRE(document.root()->contains(NodeType::OptionValue));
        REQUIRE(document.root()->contains(NodeType::OptionShort));
        auto args = StringList{"tool"_el, "--color=deep blue!"_el};
        const auto result = manager.parse(args);
        REQUIRE_EQUAL(result.status(), OptionResultStatus::Success);
        REQUIRE_EQUAL(result.values()->getText(cColorOption), "Deep Blue!"_el);
        editor.setHelpNames({});
        REQUIRE(choice->helpNames().isEmpty());
        REQUIRE_THROWS_AS(OptionError, manager.detailedHelpDocument({}, "blue"_el));
        choice->setText("Auto_Mode"_el);
        REQUIRE(manager.detailedHelpDocument({}, "auto-mode"_el).toString().contains("Use deep blue"_el));
    }

    void testAutomaticChoiceNamesAndAmbiguity() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        for (const auto &name : {cColorOption, "--format"_el}) {
            auto choices = OptionChoices::create();
            choices->addChoice("Auto_Mode"_el)
                .setHelp("Choose automatically"_el)
                .setHelpDescription("Automatic selection."_el);
            choices->addChoice("punctuation!"_el).setHelp("Not directly addressable"_el);
            options->addOption(name).setChoices(choices);
        }
        auto manager = OptionManager{options};
        const auto list = manager.detailedHelpDocument({}, "AUTO_MODE"_el).toString();
        REQUIRE(list.startsWith("\nMatching help for \"AUTO_MODE\"\n"_el));
        REQUIRE(list.contains("Choices for --color:\n  Auto_Mode"_el));
        REQUIRE(list.contains("Choices for --format:\n  Auto_Mode"_el));
        REQUIRE(!list.contains("Automatic selection."_el));
        const auto detail = manager.detailedHelpDocument({}, "--COLOR/auto_mode"_el).toString();
        REQUIRE(detail.contains("Automatic selection."_el));
        auto args = StringList{"tool"_el, "--help=--COLOR/AUTO_MODE"_el};
        REQUIRE_EQUAL(manager.parse(args).helpName(), "--color/auto-mode"_el);
    }

    void testDifferentTargetKindsProduceResults() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        options->addOption({"--run"_el, "run"_el}).setHelp("Run as an option"_el);
        auto choices = OptionChoices::create();
        choices->addChoice("run"_el).setHelp("Run as a choice"_el);
        options->addOption(cModeOption).setChoices(choices);
        auto module = OptionModule::create("run"_el);
        module->setHelpTitle("Run as a command"_el);
        options->addModule(module);
        auto manager = OptionManager{options};
        const auto text = manager.detailedHelpDocument({}, "run"_el).toString();
        REQUIRE(text.contains("Options:\n  --run"_el));
        REQUIRE(text.contains("Choices for --mode:\n  run"_el));
        REQUIRE(text.contains("Modules:\n  run"_el));
        REQUIRE(text.contains("Run as an option"_el));
        REQUIRE(text.contains("Run as a choice"_el));
        REQUIRE(text.contains("Run as a command"_el));
        REQUIRE(!text.contains("Description:"_el));
    }

    void testScopePriorityAndRepeatedRegistrations() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        auto repeated = Option::create({cCheckOption, "check"_el});
        repeated->setHelpTitle("Global check"_el);
        for (auto index = 0; index < 2; ++index) {
            auto set = OptionSet::create();
            set->addOption(repeated);
            options->addSet(set);
        }
        auto module = OptionModule::create("generate-data"_el);
        module->addOption({cCheckOption, "check"_el}).setHelp("Module check"_el);
        options->addModule(module);
        auto manager = OptionManager{options};
        const auto global = manager.detailedHelpDocument({}, "check"_el).toString();
        REQUIRE(global.startsWith("\nGlobal check\n"_el));
        REQUIRE(global.contains("tool generate-data --help=--check"_el));
        REQUIRE(!global.contains("tool --help=--check"_el));
        const auto local = manager.detailedHelpDocument("generate-data"_el, "check"_el).toString();
        REQUIRE(local.startsWith("\nModule check\n"_el));
        REQUIRE_EQUAL(local.count("tool --help=--check"_el), el::unit::ItemCount::one());
        REQUIRE(!local.contains("tool generate-data --help=--check"_el));
    }

    void testPrefixResultsLimitAndFuzzyOrdering() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        for (auto index = 0; index < 12; ++index) {
            auto module = OptionModule::create(el::join("generate-table-"_el, String::fromInteger(index)));
            module->setHelpTitle(el::join("Generate table "_el, String::fromInteger(index)));
            options->addModule(module);
        }
        auto manager = OptionManager{options};
        const auto list = manager.detailedHelpDocument({}, "generate"_el).toString();
        REQUIRE(list.startsWith("\nMatching help for \"generate\"\n\nModules:\n"_el));
        REQUIRE_EQUAL(list.count("generate-table-0"_el), el::unit::ItemCount::one());
        REQUIRE(list.contains("Generate table 0"_el));
        REQUIRE(list.contains("generate-table-9"_el));
        REQUIRE(!list.contains("generate-table-10"_el));
        REQUIRE(list.contains("2 additional matches."_el));
        const auto fuzzy = manager.detailedHelpDocument({}, "generate-tabel-8"_el).toString();
        REQUIRE(fuzzy.startsWith("\nMatching help for \"generate-tabel-8\"\n\nModules:\n  generate-table-8"_el));
        REQUIRE(!fuzzy.contains("Description:"_el));
    }

    void testHiddenDisabledAndUnnamedChoiceTargetsAreExcluded() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        auto hidden = OptionModule::create("secret-module"_el);
        hidden->setHelpVisibility(OptionHelpVisibility::Hidden);
        hidden->addOption("--secret"_el);
        options->addModule(hidden);
        auto set = OptionSet::create();
        set->setFlags(OptionFlag::Disabled);
        set->addOption("--disabled"_el);
        options->addSet(set);
        auto choices = OptionChoices::create();
        choices->addChoice("secret-choice"_el).setHelpVisibility(OptionHelpVisibility::Hidden);
        choices->addChoice("odd value!"_el);
        options->addOption(cModeOption).setChoices(choices);
        auto manager = OptionManager{options};
        for (
            const auto &name :
            {"secret-module"_el, "--secret"_el, "--disabled"_el, "secret-choice"_el, "odd-value"_el}) {
            REQUIRE_THROWS_AS(OptionError, manager.detailedHelpDocument({}, name));
        }
    }

    void testMalformedSelectorsDoNotDiscloseInputOrRunCallbacks() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        auto called = false;
        options->addOption(cColorOption);
        options->optionSets().front()->setPreParsingFn([&called](const OptionSetPtr &) -> void { called = true; });
        auto manager = OptionManager{options};
        for (
            const auto &name :
            {"/auto"_el,
                "--color/"_el,
                "--color/auto/more"_el,
                "__color/auto"_el,
                "--color/--auto"_el,
                "--color/a uto"_el,
                "--color/áuto"_el}) {
            auto args = StringList{"tool"_el, el::join("--help="_el, name)};
            const auto result = manager.parse(args);
            REQUIRE_EQUAL(result.status(), OptionResultStatus::Error);
            REQUIRE_EQUAL(result.errorContext()->reason(), OptionErrorReason::SyntaxError);
            REQUIRE_EQUAL(result.errorContext()->title(), "Invalid help keyword"_el);
            REQUIRE(result.errorContext()->arguments().isEmpty());
            REQUIRE(!result.errorContext()->description().contains(name));
            REQUIRE(result.errorContext()->suggestions().isEmpty());
            REQUIRE(!called);
            REQUIRE_THROWS_AS(OptionError, manager.detailedHelpDocument({}, name));
        }
        const auto body = String{el::text::StringEditor{}.append(U'a', el::unit::CpCount{100U})};
        const auto maximum = el::join("--"_el, body, "/"_el, body);
        REQUIRE(el::options::impl::isValidHelpName(maximum));
        REQUIRE(!el::options::impl::isValidHelpName(el::join(maximum, "a"_el)));
    }

    void testUnknownHelpAndLocalizedWording() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        auto manager = OptionManager{options};
        try {
            static_cast<void>(manager.detailedHelpDocument({}, "nonexistent-topic"_el));
            REQUIRE(false);
        } catch (const OptionError &error) {
            REQUIRE_EQUAL(error.context().reason(), OptionErrorReason::UnknownName);
            REQUIRE_EQUAL(error.context().title(), "No matching help found"_el);
            REQUIRE(error.context().description().contains("No module, option or choice matches"_el));
            REQUIRE(error.context().description().contains("nonexistent-topic"_el));
            REQUIRE(error.context().description().contains("Use --help"_el));
        }
        auto wording = DisplayTextTestLayer{
            {"options"_el, "option_details_title"_el, "Details: {}"_el},
            {"options"_el, "help_results_title"_el, "Search results: {}"_el},
            {"options"_el, "unknown_help_name_title"_el, "No translated help"_el},
            {"options"_el, "unknown_help_name_description"_el, "Cannot find {}. Browse with --help."_el},
        };
        options->addOption("--output-file"_el);

        REQUIRE(
            manager.detailedHelpDocument({}, "--output-file"_el).toString().startsWith("\nDetails: --output-file"_el));
        REQUIRE(manager.detailedHelpDocument({}, "--output-fiel"_el)
                .toString()
                .startsWith("\nSearch results: --output-fiel"_el));
        try {
            static_cast<void>(manager.detailedHelpDocument({}, "nonexistent-topic"_el));
            REQUIRE(false);
        } catch (const OptionError &error) {
            REQUIRE_EQUAL(error.context().title(), "No translated help"_el);
            REQUIRE_EQUAL(error.context().description(), "Cannot find nonexistent-topic. Browse with --help."_el);
        }
    }

    void testAuthoredBuiltInHelpAndReplacementOptionsArePreserved() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        options->builtInOptionSet()
            ->editOption("--help"_el)
            .setHelpTitle("Custom help title"_el)
            .setHelpDescription("Custom help details."_el);
        auto manager = OptionManager{options};
        const auto detail = manager.detailedHelpDocument({}, "--help"_el).toString();
        REQUIRE(detail.startsWith("\nCustom help title"_el));
        REQUIRE(detail.contains("Custom help details."_el));
        options->setParserFlag(OptionParserFlag::DisableHelp);
        options->addOption("--help"_el)
            .setHelp("Protocol help option"_el)
            .setHelpDescription("Protocol help details."_el);
        REQUIRE(manager.helpDocument({}).toString().contains("Protocol help option"_el));
        REQUIRE(manager.detailedHelpDocument({}, "--help"_el).toString().contains("Protocol help details."_el));
    }

    void testDiagnosticTitlesDescribeUserProblems() {
        auto applicationScope = ApplicationTestScope<>{};

        const auto expected = std::vector<std::pair<String, String>>{
            {"options.unknown_name_title"_el, "Unrecognized command-line argument"_el},
            {"options.unknown_module_title"_el, "Unrecognized command"_el},
            {"options.module_name_is_missing_title"_el, "Choose a command"_el},
            {"options.module_name_must_come_first_title"_el, "Command must come before options"_el},
            {"options.malformed_long_option_title"_el, "Invalid long option syntax"_el},
            {"options.malformed_short_option_title"_el, "Invalid short option syntax"_el},
            {"options.option_cannot_be_grouped_title"_el, "This option cannot be combined with other short options"_el},
            {"options.flag_does_not_accept_a_value_title"_el, "This option does not accept a value"_el},
            {"options.option_processing_failed_title"_el, "Could not process command-line arguments"_el}};
        for (const auto &[key, title] : expected) {
            REQUIRE_EQUAL(el::i18n::displayText(key), title);
        }
        auto options = makeOptions();
        options->addOption("--flag"_el);
        auto manager = OptionManager{options};
        auto args = StringList{"tool"_el, "-h=flag"_el};
        const auto shortHelp = manager.parse(args);
        REQUIRE_EQUAL(shortHelp.errorContext()->reason(), OptionErrorReason::UnexpectedValueType);
        REQUIRE_EQUAL(shortHelp.errorContext()->title(), "Detailed help requires --help"_el);
        REQUIRE(shortHelp.errorContext()->description().contains("--help=<name>"_el));
        auto wording = DisplayTextTestLayer{
            {"options"_el, "short_help_does_not_accept_a_value_title"_el, "Translated detailed help title"_el},
        };

        REQUIRE_EQUAL(manager.parse(args).errorContext()->title(), "Translated detailed help title"_el);
    }

private:
    [[nodiscard]] static auto makeOptions() -> OptionsPtr {
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        return options;
    }
};
