// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/ApplicationInfo.hpp>
#include <erbsland/err/Diagnostic.hpp>
#include <erbsland/i18n/DisplayTextMap.hpp>
#include <erbsland/options/impl/OptionDocumentBuilder.hpp>
#include <erbsland/options/Option.hpp>
#include <erbsland/options/OptionChoice.hpp>
#include <erbsland/options/OptionChoices.hpp>
#include <erbsland/options/OptionError.hpp>
#include <erbsland/options/OptionErrorContext.hpp>
#include <erbsland/options/OptionHelp.hpp>
#include <erbsland/options/OptionHelpVisibility.hpp>
#include <erbsland/options/OptionManager.hpp>
#include <erbsland/options/OptionModule.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionSet.hpp>
#include <erbsland/options/OptionType.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/text/TextDocument.hpp>
#include <erbsland/text/TextNode.hpp>
#include <erbsland/text/TextNodeData.hpp>
#include <erbsland/text/TextNodeType.hpp>
#include <erbsland/unit/ArgumentUnit.hpp>
#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unit/Version.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>
#include <string>
#include <string_view>

using namespace el::options;
using namespace el::text::literals;
namespace th = erbsland::unittest::th;

TESTED_TARGETS(ApplicationInfo DisplayTextMap OptionDocumentBuilder OptionManager TextDocument)
class OptionDocumentTest final : public el::UnitTest {
public:
    void testRootHelp() {
        const auto options = makeOptions();
        auto manager = OptionManager{options};

        const auto text = toStdString(manager.helpDocument({}).toString());

        requireContains(text, "Operate demo files.\n");
        requireContains(text, "Usage:\n");
        requireContains(text, "demo-tool <module> [options] [--mode <choice>]\n");
        requireContains(text, "Modules:\n");
        requireContains(text, "run");
        requireContains(text, "Run module.");
        requireContains(text, "Options:\n");
        requireContains(text, "-h, --help");
        requireContains(text, "--version");
        requireContains(text, "-v, --verbose");
        requireContains(text, "Enable verbose output.");
        requireContains(text, "--mode <choice>");
        requireContains(text, "Choices: fast, safe.");
        requireContains(text, "Prefer speed.");
        requireContains(text, "Prefer safety.");
        requireMissing(text, "debug");
        requireMissing(text, "expert");
        requireMissing(text, "verbose-alias");
    }

    void testModuleHelpIncludesRootAndModuleOptions() {
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
        const auto options = makeOptions();
        auto manager = OptionManager{options};

        const auto text = toStdString(manager.versionDocument({}).toString());

        requireContains(text, "Demo Tool 1.2.3\n");
        requireContains(text, "Author:     Tobias Erbsland\n");
        requireContains(text, "Copyright 2026");
        requireContains(text, "License:    Apache-2.0\n");
    }

    void testCustomDisplayText() {
        const auto options = makeOptions();
        auto displayText = el::i18n::DisplayTextMap::defaultMap()->clone();
        displayText->set("options.UsageLabel"_el, "Use"_el)
            .set("options.ModulePlaceholder"_el, "action"_el)
            .set("options.OptionsPlaceholder"_el, "flags"_el)
            .set("options.OptionsHeading"_el, "Flags"_el)
            .set("options.HelpOptionDescription"_el, "Show assistance."_el)
            .set("options.VersionOptionDescription"_el, "Show release information."_el)
            .set("options.ChoicePlaceholder"_el, "variant"_el);
        auto manager = OptionManager{options};
        manager.setDisplayTextMap(displayText);

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

    void testGroupsMergeAndOptionsSortByDisplayedName() {
        auto options = Options::create();
        options->setExecutablePath("/usr/bin/lab-tool"_el);

        auto instrumentsA = OptionSet::create();
        instrumentsA->setHelpTitle("Instrument Control"_el);
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
        requireBefore(text, "--alpha", "--beta");
        requireBefore(text, "--beta", "--zeta");
    }

    void testCustomValueNamesAreUsedInUsageAndOptionLists() {
        auto options = Options::create();
        options->setExecutablePath("/usr/bin/lab-tool"_el);
        options->setHelpDescription("Collect readings."_el);
        options->addOption({"--config"_el, "config"_el})
            .setType(OptionType::Text)
            .setValueName("path"_el)
            .setHelpDescription("Configuration file."_el);
        options->addOption({"--repeat"_el, "repeat"_el})
            .setType(OptionType::Integer)
            .setValueName("count"_el)
            .setHelpDescription("Number of repeated readings."_el);
        options->addOption({"--mode"_el, "mode"_el})
            .addChoice("fast"_el)
            .addChoice("safe"_el)
            .setValueName("profile"_el)
            .setHelpDescription("Reading profile."_el)
            .setHelpVisibility(OptionHelpVisibility::Usage);
        options->addOption("input"_el).setValueName("source"_el).setRequired().setHelpDescription("Input source."_el);

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
        requireContains(text, "0 │ demo-tool\n");
        requireContains(text, "1 │ --name\n");
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
        const auto unsafeBytes = th::stdStringFromHex("2D 2D 74 65 73 74 1B");
        const auto unsafeArgument = el::text::StringEditor{std::string_view{unsafeBytes}};
        auto options = Options::create();
        options->setExecutablePath("demo-tool"_el);
        auto manager = OptionManager{options};
        const auto result = manager.parse(makeArgs({"demo-tool"_el, unsafeArgument}));
        REQUIRE(result.errorContext().has_value());

        const auto text = toStdString(manager.errorDocument(result.errorContext().value()).toString());
        requireContains(text, "\"--test\\033\" is not a valid long option.");
        requireContains(text, "1 │ --test\\033\n");
        requireContains(text, "  │ ▔▔▔▔▔▔▔▔▔▔\n");
        requireMissing(text, th::stdStringFromHex("1B"));
    }

    void testExternalExecutableNameIsEscaped() {
        const auto unsafeBytes = th::stdStringFromHex("2F 75 73 72 2F 62 69 6E 2F 74 6F 6F 6C 1B");
        auto options = Options::create();
        options->setExecutablePath(el::text::StringEditor{std::string_view{unsafeBytes}});
        auto manager = OptionManager{options};

        const auto text = toStdString(manager.helpDocument({}).toString());
        requireContains(text, "tool\\033 [options]");
        requireMissing(text, th::stdStringFromHex("1B"));
    }

    void testErrorOutputUsesArgumentWindow() {
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
        requireContains(text, " 3 │ a3\n");
        requireContains(text, "13 │ --bad\n");
        requireContains(text, "   │ \u2594\u2594\u2594\u2594\u2594\n");
        requireContains(text, "14 │ tail\n");
        requireContainsNode(document, el::text::TextNodeType::CodeSnippet, {}, "command-line"_el);
        requireContainsNode(document, el::text::TextNodeType::CodeLineMarker, "error"_el);
    }

    void testMissingPositionalErrorOutputUsesUsageDetails() {
        auto options = Options::create();
        options->setExecutablePath("file_size_monitor"_el);
        const auto fileOption = options->addOption("file"_el)
                                    .setType(OptionType::Text)
                                    .setRequired()
                                    .setHelpDescription("The path to the file to monitor."_el)
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
        auto context = OptionErrorContext{};
        {
            auto options = Options::create();
            options->setExecutablePath("tool"_el);
            options->addOption("--count"_el)
                .setType(OptionType::Integer)
                .setHelpDescription("Number of operations."_el);
            auto manager = OptionManager{options};
            const auto result = manager.parse(makeArgs({"tool"_el, "--count"_el, "wrong"_el}));
            REQUIRE(result.errorContext().has_value());
            context = result.errorContext().value();
        }

        REQUIRE(context.options() != nullptr);
        REQUIRE(context.option() != nullptr);
        const auto text = toStdString(OptionError{context}.diagnostic()->toTextDocument().toString());
        requireContains(text, "Invalid integer value\n");
        requireContains(text, "Usage:\n");
        requireContains(text, "tool [options]\n");
        requireContains(text, "Option Help:\n");
        requireContains(text, "--count <integer>");
        requireContains(text, "View Full Help:\n");
    }

    void testModuleErrorUsesModuleHelpCommand() {
        auto options = Options::create();
        options->setExecutablePath("tool"_el);
        auto module = OptionModule::create("run"_el);
        module->setHelpDescription("Run the configured operation."_el);
        module->addOption("--count"_el).setType(OptionType::Integer).setHelpDescription("Number of operations."_el);
        options->addModule(module);
        auto manager = OptionManager{options};
        const auto result = manager.parse(makeArgs({"tool"_el, "run"_el, "--count"_el, "wrong"_el}));
        REQUIRE(result.errorContext().has_value());

        const auto text = toStdString(manager.errorDocument(result.errorContext().value()).toString());
        requireContains(text, "tool run [options]\n");
        requireContains(text, "Option Help:\n");
        requireContains(text, "View Full Module Help:\n");
        requireContains(text, "tool run --help");
        requireContains(text, "3 │ wrong\n");
        requireContains(text, "   │ \u2594\u2594\u2594\u2594\u2594\n");
    }

    void testSetAndModuleSubjectsRenderTheirOwnHelp() {
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
        options->setHelp(rootHelp);

        options->addOption({"-v"_el, "--verbose"_el, "verbose-alias"_el})
            .setHelp("Enable verbose output."_el)
            .setHelpVisibility(OptionHelpVisibility::Overview);
        auto hiddenHelp = OptionHelp{"Hidden debug mode."_el};
        hiddenHelp.setVisibility(OptionHelpVisibility::Hidden);
        options->addOption("--debug"_el).setHelp(hiddenHelp);

        auto choices = OptionChoices::create();
        choices->addChoice(OptionChoice::create("fast"_el, OptionHelp{"Prefer speed."_el}));
        choices->addChoice(OptionChoice::create("safe"_el, OptionHelp{"Prefer safety."_el}));
        auto expertHelp = OptionHelp{"Expert mode."_el};
        expertHelp.setVisibility(OptionHelpVisibility::Hidden);
        choices->addChoice(OptionChoice::create("expert"_el, expertHelp));
        options->addOption("--mode"_el).setHelp("Select mode."_el).setChoices(choices);
        options->editOption("--mode"_el).setHelpVisibility(OptionHelpVisibility::Usage);

        auto module = OptionModule::create("run"_el);
        auto moduleHelp = OptionHelp{"Run module."_el};
        moduleHelp.setTitle("Run Commands"_el);
        module->setHelp(moduleHelp);
        module->addOption("--force"_el).setHelp("Force execution."_el);
        options->addModule(module);

        return options;
    }

    [[nodiscard]] static auto toStdString(const el::text::String &text) -> std::string {
        return el::text::StringConverter{text}.toStdString();
    }

    [[nodiscard]] static auto makeArgs(std::initializer_list<el::text::String> args) -> el::text::StringList {
        auto result = el::text::StringList{};
        result.reserve(el::unit::ElementCount{args.size()});
        for (const auto &arg : args) {
            result.append(arg.copy());
        }
        return result;
    }

    void requireContains(const std::string &text, const std::string &needle) {
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void { REQUIRE(text.find(needle) != std::string::npos); },
            [&]() -> std::string { return "needle: " + needle + "\ntext:\n" + text; });
    }

    void requireMissing(const std::string &text, const std::string &needle) {
        REQUIRE(text.find(needle) == std::string::npos);
    }

    void requireBefore(const std::string &text, const std::string &first, const std::string &second) {
        const auto firstIndex = text.find(first);
        const auto secondIndex = text.find(second);
        REQUIRE(firstIndex != std::string::npos);
        REQUIRE(secondIndex != std::string::npos);
        REQUIRE(firstIndex < secondIndex);
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
