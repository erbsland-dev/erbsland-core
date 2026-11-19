// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"
#include "../cterm/api/buffer/CursorWriterTestProbe.hpp"
#include "../cterm/support/BlockStringTestHelper.hpp"
#include "../i18n/DisplayTextTestLayer.hpp"

#include <erbsland/ApplicationInfo.hpp>
#include <erbsland/ArgumentIndex.hpp>
#include <erbsland/cterm/TerminalDocumentRenderer.hpp>
#include <erbsland/cterm/TerminalDocumentStyle.hpp>
#include <erbsland/Diagnostic.hpp>
#include <erbsland/DisplayText.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Option.hpp>
#include <erbsland/OptionChoice.hpp>
#include <erbsland/OptionChoices.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionErrorContext.hpp>
#include <erbsland/OptionHelp.hpp>
#include <erbsland/OptionHelpSorting.hpp>
#include <erbsland/OptionHelpVisibility.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/OptionModule.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/options/impl/OptionDocumentBuilder.hpp>
#include <erbsland/OptionSet.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/TextDocument.hpp>
#include <erbsland/TextNode.hpp>
#include <erbsland/TextNodeData.hpp>
#include <erbsland/TextNodeType.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/Version.hpp>

#include <initializer_list>
#include <string>
#include <string_view>

using namespace el::options;
using namespace el::text::literals;
namespace th = erbsland::unittest::th;

TESTED_TARGETS(
    ApplicationInfo DisplayText OptionDocumentBuilder OptionHelpSorting OptionManager OptionParserFlag TextDocument)
class OptionDocumentTest final : public UNITTEST_SUBCLASS(BlockStringTestHelper) {
private:
    static constexpr auto cCountOption = "--count"_el;
    static constexpr auto cForceOption = "--force"_el;
    static constexpr auto cModeOption = "--mode"_el;
    static constexpr auto cVerboseOption = "--verbose"_el;
    static constexpr auto cMOption = "-m"_el;
    static constexpr auto cVOption = "-v"_el;
    static constexpr auto cPathOption = "path"_el;

public:
    void testRootHelp() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto options = makeOptions();
        auto manager = OptionManager{options};

        const auto text = toStdString(manager.helpDocument({}).toString());

        requireContains(text, "Operate demo files.\n");
        requireContains(text, "Usage:\n");
        requireContains(text, "demo-tool <module> [options] [--mode <choice>]\n");
        requireContains(text, "Modules:\n");
        requireContains(text, "run");
        requireContains(text, "Run Commands");
        requireContains(text, "Options:\n");
        requireContains(text, "-h, --help");
        requireContains(text, "--version");
        requireContains(text, "-v, --verbose");
        requireContains(text, "Enable verbose output.");
        requireContains(text, "--mode <choice>");
        requireContains(text, "Choices: fast, safe.");
        requireContains(text, "Prefer speed.");
        requireContains(text, "Prefer safety.");
        requireMissing(text, "Positionals:");
        requireMissing(text, "debug");
        requireMissing(text, "expert");
        requireMissing(text, "verbose-alias");
    }

    void testModuleHelpIncludesRootAndModuleOptions() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto options = makeOptions();
        auto manager = OptionManager{options};

        const auto text = toStdString(manager.helpDocument("RUN"_el).toString());

        requireContains(text, "Run module.\n");
        requireContains(text, "demo-tool run [options] [--mode <choice>]\n");
        requireContains(text, "-v, --verbose");
        requireContains(text, "--force");
        requireMissing(text, "Modules:\n");
    }

    void testVersionOutput() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto options = makeOptions();
        auto manager = OptionManager{options};

        const auto text = toStdString(manager.versionDocument({}).toString());

        requireContains(text, "Demo Tool 1.2.3\n");
        requireContains(text, "  Author:   Tobias Erbsland\n");
        requireContains(text, "Copyright 2026");
        requireContains(text, "  License:  Apache-2.0\n");
    }

    void testDisabledBuiltInOptionsAreHidden() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        options->setParserFlag(OptionParserFlag::DisableVersion);
        auto manager = OptionManager{options};

        auto text = toStdString(manager.helpDocument({}).toString());
        requireContains(text, "-h, --help");
        requireMissing(text, "--version");

        options->setParserFlags(OptionParserFlag::DisableHelp | OptionParserFlag::DisableVersion);
        text = toStdString(manager.helpDocument({}).toString());
        requireMissing(text, "-h, --help");
        requireMissing(text, "--version");
    }

    void testHelpDetailsCanBeDisabled() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        options->addOption({cVOption, cVerboseOption});
        auto manager = OptionManager{options};

        auto text = toStdString(manager.helpDocument({}).toString());
        requireContains(text, "-h, --help[=<keyword>]");
        requireContains(text, "-v, --verbose");
        requireContains(text, "--version");

        options->setParserFlag(OptionParserFlag::NoHelpDetails);
        text = toStdString(manager.helpDocument({}).toString());
        requireContains(text, "-h, --help");
        requireContains(text, "-v, --verbose");
        requireContains(text, "--version");
        requireMissing(text, "[=<keyword>]");
    }

    void testDetailedHelpSearchUsageExampleAndSeeAlso() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        options->addOption({cMOption, cModeOption, "mode"_el})
            .setType(OptionType::Text)
            .setFlag(OptionFlag::AcceptAsFlag)
            .setHelpTitle("Global Mode"_el)
            .setHelpDescription("Select the global mode."_el)
            .setHelpExample("tool run --mode=fast"_el);
        auto module = OptionModule::create("run"_el);
        module->addOption({cModeOption, "mode"_el})
            .setType(OptionType::Choice)
            .addChoice("fast"_el)
            .addChoice("safe"_el)
            .setHelpTitle("Run Mode"_el)
            .setHelpDescription("Select the run mode."_el);
        options->addModule(module);
        auto manager = OptionManager{options};

        auto text = toStdString(manager.detailedHelpDocument({}, "MODE"_el).toString());
        requireContains(text, "Global Mode\n");
        requireContains(text, "tool <module> -m[=<value>]\n");
        requireContains(text, "tool <module> --mode[=<value>]\n");
        requireContains(text, "Description:\n");
        requireContains(text, "Select the global mode.");
        requireContains(text, "Example:\n");
        requireContains(text, "tool run --mode=fast");
        requireContains(text, "See Also:\n");
        requireContains(text, "tool run --help=--mode");

        text = toStdString(manager.detailedHelpDocument("run"_el, "--MODE"_el).toString());
        requireContains(text, "Run Mode\n");
        requireContains(text, "tool run --mode=<choice>\n");
        requireContains(text, "tool --help=--mode");

        text = toStdString(manager.detailedHelpDocument({}, cMOption).toString());
        requireContains(text, "Global Mode\n");
        REQUIRE_THROWS_AS(OptionError, manager.detailedHelpDocument({}, "-M"_el));
    }

    void testDetailedHelpPreservesSemanticColorsAndCommandLayout() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        options->addOption({cMOption, cModeOption, "mode"_el})
            .setType(OptionType::Text)
            .setFlag(OptionFlag::AcceptAsFlag)
            .setHelpTitle("Choose a mode."_el);
        auto module = OptionModule::create("run"_el);
        module->addOption({cModeOption, "mode"_el}).setType(OptionType::Text);
        options->addModule(module);
        auto manager = OptionManager{options};
        const auto document = manager.detailedHelpDocument({}, "mode"_el);
        const auto plain = toStdString(document.toString());
        requireContains(plain, "-m, --mode <value>\n");
        requireContains(plain, "tool <module> -m[=<value>]\n");
        requireContains(plain, "tool <module> --mode[=<value>]\n");
        requireContains(plain, "tool run --help=--mode");
        REQUIRE(document.root()->contains(el::text::TextNodeType::OptionExecutable));
        REQUIRE(document.root()->contains(el::text::TextNodeType::OptionShort));
        REQUIRE(document.root()->contains(el::text::TextNodeType::OptionLong));
        REQUIRE(document.root()->contains(el::text::TextNodeType::OptionMeta));
        REQUIRE(document.root()->contains(el::text::TextNodeType::OptionOptional));
        REQUIRE(document.root()->contains(el::text::TextNodeType::OptionValue));

        for (const auto width : {80, 40}) {
            auto writer = renderHelp(document, width);
            const auto &shortUsage = requireRenderedLine(writer, "  tool <module> -m[=<value>]");
            requireColor(shortUsage, "tool", fg::BrightGreen);
            requireColor(shortUsage, "module", fg::BrightGreen);
            requireColor(shortUsage, "-m", fg::BrightYellow);
            requireColor(shortUsage, "value", fg::BrightGreen);
            requireColor(shortUsage, "<", fg::BrightBlack);
            requireColor(shortUsage, "]", fg::BrightBlack);
            const auto &longUsage = requireRenderedLine(writer, "  tool <module> --mode[=<value>]");
            requireColor(longUsage, "--mode", fg::BrightCyan);
            const auto &seeAlso = requireRenderedLine(writer, "  tool run --help=--mode");
            requireColor(seeAlso, "run", fg::BrightGreen);
            requireColor(seeAlso, "--help", fg::BrightCyan);
            requireColor(seeAlso, "mode", fg::BrightGreen);
            const auto &title = requireRenderedLine(writer, "-m, --mode <value>");
            requireColor(title, "-m", fg::BrightYellow);
            requireColor(title, "--mode", fg::BrightCyan);
        }
    }

    void testDetailedHelpFlagsRequiredValuesLocalizationAndCustomTitles() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        options->addOption(cForceOption).setHelpTitle("Force execution"_el);
        options->addOption(cCountOption).setType(OptionType::Integer);
        auto wording = DisplayTextTestLayer{
            {"options"_el, "integer_placeholder"_el, "number"_el},
        };
        auto manager = OptionManager{options};
        const auto flag = manager.detailedHelpDocument({}, cForceOption);
        requireContains(toStdString(flag.toString()), "Force execution\n");
        requireContains(toStdString(flag.toString()), "tool --force");
        REQUIRE(!flag.root()->children().first()->contains(el::text::TextNodeType::OptionLong));
        const auto required = manager.detailedHelpDocument({}, cCountOption);
        requireContains(toStdString(required.toString()), "tool --count=<number>");
        auto writer = renderHelp(required);
        const auto &usage = requireRenderedLine(writer, "  tool --count=<number>");
        requireColor(usage, "number", fg::BrightGreen);
    }

    void testModuleChoiceAndSearchDocumentsRetainSemanticColors() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        auto module = OptionModule::create("generate-data"_el);
        module->setHelpTitle("Generate data"_el);
        module->setHelpDescription("Generate the configured tables."_el);
        auto choices = OptionChoices::create();
        choices->addChoice("fast"_el).setHelp("Prefer speed"_el).setHelpDescription("Use the fast path."_el);
        module->addOption({cMOption, cModeOption}).setChoices(choices).setValueName("profile"_el);
        options->addModule(module);
        auto manager = OptionManager{options};
        const auto moduleDocument = manager.detailedHelpDocument({}, "generate-data"_el);
        const auto choiceDocument = manager.detailedHelpDocument("generate-data"_el, "--mode/fast"_el);
        const auto resultsDocument = manager.detailedHelpDocument({}, "generate"_el);
        const auto optionResults = manager.detailedHelpDocument("generate-data"_el, "--mod"_el);
        const auto choiceResults = manager.detailedHelpDocument("generate-data"_el, "--mode/fas"_el);
        REQUIRE(optionResults.root()->contains(el::text::TextNodeType::OptionName));
        REQUIRE(!optionResults.root()->contains(el::text::TextNodeType::OptionExecutable));
        REQUIRE(choiceResults.root()->contains(el::text::TextNodeType::OptionValue));
        REQUIRE(choiceDocument.root()->contains(el::text::TextNodeType::OptionValue));
        for (const auto width : {80, 40}) {
            auto moduleWriter = renderHelp(moduleDocument, width);
            requireColor(requireRenderedLine(moduleWriter, "generate-data"), "generate-data", fg::BrightGreen);
            const auto &moduleUsage = requireRenderedLine(moduleWriter, "  tool generate-data [options]");
            requireColor(moduleUsage, "tool", fg::BrightGreen);
            requireColor(moduleUsage, "[", fg::BrightBlack);
            auto choiceWriter = renderHelp(choiceDocument, width);
            const auto &choiceUsage = requireRenderedLine(choiceWriter, "  tool generate-data --mode=fast");
            requireColor(choiceUsage, "--mode", fg::BrightCyan);
            requireColor(choiceUsage, "fast", fg::BrightGreen);
            requireColor(requireRenderedLine(choiceWriter, "  tool generate-data -m=fast"), "-m", fg::BrightYellow);
            auto resultsWriter = renderHelp(resultsDocument, width);
            const auto &moduleResult = requireRenderedContainingLine(resultsWriter, "generate-data");
            requireColor(moduleResult, "generate-data", fg::BrightGreen);
            auto optionWriter = renderHelp(optionResults, width);
            const auto &optionResult = requireRenderedContainingLine(optionWriter, "-m, --mode");
            requireColor(optionResult, "-m", fg::BrightYellow);
            requireColor(optionResult, "--mode", fg::BrightCyan);
            requireColor(optionResult, "profile", fg::BrightGreen);
            requireColor(optionResult, "<", fg::BrightBlack);
            requireColor(optionResult, ">", fg::BrightBlack);
            static_cast<void>(requireRenderedContainingLine(optionWriter, "  -m, --mode"));
            auto valueWriter = renderHelp(choiceResults, width);
            const auto &valueResult = requireRenderedContainingLine(valueWriter, "fast");
            requireColor(valueResult, "fast", fg::BrightGreen);
        }
    }

    void testSuggestionsRetainContextSpecificColors() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        options->addOption({cVOption, cVerboseOption, "verbose-alias"_el});
        options->addOption(cModeOption).setType(OptionType::Choice).addChoice("--safe"_el);
        options->addModule(OptionModule::create("run"_el));
        auto manager = OptionManager{options};
        auto context = OptionErrorContext{}.setOptions(options).setReason(OptionErrorReason::UnknownName);
        context.setSuggestions(
            el::text::StringList{cVOption, cVerboseOption, "run"_el, "verbose-alias"_el, "custom"_el, "--custom"_el});
        auto writer = renderHelp(manager.errorDocument(context));
        requireColor(requireRenderedLine(writer, "  -v"), "-v", fg::BrightYellow);
        requireColor(requireRenderedLine(writer, "  --verbose"), "--verbose", fg::BrightCyan);
        requireColor(requireRenderedLine(writer, "  run"), "run", fg::BrightGreen);
        requireColor(requireRenderedLine(writer, "  verbose-alias"), "verbose-alias", fg::BrightGreen);
        requireColor(requireRenderedLine(writer, "  custom"), "custom", fg::Default);
        requireColor(requireRenderedLine(writer, "  --custom"), "--custom", fg::Default);
        context.setReason(OptionErrorReason::UnexpectedValueType)
            .setOption(options->optionSets().back()->options().back());
        context.setSuggestions(el::text::StringList{"--safe"_el});
        writer = renderHelp(manager.errorDocument(context));
        requireColor(requireRenderedLine(writer, "  --safe"), "--safe", fg::BrightGreen);
    }

    void testCanonicalHelpNamesAndSuggestions() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        options->addOption({"-A"_el, "--One_Two"_el, "One_Two"_el});
        auto module = OptionModule::create("Run_One"_el);
        options->addModule(module);
        auto manager = OptionManager{options};
        module->addOption("--Two_Three"_el);
        requireContains(toStdString(manager.helpDocument("RUN_ONE"_el).toString()), "tool run-one");
        requireContains(
            toStdString(manager.detailedHelpDocument("RUN_ONE"_el, "--TWO_THREE"_el).toString()),
            "tool run-one --two-three");
        const auto document = manager.detailedHelpDocument("RUN_ONE"_el, "--ONE_TWO"_el);
        requireContains(toStdString(document.toString()), "tool <module> --one-two");
        REQUIRE_EQUAL(document.toString(), manager.detailedHelpDocument("run-one"_el, "one_two"_el).toString());
        auto arguments = makeArgs({"tool"_el, "run_one"_el, "--HELP=ONE_TWO"_el});
        const auto result = manager.parse(arguments);
        REQUIRE_EQUAL(result.status(), OptionResultStatus::DisplayHelp);
        REQUIRE_EQUAL(result.helpName(), "one-two"_el);
        const auto results = manager.detailedHelpDocument({}, "--one_tow"_el).toString();
        REQUIRE(results.contains("Matching help for"_el));
        REQUIRE(results.contains("--one-two"_el));
        auto unknownModule = makeArgs({"tool"_el, "run_eno"_el});
        const auto moduleError = manager.parse(unknownModule);
        REQUIRE(moduleError.errorContext()->suggestions().contains("run-one"_el));
    }

    void testMalformedHelpKeywordsAreRejectedWithoutInputDisclosure() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cVerboseOption);
        auto callbackCalled = false;
        options->optionSets().front()->setPreParsingFn(
            [&callbackCalled](const OptionSetPtr &) -> void { callbackCalled = true; });
        auto wording = DisplayTextTestLayer{
            {"options"_el, "malformed_help_name_title"_el, "Invalid translated help keyword"_el},
            {"options"_el, "malformed_help_name_description"_el, "Translated help keyword rules."_el},
        };
        auto manager = OptionManager{options};
        const auto oversized = el::text::String{el::text::StringEditor{}.append(U'a', el::unit::CpCount{101U})};
        const auto malformed = el::text::StringList{
            ""_el,
            "bad keyword"_el,
            "bad=keyword"_el,
            "bad/keyword/more"_el,
            "_a"_el,
            "__help"_el,
            "-_a"_el,
            "one__two"_el,
            "one-"_el,
            "---verbose"_el,
            "-ab"_el,
            "9mode"_el,
            "_mode"_el,
            "mödé"_el,
            "bad\nkeyword"_el,
            el::text::String{el::text::StringEditor{th::stdStringFromHex("ff")}},
            oversized,
            el::join("--"_el, oversized)};
        for (const auto &keyword : malformed) {
            runWithContext(SOURCE_LOCATION(), [&]() -> void {
                const auto helpArgument = el::join("--help="_el, keyword);
                auto arguments = makeArgs({"tool"_el, helpArgument});
                const auto result = manager.parse(arguments);
                REQUIRE_EQUAL(result.status(), OptionResultStatus::Error);
                REQUIRE(result.errorContext().has_value());
                const auto &context = result.errorContext().value();
                REQUIRE_EQUAL(context.reason(), OptionErrorReason::SyntaxError);
                REQUIRE_EQUAL(context.title(), "Invalid translated help keyword"_el);
                REQUIRE_EQUAL(context.description(), "Translated help keyword rules."_el);
                REQUIRE(context.arguments().isEmpty());
                REQUIRE(context.suggestions().isEmpty());
                REQUIRE(!manager.errorDocument(context).toString().contains(helpArgument));
                REQUIRE_FALSE(callbackCalled);
                try {
                    static_cast<void>(manager.detailedHelpDocument({}, keyword));
                    REQUIRE(false);
                } catch (const OptionError &error) {
                    REQUIRE_EQUAL(error.context().reason(), OptionErrorReason::SyntaxError);
                    REQUIRE_EQUAL(error.context().description(), "Translated help keyword rules."_el);
                    REQUIRE(error.context().arguments().isEmpty());
                    REQUIRE(error.context().suggestions().isEmpty());
                }
            });
        }
    }

    void testHelpKeywordBoundariesAndTerminator() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        auto manager = OptionManager{options};
        const auto maximumName = el::text::String{el::text::StringEditor{}.append(U'a', el::unit::CpCount{100U})};
        for (
            const auto &keyword : el::text::StringList{
                "verbose"_el,
                cVerboseOption,
                "-A"_el,
                "-1"_el,
                "mode-2-safe"_el,
                maximumName,
                el::join("--"_el, maximumName)}) {
            auto arguments = makeArgs({"tool"_el, el::join("--help="_el, keyword)});
            const auto result = manager.parse(arguments);
            REQUIRE_EQUAL(result.status(), OptionResultStatus::DisplayHelp);
            REQUIRE_EQUAL(result.helpName(), keyword);
        }
        options->addModule(OptionModule::create("run"_el));
        auto moduleArguments = makeArgs({"tool"_el, "run"_el, "--help=bad keyword"_el});
        const auto moduleResult = manager.parse(moduleArguments);
        REQUIRE_EQUAL(moduleResult.status(), OptionResultStatus::Error);
        REQUIRE(moduleResult.errorContext()->arguments().isEmpty());
        options->setParserFlag(OptionParserFlag::NoHelpDetails);
        auto disabledDetails = makeArgs({"tool"_el, "--help=bad keyword"_el});
        const auto disabledResult = manager.parse(disabledDetails);
        REQUIRE_EQUAL(disabledResult.errorContext()->reason(), OptionErrorReason::SyntaxError);
        REQUIRE(disabledResult.errorContext()->arguments().isEmpty());

        auto positionalOptions = Options::create();
        positionalOptions->addOption(cPathOption).setType(OptionType::Text);
        auto positionalManager = OptionManager{positionalOptions};
        auto afterTerminator = makeArgs({"tool"_el, "--"_el, "--help=bad keyword"_el});
        const auto positionalResult = positionalManager.parse(afterTerminator);
        REQUIRE_EQUAL(positionalResult.status(), OptionResultStatus::Success);
        REQUIRE_EQUAL(positionalResult.values()->getText(cPathOption), "--help=bad keyword"_el);
    }

    void testDetailedHelpRejectsHiddenNamesWithSuggestions() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        options->addOption(cVerboseOption).setHelpTitle("Verbose output."_el);
        options->addOption("--hidden"_el).setHelpVisibility(OptionHelpVisibility::Hidden);
        auto manager = OptionManager{options};

        const auto results = manager.detailedHelpDocument({}, "--verbsoe"_el).toString();
        REQUIRE(results.contains("Matching help for"_el));
        REQUIRE(results.contains(cVerboseOption));
        REQUIRE(!results.contains("--hidden"_el));
        REQUIRE_THROWS_AS(OptionError, manager.detailedHelpDocument({}, "--hidden"_el));
    }

    void testReducedModuleOverview() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptions();
        auto manager = OptionManager{options};

        const auto text = toStdString(manager.moduleOverviewDocument().toString());
        requireContains(text, "demo-tool <module> [options]\n");
        requireContains(text, "Modules:\n");
        requireContains(text, "run");
        requireMissing(text, "Operate demo files.");
        requireMissing(text, "-v, --verbose");
        requireMissing(text, "Select mode.");
    }

    void testReducedModuleOverviewWithNoVisibleModules() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        auto module = OptionModule::create("hidden"_el);
        module->setHelpVisibility(OptionHelpVisibility::Hidden);
        options->addModule(module);
        auto manager = OptionManager{options};

        const auto text = toStdString(manager.moduleOverviewDocument().toString());
        requireContains(text, "tool <module> [options]");
        requireMissing(text, "Modules:\n");
        requireMissing(text, "hidden");
    }

    void testSuggestionsAreLocalizedBeforeUsage() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        options->addOption(cVerboseOption);
        auto displayText = DisplayTextTestLayer{
            {"options"_el, "did_you_mean_heading"_el, "Perhaps"_el},
        };
        auto manager = OptionManager{options};
        auto arguments = makeArgs({"tool"_el, "--verbsoe"_el});
        const auto result = manager.parse(arguments);
        REQUIRE(result.errorContext().has_value());

        const auto text = toStdString(manager.errorDocument(result.errorContext().value()).toString());
        requireContains(text, "Perhaps:\n");
        requireContains(text, "--verbose");
        requireBefore(text, "Perhaps:", "Usage:");
    }

    void testCustomDisplayText() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto options = makeOptions();
        auto displayText = DisplayTextTestLayer{
            {"options"_el, "usage_label"_el, "Use"_el},
            {"options"_el, "module_placeholder"_el, "action"_el},
            {"options"_el, "options_placeholder"_el, "flags"_el},
            {"options"_el, "options_heading"_el, "Flags"_el},
            {"options"_el, "help_option_description"_el, "Show assistance."_el},
            {"options"_el, "version_option_description"_el, "Show release information."_el},
            {"options"_el, "choice_placeholder"_el, "variant"_el},
        };
        auto manager = OptionManager{options};

        const auto text = toStdString(manager.helpDocument({}).toString());

        requireContains(text, "Use:\n");
        requireContains(text, "demo-tool <action> [flags] [--mode <variant>]\n");
        requireContains(text, "Flags:\n");
        requireContains(text, "-h, --help");
        requireContains(text, "Show assistance.");
        requireContains(text, "--version");
        requireContains(text, "Show release information.");
        requireContains(text, "--mode <variant>");
    }

    void testParserAndStorageDiagnosticsUseDisplayTextLayer() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cCountOption).setType(OptionType::Integer);
        options->addOption(cModeOption).setType(OptionType::Choice).addChoice("safe"_el);
        auto wording = DisplayTextTestLayer{
            {"options"_el, "unknown_option_title"_el, "Unknown translated option"_el},
            {"options"_el, "unknown_option_description"_el, "Translated unknown: {}"_el},
            {"options"_el, "invalid_integer_value_title"_el, "Translated integer error"_el},
            {"options"_el, "invalid_integer_description"_el, "Translated integer: {1} rejects {0}"_el},
            {"options"_el, "invalid_choice_title"_el, "Translated choice error"_el},
            {"options"_el, "invalid_choice_description"_el, "Translated choice: {1} rejects {0}. {2}"_el},
            {"options"_el, "choices_label"_el, "Translated choices"_el},
        };
        auto manager = OptionManager{options};
        auto unknown = makeArgs({"tool"_el, "--missing"_el});
        auto result = manager.parse(unknown);
        REQUIRE(result.errorContext().has_value());
        REQUIRE_EQUAL(result.errorContext()->title(), "Unknown translated option"_el);
        REQUIRE_EQUAL(result.errorContext()->description(), "Translated unknown: --missing"_el);
        auto invalidInteger = makeArgs({"tool"_el, "--count=wrong"_el});
        result = manager.parse(invalidInteger);
        REQUIRE(result.errorContext().has_value());
        REQUIRE_EQUAL(result.errorContext()->title(), "Translated integer error"_el);
        requireContains(
            toStdString(result.errorContext()->description()), "Translated integer: --count <integer> rejects wrong");
        auto invalidChoice = makeArgs({"tool"_el, "--mode=wrong"_el});
        result = manager.parse(invalidChoice);
        REQUIRE(result.errorContext().has_value());
        REQUIRE_EQUAL(result.errorContext()->title(), "Translated choice error"_el);
        requireContains(toStdString(result.errorContext()->description()), "Translated choices: safe.");
    }

    void testBuiltInDetailedHelpUsesDisplayTextLayer() {
        auto applicationScope = ApplicationTestScope<>{};
        auto wording = DisplayTextTestLayer{
            {"options"_el, "help_option_description"_el, "Translated help description."_el},
            {"options"_el, "version_option_description"_el, "Translated version description."_el},
            {"options"_el, "help_option_details"_el, "Translated help details."_el},
            {"options"_el, "version_option_details"_el, "Translated version details."_el},
            {"options"_el, "help_keyword_placeholder"_el, "topic"_el},
            {"options"_el, "unknown_help_name_title"_el, "Translated unknown help"_el},
            {"options"_el, "unknown_help_name_description"_el, "Translated help failure: {}"_el},
        };
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        auto manager = OptionManager{options};
        const auto help = toStdString(manager.detailedHelpDocument({}, "--help"_el).toString());
        requireContains(help, "-h, --help <topic>");
        requireContains(help, "tool --help[=<topic>]");
        requireContains(help, "Translated help description.");
        const auto version = toStdString(manager.detailedHelpDocument({}, "--version"_el).toString());
        requireContains(version, "Translated version details.");
        try {
            static_cast<void>(manager.detailedHelpDocument({}, "missing"_el));
            REQUIRE(false);
        } catch (const OptionError &error) {
            REQUIRE_EQUAL(error.context().title(), "Translated unknown help"_el);
            REQUIRE_EQUAL(error.context().description(), "Translated help failure: missing"_el);
        }
    }

    void testGroupsMergeAndOptionsSortByDisplayedName() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("/usr/bin/lab-tool"_el);

        auto instrumentsA = OptionSet::create();
        instrumentsA->setHelpTitle("Instrument Control"_el);
        instrumentsA->addOption("zone"_el).setHelpTitle("First argument."_el);
        instrumentsA->addOption("unused"_el).setFlag(OptionFlag::Disabled);
        instrumentsA->addOption("secret"_el).setHelpVisibility(OptionHelpVisibility::Hidden);
        instrumentsA->addOption("--zeta"_el).setHelp("Last instrument option."_el);
        instrumentsA->addOption("--Alpha"_el).setHelp("First instrument option."_el);
        options->addSet(instrumentsA);

        auto instrumentsB = OptionSet::create();
        instrumentsB->setHelpTitle("Instrument Control"_el);
        instrumentsB->addOption("--beta"_el).setHelp("Second instrument option."_el);
        options->addSet(instrumentsB);

        auto untitled = OptionSet::create();
        untitled->addOption("-c"_el).setHelp("Short-only option."_el);
        untitled->addOption("sample"_el).setHelp("Sample identifier."_el);
        options->addSet(untitled);

        auto manager = OptionManager{options};
        const auto text = toStdString(manager.helpDocument({}).toString());

        requireContains(text, "Options:\n");
        requireContains(text, "Instrument Control:\n");
        requireContains(text, "[options] <zone> <sample>");
        requireContains(text, "Positionals:\n  <zone>    First argument.\n  <sample>  Sample identifier.\n\nOptions:");
        requireMissing(text.substr(text.find("Options:")), "<zone>");
        requireMissing(text.substr(text.find("Options:")), "<sample>");
        requireMissing(text, "<secret>");
        requireMissing(text, "<unused>");
        requireBefore(text, "--alpha", "--beta");
        requireBefore(text, "--beta", "--zeta");
    }

    void testShortAliasesLeadInHelpRegardlessOfDeclarationOrder() {
        constexpr auto cLongOnlyAlias = "--config"_el;
        const auto longAlias = toStdString(cLongOnlyAlias);
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        options->addOption({"--dry-run"_el, "preview"_el, "-d"_el}).setHelpTitle("Preview."_el);
        options->addOption({"--recursive"_el, "-r"_el}).setHelpTitle("Recurse."_el);
        options->addOption(cLongOnlyAlias).setHelpTitle("Configure."_el);
        options->addOption({"--output"_el, "-o"_el, "--out"_el, "-O"_el});

        auto manager = OptionManager{options};
        const auto document = manager.helpDocument({});
        const auto text = toStdString(document.toString());
        requireContains(text, "  -d, --dry-run");
        requireContains(text, "  -r, --recursive");
        requireContains(text, "      " + longAlias);
        requireContains(text, "-o, -O, --output, --out");
        requireMissing(text, "preview");

        const auto writer = renderHelp(document, 100);
        const auto &shortRow = requireRenderedContainingLine(writer, "-d, --dry-run");
        const auto &longRow = requireRenderedContainingLine(writer, longAlias);
        REQUIRE_EQUAL(render(shortRow).find("--dry-run"), render(longRow).find(longAlias));

        const auto detail = toStdString(manager.detailedHelpDocument({}, "preview"_el).toString());
        requireContains(detail, "\n-d, --dry-run\n");
        requireBefore(detail, "tool -d", "tool --dry-run");
    }

    void testHelpSortingPolicies() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        options->setParserFlags(OptionParserFlag::DisableHelp | OptionParserFlag::DisableVersion);
        REQUIRE(options->help().sorting() == OptionHelpSorting::ByLongName);
        REQUIRE(OptionHelp{}.sorting() == OptionHelpSorting::Inherit);
        REQUIRE_EQUAL(OptionHelp{}.weight(), 0);

        options->addOption({"--zulu"_el, "-B"_el}).setHelpVisibility(OptionHelpVisibility::Usage).setHelpWeight(-1);
        options->addOption({"--alpha"_el, "-z"_el}).setHelpVisibility(OptionHelpVisibility::Usage).setHelpWeight(2);
        options->addOption({"-b"_el, "--beta"_el}).setHelpVisibility(OptionHelpVisibility::Usage).setHelpWeight(-1);
        options->addOption("--aardvark"_el).setHelpVisibility(OptionHelpVisibility::Usage).setHelpWeight(2);
        options->addOption({"lookup"_el, "-c"_el}).setHelpVisibility(OptionHelpVisibility::Usage);
        options->addOption("zebra"_el).setHelpWeight(9);
        options->addOption("apple"_el).setHelpWeight(-9);

        const auto manager = OptionManager{options};
        const auto requireOrder = [&](const OptionHelpSorting sorting,
                                      const std::initializer_list<std::string_view> names) {
            options->setHelpSorting(sorting);
            const auto text = toStdString(manager.helpDocument({}).toString());
            requireContains(text, " <zebra> <apple>\n");
            requireBefore(text, "Positionals:", "Options:");
            const auto usage = text.substr(0, text.find("Positionals:"));
            const auto rows = text.substr(text.find("Options:"));
            WITH_CONTEXT(requireInOrder(usage, names));
            WITH_CONTEXT(requireInOrder(rows, names));
            requireBefore(text.substr(text.find("Positionals:")), "<zebra>", "<apple>");
        };
        WITH_CONTEXT(requireOrder(OptionHelpSorting::None, {"--zulu", "--alpha", "--beta", "--aardvark", "-c"}));
        WITH_CONTEXT(requireOrder(OptionHelpSorting::ByLongName, {"--aardvark", "--alpha", "--beta", "-c", "--zulu"}));
        WITH_CONTEXT(requireOrder(OptionHelpSorting::Inherit, {"--aardvark", "--alpha", "--beta", "-c", "--zulu"}));
        WITH_CONTEXT(requireOrder(OptionHelpSorting::ByShortName, {"--beta", "--zulu", "-c", "--alpha", "--aardvark"}));
        WITH_CONTEXT(requireOrder(OptionHelpSorting::ByWeight, {"--beta", "--zulu", "-c", "--aardvark", "--alpha"}));
    }

    void testHelpSortingForModulesAndNamedSections() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        auto zulu = OptionModule::create("zulu"_el);
        auto alpha = OptionModule::create("alpha"_el);
        auto beta = OptionModule::create("beta"_el);
        beta->setHelpWeight(-1);
        options->addModule(zulu);
        options->addModule(alpha);
        options->addModule(beta);

        auto first = OptionSet::create();
        first->setHelpTitle("Zulu Section"_el);
        first->setHelpSorting(OptionHelpSorting::None);
        first->setHelpWeight(-1);
        first->addOption("--zulu-first"_el).setHelpVisibility(OptionHelpVisibility::Overview);
        first->addOption("--alpha-first"_el).setHelpVisibility(OptionHelpVisibility::Overview);
        options->addSet(first);

        auto second = OptionSet::create();
        second->setHelpTitle("alpha Section"_el);
        second->setHelpWeight(2);
        second->addOption("--second"_el).setHelpVisibility(OptionHelpVisibility::Overview);
        options->addSet(second);

        auto shared = OptionSet::create();
        shared->setHelpTitle("Zulu Section"_el);
        shared->setHelpSorting(OptionHelpSorting::ByLongName);
        shared->setHelpWeight(10);
        shared->addOption("--aardvark-shared"_el).setHelpVisibility(OptionHelpVisibility::Overview);
        options->addSet(shared);

        const auto manager = OptionManager{options};
        auto text = toStdString(manager.moduleOverviewDocument().toString());
        WITH_CONTEXT(requireInOrder(text, {"alpha", "beta", "zulu"}));
        text = toStdString(manager.helpDocument({}).toString());
        requireBefore(text, "alpha Section:", "Zulu Section:");
        WITH_CONTEXT(requireInOrder(text, {"--zulu-first", "--alpha-first", "--aardvark-shared"}));

        options->setHelpSorting(OptionHelpSorting::None);
        text = toStdString(manager.helpDocument({}).toString());
        WITH_CONTEXT(requireInOrder(text, {"  zulu", "  alpha", "  beta", "Zulu Section:", "alpha Section:"}));

        options->setHelpSorting(OptionHelpSorting::ByWeight);
        text = toStdString(manager.helpDocument({}).toString());
        WITH_CONTEXT(requireInOrder(text, {"  beta", "  alpha", "  zulu", "Zulu Section:", "alpha Section:"}));
    }

    void testModuleAndSetHelpSortingInheritance() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption("--zulu-global"_el);
        options->addOption("--alpha-global"_el);
        auto module = OptionModule::create("run"_el);
        auto unnamed = OptionSet::create();
        unnamed->addOption("--zulu-local"_el);
        unnamed->addOption("--alpha-local"_el);
        module->addSet(unnamed);
        auto named = OptionSet::create();
        named->setHelpTitle("Module Settings"_el);
        named->addOption("--zulu-setting"_el);
        named->addOption("--alpha-setting"_el);
        module->addSet(named);
        options->addModule(module);

        const auto manager = OptionManager{options};
        auto text = toStdString(manager.helpDocument("run"_el).toString());
        WITH_CONTEXT(requireInOrder(text, {"--alpha-global", "--alpha-local", "--zulu-global", "--zulu-local"}));
        requireBefore(text, "--alpha-setting", "--zulu-setting");

        module->setHelpSorting(OptionHelpSorting::None);
        text = toStdString(manager.helpDocument("run"_el).toString());
        WITH_CONTEXT(requireInOrder(text, {"--zulu-global", "--alpha-global", "--zulu-local", "--alpha-local"}));
        requireBefore(text, "--zulu-setting", "--alpha-setting");

        unnamed->setHelpSorting(OptionHelpSorting::ByLongName);
        text = toStdString(manager.helpDocument("run"_el).toString());
        WITH_CONTEXT(requireInOrder(text, {"--alpha-global", "--alpha-local", "--zulu-global", "--zulu-local"}));
        requireBefore(text, "--zulu-setting", "--alpha-setting");
    }

    void testCustomValueNamesAreUsedInUsageAndOptionLists() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("/usr/bin/lab-tool"_el);
        options->setHelpDescription("Collect readings."_el);
        options->addOption({"--config"_el, "config"_el})
            .setType(OptionType::Text)
            .setValueName(cPathOption)
            .setHelpTitle("Configuration file."_el);
        options->addOption({"--repeat"_el, "repeat"_el})
            .setType(OptionType::Integer)
            .setValueName("count"_el)
            .setHelpTitle("Number of repeated readings."_el);
        options->addOption({cModeOption, "mode"_el})
            .addChoice("fast"_el)
            .addChoice("safe"_el)
            .setValueName("profile"_el)
            .setHelpTitle("Reading profile."_el)
            .setHelpVisibility(OptionHelpVisibility::Usage);
        options->addOption("input"_el).setValueName("source"_el).setRequired().setHelpTitle("Input source."_el);

        auto manager = OptionManager{options};
        const auto text = toStdString(manager.helpDocument({}).toString());

        requireContains(text, "lab-tool [options] [--mode <profile>] <source>\n");
        requireContains(text, "--config <path>");
        requireContains(text, "--repeat <count>");
        requireContains(text, "--mode <profile>");
        requireContains(text, "<source>");
        requireContains(text, "Choices: fast, safe.");
        requireMissing(text, "--config <value>");
        requireMissing(text, "--repeat <integer>");
        requireMissing(text, "--mode <choice>");
        requireMissing(text, "<input>");
    }

    void testErrorOutputUsesCommandLineSnippet() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("demo-tool"_el);
        const auto option = options->addOption("--name"_el).setType(OptionType::Text).option();
        auto manager = OptionManager{options};
        auto context = OptionErrorContext{};
        context.setTitle("Invalid option value"_el)
            .setDescription("The supplied name is not valid."_el)
            .setOption(option)
            .setArgumentIndex(el::unit::ArgumentIndex{1U})
            .setArguments(makeArgs({"demo-tool"_el, "--name"_el}));

        auto document = manager.errorDocument(context);
        const auto text = toStdString(document.toString());
        requireContains(text, "Error: Invalid option value\n");
        requireContains(text, "Error Source:\n");
        requireContains(text, "Command Line Arguments:\n");
        requireContains(text, "Argument: 1\n");
        requireMissing(text, "option: --name\n");
        requireMissing(text, "argument index:");
        requireMissing(text, "--> command line\n");
        requireContains(text, "1 │ demo-tool\n");
        requireContains(text, "2 │ --name\n");
        requireContains(text, "  │ \u2594\u2594\u2594\u2594\u2594\u2594\n");
        requireContains(text, "Usage:\n");
        requireContains(text, "demo-tool [options]\n");
        requireContains(text, "Option Help:\n");
        requireContains(text, "View Full Help:\n");
        requireContains(text, "demo-tool --help");
        requireContainsNode(document, el::text::TextNodeType::CodeSnippet, {}, "command-line"_el);
        requireContainsNode(document, el::text::TextNodeType::CodeLineMarker, "error"_el);
    }

    void testExternalCommandLineTextIsEscapedAndMarkerMatches() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto unsafeBytes = th::stdStringFromHex("2D 2D 74 65 73 74 1B");
        const auto unsafeArgument = el::text::StringEditor{std::string_view{unsafeBytes}};
        auto options = Options::create();
        options->setExecutablePath("demo-tool"_el);
        auto manager = OptionManager{options};
        auto arguments = makeArgs({"demo-tool"_el, unsafeArgument});
        const auto result = manager.parse(arguments);
        REQUIRE(result.errorContext().has_value());

        const auto text = toStdString(manager.errorDocument(result.errorContext().value()).toString());
        requireContains(text, "\"--test\\u{1b}\" is not a valid long option.");
        requireContains(text, "2 │ --test\\u{1b}\n");
        requireContains(text, "  │ ▔▔▔▔▔▔▔▔▔▔▔▔\n");
        requireMissing(text, th::stdStringFromHex("1B"));
    }

    void testExternalExecutableNameIsEscaped() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto unsafeBytes = th::stdStringFromHex("2F 75 73 72 2F 62 69 6E 2F 74 6F 6F 6C 1B");
        auto options = Options::create();
        options->setExecutablePath(el::text::StringEditor{std::string_view{unsafeBytes}});
        auto manager = OptionManager{options};

        const auto text = toStdString(manager.helpDocument({}).toString());
        requireContains(text, "tool\\u{1b} [options]");
        requireMissing(text, th::stdStringFromHex("1B"));
    }

    void testErrorOutputUsesArgumentWindow() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        auto manager = OptionManager{options};
        auto context = OptionErrorContext{};
        context.setTitle("Unknown option"_el)
            .setArgumentIndex(el::unit::ArgumentIndex{13U})
            .setArguments(makeArgs(
                {"tool"_el,
                    "a1"_el,
                    "a2"_el,
                    "a3"_el,
                    "a4"_el,
                    "a5"_el,
                    "a6"_el,
                    "a7"_el,
                    "a8"_el,
                    "a9"_el,
                    "a10"_el,
                    "a11"_el,
                    "a12"_el,
                    "--bad"_el,
                    "tail"_el}));

        auto document = manager.errorDocument(context);
        const auto text = toStdString(document.toString());
        requireMissing(text, "0 | tool\n");
        requireContains(text, " 4 │ a3\n");
        requireContains(text, "14 │ --bad\n");
        requireContains(text, "   │ \u2594\u2594\u2594\u2594\u2594\n");
        requireContains(text, "15 │ tail\n");
        requireContainsNode(document, el::text::TextNodeType::CodeSnippet, {}, "command-line"_el);
        requireContainsNode(document, el::text::TextNodeType::CodeLineMarker, "error"_el);
    }

    void testMissingPositionalErrorOutputUsesUsageDetails() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("file_size_monitor"_el);
        const auto fileOption = options->addOption("file"_el)
                                    .setType(OptionType::Text)
                                    .setRequired()
                                    .setHelpTitle("The path to the file to monitor."_el)
                                    .option();
        auto manager = OptionManager{options};
        auto context = OptionErrorContext{};
        context.setTitle("Required argument is missing"_el)
            .setDescription("Provide <file> before running this command."_el)
            .setOption(fileOption);

        const auto text = toStdString(manager.errorDocument(context).toString());
        requireContains(text, "Required argument is missing\n");
        requireContains(text, "Usage:\n");
        requireContains(text, "file_size_monitor [options] <file>\n");
        requireContains(text, "Option Help:\n");
        requireContains(text, "<file>");
        requireContains(text, "The path to the file to monitor.");
        requireMissing(text, "option: file\n");
        requireContains(text, "View Full Help:\n");
        requireContains(text, "file_size_monitor --help");
    }

    void testErrorContextRetainsDefinitionsForExceptionDiagnostic() {
        auto applicationScope = ApplicationTestScope<>{};
        auto context = OptionErrorContext{};
        {
            auto options = Options::create();
            options->setExecutablePath("tool"_el);
            options->addOption(cCountOption).setType(OptionType::Integer).setHelpTitle("Number of operations."_el);
            auto manager = OptionManager{options};
            auto arguments = makeArgs({"tool"_el, cCountOption, "wrong"_el});
            const auto result = manager.parse(arguments);
            REQUIRE(result.errorContext().has_value());
            context = result.errorContext().value();
        }

        REQUIRE(context.options());
        REQUIRE(context.option());
        const auto text = toStdString(OptionError{context}.diagnostic()->toTextDocument().toString());
        requireContains(text, "Invalid integer value\n");
        requireContains(text, "Usage:\n");
        requireContains(text, "tool [options]\n");
        requireContains(text, "Option Help:\n");
        requireContains(text, "--count <integer>");
        requireContains(text, "View Full Help:\n");
    }

    void testModuleErrorUsesModuleHelpCommand() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        auto module = OptionModule::create("run"_el);
        module->setHelpDescription("Run the configured operation."_el);
        module->addOption(cCountOption).setType(OptionType::Integer).setHelpTitle("Number of operations."_el);
        options->addModule(module);
        auto manager = OptionManager{options};
        auto arguments = makeArgs({"tool"_el, "run"_el, cCountOption, "wrong"_el});
        const auto result = manager.parse(arguments);
        REQUIRE(result.errorContext().has_value());

        const auto text = toStdString(manager.errorDocument(result.errorContext().value()).toString());
        requireContains(text, "tool run [options]\n");
        requireContains(text, "Option Help:\n");
        requireContains(text, "View Full Module Help:\n");
        requireContains(text, "tool run --help");
        requireContains(text, "4 │ wrong\n");
        requireContains(text, "   │ \u2594\u2594\u2594\u2594\u2594\n");
    }

    void testSetAndModuleSubjectsRenderTheirOwnHelp() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        auto optionSet = OptionSet::create();
        optionSet->setHelpTitle("Network Options"_el);
        optionSet->setHelpDescription("Configure network access."_el);
        options->addSet(optionSet);
        auto module = OptionModule::create("run"_el);
        module->setHelpDescription("Run the configured operation."_el);
        options->addModule(module);
        auto manager = OptionManager{options};

        auto setContext = OptionErrorContext{};
        setContext.setTitle("Option group rejected the command"_el).setOptionSet(optionSet);
        auto text = toStdString(manager.errorDocument(setContext).toString());
        requireContains(text, "Option Group Help:\n");
        requireContains(text, "Network Options");
        requireContains(text, "Configure network access.");

        auto moduleContext = OptionErrorContext{};
        moduleContext.setTitle("Module rejected the command"_el).setModule(module);
        text = toStdString(manager.errorDocument(moduleContext).toString());
        requireContains(text, "Module Help:\n");
        requireContains(text, "run");
        requireContains(text, "Run the configured operation.");
        requireContains(text, "View Full Module Help:\n");
        requireContains(text, "tool run --help");
    }

private:
    [[nodiscard]] auto renderHelp(const el::text::TextDocument &document, const int width = 80) -> CursorWriterProbe {
        auto writer = CursorWriterProbe{};
        writer._size = block::Size{width, 25};
        TerminalDocumentRenderer{TerminalDocumentStyle::defaultSystemOutput()}.renderTo(writer, document);
        return writer;
    }

    [[nodiscard]] auto requireRenderedLine(const CursorWriterProbe &writer, const std::string &expected)
        -> const BlockStringEditor & {
        for (const auto &line : writer._writtenStrings) {
            if (render(line) == expected) {
                return line;
            }
        }
        auto output = std::string{};
        for (const auto &line : writer._writtenStrings) {
            output += render(line) + "\n";
        }
        requireContains(output, expected);
        REQUIRE(false);
        return writer._writtenStrings.front();
    }

    [[nodiscard]] auto requireRenderedContainingLine(const CursorWriterProbe &writer, const std::string &expected)
        -> const BlockStringEditor & {
        for (const auto &line : writer._writtenStrings) {
            if (render(line).find(expected) != std::string::npos) {
                return line;
            }
        }
        REQUIRE(false);
        return writer._writtenStrings.front();
    }

    void requireColor(const BlockStringEditor &line, const std::string &text, const Foreground color) {
        const auto position = render(line).find(text);
        REQUIRE_NOT_EQUAL(position, std::string::npos);
        for (auto index = position; index < position + text.size(); ++index) {
            REQUIRE_EQUAL(line[BlockIndex::fromSizeT(index)].style().fg(), color);
        }
    }

    [[nodiscard]] static auto makeOptions() -> OptionsPtr {
        auto options = Options::create();
        auto applicationInfo = el::core::ApplicationInfo{};
        applicationInfo.setApplicationName("Demo Tool"_el);
        applicationInfo.setApplicationVersion(el::unit::Version{1, 2, 3, 4});
        applicationInfo.setAuthorName("Tobias Erbsland"_el);
        applicationInfo.setCopyrightLine("Copyright 2026"_el);
        applicationInfo.setLicenseText("Apache-2.0"_el);
        options->setApplicationInfo(applicationInfo);
        options->setExecutablePath("/usr/bin/demo-tool.exe"_el);

        auto rootHelp = OptionHelp{"Operate demo files."_el};
        rootHelp.setTitle("Demo Tool"_el);
        rootHelp.setDescription("Operate demo files."_el);
        options->setHelp(rootHelp);

        options->addOption({cVOption, cVerboseOption, "verbose-alias"_el})
            .setHelp("Enable verbose output."_el)
            .setHelpVisibility(OptionHelpVisibility::Overview);
        auto hiddenHelp = OptionHelp{"Hidden debug mode."_el};
        hiddenHelp.setVisibility(OptionHelpVisibility::Hidden);
        options->addOption("--debug"_el).setHelp(hiddenHelp);

        auto choices = OptionChoices::create();
        choices->addChoice("fast"_el).setHelpTitle("Prefer speed."_el);
        choices->addChoice("safe"_el).setHelpTitle("Prefer safety."_el);
        choices->addChoice("expert"_el).setHelpTitle("Expert mode."_el).setHelpVisibility(OptionHelpVisibility::Hidden);
        options->addOption(cModeOption).setHelp("Select mode."_el).setChoices(choices);
        options->editOption(cModeOption).setHelpVisibility(OptionHelpVisibility::Usage);

        auto module = OptionModule::create("run"_el);
        auto moduleHelp = OptionHelp{"Run module."_el};
        moduleHelp.setTitle("Run Commands"_el);
        moduleHelp.setDescription("Run module."_el);
        module->setHelp(moduleHelp);
        module->addOption(cForceOption).setHelp("Force execution."_el);
        options->addModule(module);

        return options;
    }

    [[nodiscard]] static auto toStdString(const el::text::String &text) -> std::string {
        return el::text::StringConverter{text}.toStdString();
    }

    [[nodiscard]] static auto makeArgs(std::initializer_list<el::text::String> args) -> el::text::StringList {
        auto result = el::text::StringList{};
        result.reserve(el::unit::ItemCount::fromInteger(args.size()));
        for (const auto &arg : args) {
            result.append(arg.copy());
        }
        return result;
    }

    void requireContains(const std::string &text, const std::string &needle) {
        const auto position = text.find(needle);
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void { REQUIRE_NOT_EQUAL(position, std::string::npos); },
            [&]() -> std::string { return "needle: " + needle + "\ntext:\n" + text; });
    }

    void requireMissing(const std::string &text, const std::string &needle) {
        const auto position = text.find(needle);
        REQUIRE_EQUAL(position, std::string::npos);
    }

    void requireBefore(const std::string &text, const std::string &first, const std::string &second) {
        const auto firstIndex = text.find(first);
        const auto secondIndex = text.find(second);
        REQUIRE_NOT_EQUAL(firstIndex, std::string::npos);
        REQUIRE_NOT_EQUAL(secondIndex, std::string::npos);
        REQUIRE_LESS(firstIndex, secondIndex);
    }

    void requireInOrder(const std::string &text, const std::initializer_list<std::string_view> names) {
        auto offset = std::size_t{0};
        for (const auto name : names) {
            const auto found = text.find(name, offset);
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void { REQUIRE_NOT_EQUAL(found, std::string::npos); },
                [&]() -> std::string { return "expected in order: " + std::string{name} + "\ntext:\n" + text; });
            offset = found + name.size();
        }
    }
    void requireContainsNode(
        const el::text::TextDocument &document,
        const el::text::TextNodeType type,
        const el::text::String &style = {},
        const el::text::String &data = {}) {
        REQUIRE(document.root()->anyOf([type, style, data](const el::text::TextNode &node) {
            if (node.type() != type) {
                return false;
            }
            if (!style.isEmpty() && node.style() != style) {
                return false;
            }
            if (!data.isEmpty() && (node.data() == nullptr || node.data()->toString() != data)) {
                return false;
            }
            return true;
        }));
    }
};
