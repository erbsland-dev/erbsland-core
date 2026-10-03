// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/AnyStringBuilderStream.hpp>
#include <erbsland/CommandLineArguments.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionChoices.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionErrorContext.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/OptionModule.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/options/impl/ExecutableName.hpp>
#include <erbsland/OptionSensitiveTextLocation.hpp>
#include <erbsland/OptionSet.hpp>
#include <erbsland/OptionValue.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>
#include <string>
#include <vector>

using el::core::CommandLineArguments;
using el::options::OptionError;
using el::text::String;
using el::text::StringConverter;
using el::text::StringEditor;
using el::unit::ArgumentCount;
using el::unit::ArgumentIndex;
using el::unit::ExitCode;
using el::unit::ItemCount;
using namespace el::options;
using namespace el::text::literals;

TESTED_TARGETS(OptionManager OptionParser OptionParserFlag)
class OptionsParserTest final : public el::UnitTest {
private:
    static constexpr auto cAllOption = "--all"_el;
    static constexpr auto cChoiceOption = "--choice"_el;
    static constexpr auto cColorOption = "--color"_el;
    static constexpr auto cCountOption = "--count"_el;
    static constexpr auto cEqualsOption = "--equals"_el;
    static constexpr auto cFeatureOption = "--feature"_el;
    static constexpr auto cForceOption = "--force"_el;
    static constexpr auto cHelpOption = "--help"_el;
    static constexpr auto cIncludeOption = "--include"_el;
    static constexpr auto cIntegerOption = "--integer"_el;
    static constexpr auto cLateOption = "--late"_el;
    static constexpr auto cLongOption = "--long"_el;
    static constexpr auto cModeOption = "--mode"_el;
    static constexpr auto cModuleFlagOption = "--module-flag"_el;
    static constexpr auto cNameOption = "--name"_el;
    static constexpr auto cOffOption = "--off"_el;
    static constexpr auto cRequiredOption = "--required"_el;
    static constexpr auto cSecretOption = "--secret"_el;
    static constexpr auto cSeparateOption = "--separate"_el;
    static constexpr auto cSpaceOption = "--space"_el;
    static constexpr auto cTextOption = "--text"_el;
    static constexpr auto cThreadsOption = "--threads"_el;
    static constexpr auto cVerboseOption = "--verbose"_el;
    static constexpr auto cVisibleOption = "--visible"_el;
    static constexpr auto cUpperAOption = "-A"_el;
    static constexpr auto cLowerAOption = "-a"_el;
    static constexpr auto cBOption = "-b"_el;
    static constexpr auto cEOption = "-e"_el;
    static constexpr auto cFOption = "-f"_el;
    static constexpr auto cPOption = "-p"_el;
    static constexpr auto cSOption = "-s"_el;
    static constexpr auto cTOption = "-t"_el;
    static constexpr auto cVOption = "-v"_el;
    static constexpr auto cArgumentsOption = "arguments"_el;
    static constexpr auto cCountPositionalOption = "count"_el;
    static constexpr auto cFilesOption = "files"_el;
    static constexpr auto cInputOption = "input"_el;
    static constexpr auto cInputsOption = "inputs"_el;
    static constexpr auto cLiteralOption = "literal"_el;
    static constexpr auto cModePositionalOption = "mode"_el;
    static constexpr auto cOutputOption = "output"_el;
    static constexpr auto cPathOption = "path"_el;
    static constexpr auto cPositionalOption = "positional"_el;
    static constexpr auto cSourceOption = "source"_el;
    static constexpr auto cTargetOption = "target"_el;

public:
    void testCanonicalCommandLineNamesPreserveValuesAndArguments() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption({cUpperAOption, "--One_Two"_el, "One_Two"_el});
        options->addOption(cLowerAOption);
        options->addOption("--text_value"_el).setType(OptionType::Text);
        options->addOption("--choice_value"_el).addChoice("safe_mode"_el);
        options->addOption("path_name"_el).setType(OptionType::Text).setMaximum(ArgumentCount{2U});
        auto module = OptionModule::create("Run_One"_el);
        options->addModule(module);
        auto args = makeArgs(
            {"tool"_el,
                "RUN_ONE"_el,
                "--ONE_TWO"_el,
                "-aA"_el,
                "--TEXT_VALUE=Keep_This_Case"_el,
                "--choice_value"_el,
                "safe_mode"_el,
                "--"_el,
                "__help"_el,
                "keep_this_path"_el});
        options->editOption("--one-two"_el).setMaximum(ArgumentCount{2U});
        const auto result = OptionManager{options}.parse(args);
        REQUIRE_EQUAL(result.status(), OptionResultStatus::Success);
        REQUIRE_EQUAL(result.values()->module()->name(), "run-one"_el);
        REQUIRE_EQUAL(result.values()->moduleName(), "run-one"_el);
        REQUIRE_EQUAL(result.values()->getFlagCount(cUpperAOption), ArgumentCount{2U});
        REQUIRE_EQUAL(result.values()->getFlagCount(cLowerAOption), ArgumentCount::one());
        REQUIRE_EQUAL(result.values()->getText("TEXT_VALUE"_el), String{});
        REQUIRE_EQUAL(result.values()->getText("--TEXT_VALUE"_el), "Keep_This_Case"_el);
        REQUIRE_EQUAL(result.values()->getText("--choice-value"_el), "safe_mode"_el);
        REQUIRE_EQUAL(result.values()->getTextList("PATH_NAME"_el).first(), "__help"_el);
        REQUIRE_EQUAL(args.get(el::unit::ItemIndex::one()), "RUN_ONE"_el);
        REQUIRE_EQUAL(args.get(el::unit::ItemIndex{4U}), "--TEXT_VALUE=Keep_This_Case"_el);
        const auto separate = parse(options, {"tool"_el, "run-one"_el, "--text_value"_el, "keep_this"_el});
        REQUIRE_EQUAL(separate.status(), OptionResultStatus::Success);
        REQUIRE_EQUAL(separate.values()->getText("--text-value"_el), "keep_this"_el);
    }

    void testNameNormalizationCannotCreateOptionPrefixes() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption("--one-two"_el);
        for (
            const auto &argument :
            el::text::StringList{"_a"_el, "__one_two"_el, "__help"_el, "-_"_el, "-_a"_el, "--one__two"_el}) {
            const auto result = parse(options, {"tool"_el, argument});
            REQUIRE_EQUAL(result.status(), OptionResultStatus::Error);
            REQUIRE(result.errorContext().has_value());
        }
        REQUIRE_EQUAL(parse(options, {"tool"_el, "--ONE_TWO"_el}).status(), OptionResultStatus::Success);
        REQUIRE_EQUAL(parse(options, {"tool"_el, "--HELP"_el}).status(), OptionResultStatus::DisplayHelp);
        REQUIRE_EQUAL(parse(options, {"tool"_el, "--VERSION"_el}).status(), OptionResultStatus::DisplayVersion);
        REQUIRE_EQUAL(parse(options, {"tool"_el, "-H"_el}).status(), OptionResultStatus::Error);
        options->addOption("--one_two"_el);
        REQUIRE_EQUAL(
            parse(options, {"tool"_el, "--one-two"_el}).errorContext()->reason(), OptionErrorReason::SyntaxError);
    }

    void testRootOptionSuccess() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption({cVOption, cVerboseOption}).setType(OptionType::Flag);
        options->addOption({cLowerAOption, cAllOption}).setType(OptionType::Flag);
        options->addOption({cBOption, "--binary"_el}).setType(OptionType::Flag);
        options->addOption({"-c"_el, "--create"_el}).setType(OptionType::Flag);
        options->addOption({"-n"_el, cNameOption}).setType(OptionType::Text);
        options->addOption({"-i"_el, "--iterations"_el}).setType(OptionType::Integer);
        options->addOption(cModeOption)
            .setType(OptionType::Choice)
            .setChoices(OptionChoices::create({"fast"_el, "safe"_el}));

        const auto result = parse(
            options,
            {"tool"_el,
                "--VERBOSE"_el,
                "-abc"_el,
                cNameOption,
                "Ada"_el,
                "--iterations=42"_el,
                cModeOption,
                "SAFE"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values());
        REQUIRE(result.values()->getFlag(cVerboseOption));
        REQUIRE(result.values()->getFlag(cAllOption));
        REQUIRE(result.values()->getFlag("--binary"_el));
        REQUIRE(result.values()->getFlag("--create"_el));
        REQUIRE(result.values()->getText(cNameOption) == "Ada"_el);
        REQUIRE_EQUAL(result.values()->getInteger("--iterations"_el), OptionInteger{42});
        REQUIRE(result.values()->getText(cModeOption) == "safe"_el);
    }

    void testListValuesAndDefaults() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cIncludeOption).setType(OptionType::Text).setMaximum(ArgumentCount{3U});
        options->addOption(cCountOption).setType(OptionType::Integer).setMaximum(ArgumentCount{2U});
        options->addOption(cThreadsOption).setType(OptionType::Integer).setDefaultValue(OptionInteger{4});
        options->addOption(cColorOption)
            .setType(OptionType::Choice)
            .setChoices(OptionChoices::create({"auto"_el, "never"_el}))
            .setDefaultValue(StringEditor{"AUTO"_el});

        const auto result = parse(
            options,
            {"tool"_el, cIncludeOption, "src"_el, cIncludeOption, "test"_el, cCountOption, "1"_el, "--count=2"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        const auto includes = result.values()->getTextList(cIncludeOption);
        REQUIRE_EQUAL(includes.count(), el::unit::ItemCount{2U});
        REQUIRE(includes.get(el::unit::ItemIndex::zero()) == "src"_el);
        REQUIRE(includes.get(el::unit::ItemIndex{1U}) == "test"_el);
        const auto counts = result.values()->getIntegerList(cCountOption);
        REQUIRE_EQUAL(counts.size(), 2U);
        REQUIRE_EQUAL(counts.at(0), OptionInteger{1});
        REQUIRE_EQUAL(counts.at(1), OptionInteger{2});
        REQUIRE_EQUAL(result.values()->getInteger(cThreadsOption), OptionInteger{4});
        REQUIRE(result.values()->getText(cColorOption) == "auto"_el);
    }

    void testBooleanScalarListAndDefaults() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption("--enabled"_el).setType(OptionType::Boolean);
        options->addOption("--switch"_el).setType(OptionType::Boolean).setMaximum(ArgumentCount{3U});
        options->addOption("--defaults"_el)
            .setType(OptionType::Boolean)
            .setMaximum(ArgumentCount{2U})
            .setDefaultValue(std::vector<bool>{true, false});

        const auto result =
            parse(options, {"tool"_el, "--enabled=YES"_el, "--switch"_el, "off"_el, "--switch=enabled"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getBoolean("--enabled"_el));
        const auto scalarList = result.values()->getBooleanList("--enabled"_el);
        REQUIRE_EQUAL(scalarList.size(), 1U);
        REQUIRE(scalarList.front());
        const auto values = result.values()->getBooleanList("--switch"_el);
        REQUIRE_EQUAL(values.size(), 2U);
        REQUIRE_FALSE(values.at(0));
        REQUIRE(values.at(1));
        REQUIRE(result.values()->getBoolean("--switch"_el, true));
        const auto defaults = result.values()->getBooleanList("--defaults"_el);
        REQUIRE_EQUAL(defaults.size(), 2U);
        REQUIRE(defaults.at(0));
        REQUIRE_FALSE(defaults.at(1));
    }

    void testAcceptAsFlagForAllValueTypes() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption("--boolean"_el).setType(OptionType::Boolean).setFlag(OptionFlag::AcceptAsFlag);
        options->addOption(cIntegerOption).setType(OptionType::Integer).setFlag(OptionFlag::AcceptAsFlag);
        options->addOption(cTextOption).setType(OptionType::Text).setFlag(OptionFlag::AcceptAsFlag);
        options->addOption(cChoiceOption)
            .setChoices(OptionChoices::create({"fast"_el, "safe"_el}))
            .setFlag(OptionFlag::AcceptAsFlag);
        options->addOption(cSecretOption).setType(OptionType::SensitiveText).setFlag(OptionFlag::AcceptAsFlag);

        auto result =
            parse(options, {"tool"_el, "--boolean"_el, cIntegerOption, cTextOption, cChoiceOption, cSecretOption});
        REQUIRE(result.status() == OptionResultStatus::Success);
        for (const auto &name : {"--boolean"_el, cIntegerOption, cTextOption, cChoiceOption, cSecretOption}) {
            REQUIRE(result.values()->getFlag(name));
        }
        REQUIRE_FALSE(result.values()->getBoolean("--boolean"_el));
        REQUIRE_EQUAL(result.values()->getInteger(cIntegerOption), OptionInteger{0});
        REQUIRE(result.values()->getText(cTextOption).isEmpty());
        REQUIRE(result.values()->getText(cChoiceOption).isEmpty());
        REQUIRE(result.values()->getText(cSecretOption).isEmpty());

        result = parse(
            options,
            {"tool"_el,
                "--boolean=true"_el,
                "--integer=42"_el,
                "--text=Ada"_el,
                "--choice=SAFE"_el,
                "--secret=value"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getBoolean("--boolean"_el));
        REQUIRE_EQUAL(result.values()->getInteger(cIntegerOption), OptionInteger{42});
        REQUIRE(result.values()->getText(cTextOption) == "Ada"_el);
        REQUIRE(result.values()->getText(cChoiceOption) == "safe"_el);
        REQUIRE(result.values()->getText(cSecretOption) == "value"_el);
        REQUIRE_FALSE(result.values()->getFlag(cTextOption));
    }

    void testAcceptAsFlagDefinitionAndValidationRules() {
        auto applicationScope = ApplicationTestScope<>{};
        auto validatorSawFlag = false;
        auto options = Options::create();
        options->addOption(cRequiredOption)
            .setType(OptionType::Text)
            .setFlag(OptionFlag::AcceptAsFlag)
            .setRequired()
            .setValidateFn([&validatorSawFlag](const OptionValuePtr &value, const OptionValuesPtr &) -> void {
                validatorSawFlag = value->getFlag();
            });
        auto result = parse(options, {"tool"_el, cRequiredOption});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(validatorSawFlag);

        options = Options::create();
        options->addOption("value"_el).setType(OptionType::Text).setFlag(OptionFlag::AcceptAsFlag);
        assertError(options, {"tool"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        options->addOption("--value"_el)
            .setType(OptionType::Text)
            .setMaximum(ArgumentCount{2U})
            .setFlag(OptionFlag::AcceptAsFlag);
        assertError(options, {"tool"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        options->addOption("--flag"_el).setType(OptionType::Flag).setDefaultValue(true);
        assertError(options, {"tool"_el}, OptionErrorReason::SyntaxError);
    }

    void testDashLeadingValuesRequireAttachedSyntax() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cCountOption).setType(OptionType::Integer);
        assertError(options, {"tool"_el, cCountOption, "-22"_el}, OptionErrorReason::UnexpectedValueType);
        const auto result = parse(options, {"tool"_el, "--count=-22"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE_EQUAL(result.values()->getInteger(cCountOption), OptionInteger{-22});
    }

    void testAttachedEmptyAndRepeatedSeparators() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cTextOption).setType(OptionType::Text);
        options->addOption(cTOption).setType(OptionType::Text);
        for (const auto &argument : el::StringList{"--text="_el, "-t="_el}) {
            const auto result = parse(options, {"tool"_el, argument});
            REQUIRE(result.status() == OptionResultStatus::Success);
        }
        const auto result = parse(options, {"tool"_el, "--text=a=b"_el, "-t=秘密=x"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE_EQUAL(result.values()->getText(cTextOption), "a=b"_el);
        REQUIRE_EQUAL(result.values()->getText(cTOption), "秘密=x"_el);
    }

    void testValueArgumentIndexes() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cEqualsOption).setType(OptionType::Text);
        options->addOption(cSpaceOption).setType(OptionType::Text);
        options->addOption(cEOption).setType(OptionType::Text);
        options->addOption(cSOption).setType(OptionType::Text);

        auto result = parse(options, {"tool"_el, "--equals=A"_el, cSpaceOption, "B"_el, "-e=C"_el, cSOption, "D"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        requireArgumentIndexes(result.values()->value(cEqualsOption), {ArgumentIndex{1U}});
        requireArgumentIndexes(result.values()->value(cSpaceOption), {ArgumentIndex{3U}});
        requireArgumentIndexes(result.values()->value(cEOption), {ArgumentIndex{4U}});
        requireArgumentIndexes(result.values()->value(cSOption), {ArgumentIndex{6U}});

        options = Options::create();
        options->addOption(cPathOption).setType(OptionType::Text);
        options->addOption(cLiteralOption).setType(OptionType::Text);
        result = parse(options, {"tool"_el, "input.txt"_el, "--"_el, cHelpOption});

        REQUIRE(result.status() == OptionResultStatus::Success);
        requireArgumentIndexes(result.values()->value(cPathOption), {ArgumentIndex{1U}});
        requireArgumentIndexes(result.values()->value(cLiteralOption), {ArgumentIndex{3U}});
    }

    void testSensitiveTextValuesLocationsAndMasking() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cLongOption).setType(OptionType::SensitiveText);
        options->addOption(cSOption).setType(OptionType::SensitiveText);
        options->addOption(cSeparateOption).setType(OptionType::SensitiveText);
        options->addOption(cPOption).setType(OptionType::SensitiveText);
        options->addOption(cPositionalOption).setType(OptionType::SensitiveText);
        auto manager = OptionManager{options};
        auto arguments = makeArgs(
            {"tool"_el,
                "--long=alpha"_el,
                "-s=秘密"_el,
                cSeparateOption,
                "bravo"_el,
                cPOption,
                "charlie"_el,
                "delta"_el});
        const auto retainedUnicodeArgument = arguments.get(el::unit::ItemIndex{2U});
        const auto retainedSeparateArgument = arguments.get(el::unit::ItemIndex{4U});
        const auto retainedPositionalArgument = arguments.get(el::unit::ItemIndex{7U});

        const auto result = manager.parse(arguments);

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->value(cLongOption)->type() == OptionValueType::SensitiveText);
        REQUIRE(result.values()->getText(cLongOption) == "alpha"_el);
        REQUIRE(result.values()->getText(cSOption) == u8"秘密"_el);
        REQUIRE(result.values()->getText(cSeparateOption) == "bravo"_el);
        REQUIRE(result.values()->getText(cPOption) == "charlie"_el);
        REQUIRE(result.values()->getText(cPositionalOption) == "delta"_el);
        REQUIRE(result.values()->value(cLongOption)->getText() == "alpha"_el);
        REQUIRE(result.values()->getText(cLongOption).isSensitive());
        REQUIRE(result.values()->getText(cSOption).isSensitive());

        const auto &locations = result.sensitiveTextLocations();
        REQUIRE_EQUAL(locations.size(), std::size_t{5U});
        REQUIRE(locations.at(0) == OptionSensitiveTextLocation{ArgumentIndex{1U}, el::unit::ByteIndex{7U}});
        REQUIRE(locations.at(1) == OptionSensitiveTextLocation{ArgumentIndex{2U}, el::unit::ByteIndex{3U}});
        REQUIRE(locations.at(2) == OptionSensitiveTextLocation{ArgumentIndex{4U}, el::unit::ByteIndex::zero()});
        REQUIRE(locations.at(3) == OptionSensitiveTextLocation{ArgumentIndex{6U}, el::unit::ByteIndex::zero()});
        REQUIRE(locations.at(4) == OptionSensitiveTextLocation{ArgumentIndex{7U}, el::unit::ByteIndex::zero()});

        REQUIRE(arguments.get(el::unit::ItemIndex{1U}) == "--long=*****"_el);
        REQUIRE(arguments.get(el::unit::ItemIndex{2U}) == "-s=*****"_el);
        REQUIRE(arguments.get(el::unit::ItemIndex{3U}) == cSeparateOption);
        REQUIRE(arguments.get(el::unit::ItemIndex{4U}) == "*****"_el);
        REQUIRE(arguments.get(el::unit::ItemIndex{5U}) == cPOption);
        REQUIRE(arguments.get(el::unit::ItemIndex{6U}) == "*****"_el);
        REQUIRE(arguments.get(el::unit::ItemIndex{7U}) == "*****"_el);

        REQUIRE(retainedUnicodeArgument.isSensitive());
        REQUIRE(retainedUnicodeArgument == u8"-s=秘密"_el);
        REQUIRE(retainedSeparateArgument.isSensitive());
        REQUIRE(retainedSeparateArgument == "bravo"_el);
        REQUIRE(retainedPositionalArgument.isSensitive());
        REQUIRE(retainedPositionalArgument == "delta"_el);
    }

    void testSensitiveTextRejectsDefaultsListsAndRepetition() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cSecretOption)
            .setType(OptionType::SensitiveText)
            .setDefaultValue(StringEditor{"fallback"_el});
        assertError(options, {"tool"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        options->addOption(cSecretOption).setType(OptionType::SensitiveText).setMaximum(ArgumentCount{2U});
        assertError(options, {"tool"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        options->addOption(cSecretOption).setType(OptionType::SensitiveText);
        auto manager = OptionManager{options};
        auto arguments = makeArgs({"tool"_el, "--secret=first"_el, cSecretOption, "second"_el});
        const auto result = manager.parse(arguments);
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::UnexpectedValueType);
        REQUIRE(arguments.get(el::unit::ItemIndex{1U}) == "--secret=*****"_el);
        REQUIRE(arguments.get(el::unit::ItemIndex{3U}) == "*****"_el);
        REQUIRE_EQUAL(result.sensitiveTextLocations().size(), std::size_t{2U});
    }

    void testSensitiveTextIsMaskedInValidatorErrors() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cSecretOption)
            .setType(OptionType::SensitiveText)
            .setValidateFn([](OptionValuePtr, OptionValuesPtr) -> void {
                auto context = OptionErrorContext{};
                context.setDescription("Rejected secret"_el);
                context.setArguments(makeArgs({"unsafe"_el, "snapshot"_el}));
                throw OptionError{context};
            });
        auto manager = OptionManager{options};
        auto arguments = makeArgs({"tool"_el, cSecretOption, "täuschend"_el});

        const auto result = manager.parse(arguments);

        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(arguments.get(el::unit::ItemIndex{2U}) == "*****"_el);
        REQUIRE_EQUAL(result.sensitiveTextLocations().size(), std::size_t{1U});
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->arguments().get(el::unit::ItemIndex{2U}) == "*****"_el);
        REQUIRE(result.errorContext()->arguments().get(el::unit::ItemIndex{0U}) == "tool"_el);
    }

    void testListAndDefaultArgumentIndexes() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cIncludeOption).setType(OptionType::Text).setMaximum(ArgumentCount{3U});
        options->addOption(cFilesOption).setType(OptionType::Text).setMaximum(ArgumentCount{2U});
        options->addOption(cModeOption).setType(OptionType::Text).setDefaultValue(StringEditor{"auto"_el});

        const auto result =
            parse(options, {"tool"_el, cIncludeOption, "src"_el, "--include=test"_el, "one"_el, "two"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        requireArgumentIndexes(result.values()->value(cIncludeOption), {ArgumentIndex{2U}, ArgumentIndex{3U}});
        requireArgumentIndexes(result.values()->value(cFilesOption), {ArgumentIndex{4U}, ArgumentIndex{5U}});
        REQUIRE(result.values()->value(cModeOption)->argumentIndex().isNoIndex());
        REQUIRE(result.values()->value(cModeOption)->argumentIndexes().empty());
    }

    void testFlagArgumentIndexes() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption({cLowerAOption, cAllOption}).setType(OptionType::Flag);
        options->addOption(cBOption).setType(OptionType::Flag);
        options->addOption({cVOption, cVerboseOption}).setType(OptionType::Flag);

        const auto result =
            parse(options, {"tool"_el, "-ab"_el, cVerboseOption, "-avv"_el, cAllOption, cVerboseOption});

        REQUIRE(result.status() == OptionResultStatus::Success);
        requireArgumentIndexes(
            result.values()->value(cAllOption), {ArgumentIndex{1U}, ArgumentIndex{3U}, ArgumentIndex{4U}});
        requireArgumentIndexes(result.values()->value(cBOption), {ArgumentIndex{1U}});
        requireArgumentIndexes(
            result.values()->value(cVerboseOption),
            {ArgumentIndex{2U}, ArgumentIndex{3U}, ArgumentIndex{3U}, ArgumentIndex{5U}});
        REQUIRE_EQUAL(result.values()->getFlagCount(cAllOption), ArgumentCount{3U});
        REQUIRE_EQUAL(result.values()->getFlagCount(cLowerAOption), ArgumentCount{3U});
        REQUIRE_EQUAL(result.values()->getFlagCount(cBOption), ArgumentCount::one());
        REQUIRE_EQUAL(result.values()->getFlagCount(cVerboseOption), ArgumentCount{4U});
        REQUIRE_EQUAL(result.values()->getFlagCount(cVOption), ArgumentCount{4U});
        REQUIRE_EQUAL(result.values()->valueCount(cAllOption), ArgumentCount::one());
    }

    void testBooleanValues() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption({cFOption, cFeatureOption}).setType(OptionType::Boolean);
        options->addOption(cInputOption).setType(OptionType::Text);

        auto result = parse(options, {"tool"_el, "--feature=false"_el, "file.txt"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE_FALSE(result.values()->getBoolean(cFeatureOption));
        REQUIRE_EQUAL(result.values()->getFlagCount(cFeatureOption), ArgumentCount::zero());
        requireArgumentIndexes(result.values()->value(cFeatureOption), {ArgumentIndex{1U}});
        REQUIRE_EQUAL(result.values()->getText(cInputOption), "file.txt"_el);

        result = parse(options, {"tool"_el, cFOption, "YES"_el, "file.txt"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getBoolean(cFeatureOption));
        REQUIRE_EQUAL(result.values()->getText(cInputOption), "file.txt"_el);

        result = parse(options, {"tool"_el, "-f=disabled"_el, "file.txt"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE_FALSE(result.values()->getBoolean(cFeatureOption));
        REQUIRE_EQUAL(result.values()->getFlagCount(cFeatureOption), ArgumentCount::zero());

        assertError(options, {"tool"_el, cFeatureOption, "maybe"_el}, OptionErrorReason::UnexpectedValueType);

        options->editOption(cFeatureOption).setDefaultValue(true);
        result = parse(options, {"tool"_el, "--feature=off"_el, "file.txt"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE_FALSE(result.values()->getBoolean(cFeatureOption));
    }

    void testBooleanMaximumAndFlagRepetition() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption({cFOption, cFeatureOption}).setType(OptionType::Boolean);

        assertError(
            options, {"tool"_el, "--feature=false"_el, "--feature=true"_el}, OptionErrorReason::UnexpectedValueType);

        options = Options::create();
        options->addOption({cFOption, cFeatureOption}).setType(OptionType::Flag);
        const auto repeatedImplicit = parse(options, {"tool"_el, "-fff"_el});
        REQUIRE(repeatedImplicit.status() == OptionResultStatus::Success);
        REQUIRE_EQUAL(repeatedImplicit.values()->getFlagCount(cFeatureOption), ArgumentCount{3U});
    }

    void testPureFlagsDoNotConsumeValues() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setParserFlag(OptionParserFlag::NoHelpDetails);
        options->addOption({cFOption, cFeatureOption}).setType(OptionType::Flag);
        options->addOption(cInputOption).setType(OptionType::Text);

        auto result = parse(options, {"tool"_el, cFeatureOption, "false"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getFlag(cFeatureOption));
        REQUIRE_EQUAL(result.values()->getText(cInputOption), "false"_el);

        assertError(options, {"tool"_el, "--feature=false"_el}, OptionErrorReason::UnexpectedValueType);
        assertError(options, {"tool"_el, "-f=false"_el}, OptionErrorReason::UnexpectedValueType);

        auto moduleOptions = Options::create();
        moduleOptions->setParserFlag(OptionParserFlag::NoHelpDetails);
        auto module = OptionModule::create("run"_el);
        module->addOption(cModuleFlagOption).setType(OptionType::Flag);
        module->addOption(cInputOption).setType(OptionType::Text);
        moduleOptions->addModule(module);

        result = parse(moduleOptions, {"tool"_el, "run"_el, cModuleFlagOption, "false"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getFlag(cModuleFlagOption));
        REQUIRE_EQUAL(result.values()->getText(cInputOption), "false"_el);
    }

    void testHelpAndVersionStatus() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cNameOption).setType(OptionType::Text);
        options->addOption(cVerboseOption).setType(OptionType::Flag);

        auto helpResult = parse(options, {"tool"_el, "-h"_el});
        REQUIRE(helpResult.status() == OptionResultStatus::DisplayHelp);
        REQUIRE_EQUAL(helpResult.values()->getFlagCount(cVerboseOption), ArgumentCount::zero());
        REQUIRE(parse(options, {"tool"_el, cHelpOption}).status() == OptionResultStatus::DisplayHelp);
        REQUIRE(parse(options, {"tool"_el, "--version"_el}).status() == OptionResultStatus::DisplayVersion);
        REQUIRE(parse(options, {"tool"_el, "--unknown"_el, cHelpOption}).status() == OptionResultStatus::DisplayHelp);
    }

    void testBuiltInHelpDetailsAndPureVersion() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();

        auto result = parse(options, {"tool"_el, "--help=topic"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
        REQUIRE(result.helpName() == "topic"_el);
        assertError(options, {"tool"_el, "-h=topic"_el}, OptionErrorReason::UnexpectedValueType);
        REQUIRE(parse(options, {"tool"_el, "-h"_el, "enabled"_el}).status() == OptionResultStatus::DisplayHelp);
        assertError(options, {"tool"_el, "--version=on"_el}, OptionErrorReason::UnexpectedValueType);
        assertError(options, {"tool"_el, "--help=topic"_el, "-h"_el}, OptionErrorReason::UnexpectedValueType);
        assertError(options, {"tool"_el, cHelpOption, "-h"_el}, OptionErrorReason::UnexpectedValueType);

        options->setParserFlag(OptionParserFlag::NoHelpDetails);
        REQUIRE(parse(options, {"tool"_el, cHelpOption, "false"_el}).status() == OptionResultStatus::DisplayHelp);
        assertError(options, {"tool"_el, "--help=false"_el}, OptionErrorReason::UnexpectedValueType);
    }

    void testExecutableInformationIsStoredOnOptions() {
        auto applicationScope = ApplicationTestScope<>{};
        REQUIRE(el::options::impl::extractExecutableName("/opt/demo/bitmap-showcase.EXE"_el) == "bitmap-showcase"_el);
        REQUIRE(
            el::options::impl::extractExecutableName("C:\\Tools\\bitmap-showcase.com"_el) == "bitmap-showcase.com"_el);

        auto options = Options::create();
        auto manager = OptionManager{options};

        auto arguments = makeArgs({"/opt/demo/bitmap-showcase.EXE"_el});
        auto result = manager.parse(arguments);
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(options->executablePath() == "/opt/demo/bitmap-showcase.EXE"_el);
        REQUIRE(options->executableName() == "bitmap-showcase"_el);

        arguments = makeArgs({"C:\\Tools\\bitmap-showcase.com"_el});
        result = manager.parse(arguments);
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(options->executablePath() == "C:\\Tools\\bitmap-showcase.com"_el);
        REQUIRE(options->executableName() == "bitmap-showcase.com"_el);
    }

    void testBuiltInOptionCollision() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cHelpOption).setType(OptionType::Flag);

        assertError(options, {"tool"_el}, OptionErrorReason::SyntaxError);
    }

    void testBuiltInOptionsCanBeDisabledIndividually() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->setParserFlag(OptionParserFlag::DisableVersion);
        options->addOption({"--version"_el, "language-version"_el}).setType(OptionType::Text);

        auto result = parse(options, {"tool"_el, "--version"_el, "1.0"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText("language-version"_el) == "1.0"_el);
        REQUIRE(parse(options, {"tool"_el, cHelpOption}).status() == OptionResultStatus::DisplayHelp);

        options->setParserFlag(OptionParserFlag::DisableHelp);
        options->addOption({cHelpOption, "custom-help"_el}).setType(OptionType::Flag);
        result = parse(options, {"tool"_el, cHelpOption});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getFlag("custom-help"_el));

        options->clearParserFlag(OptionParserFlag::DisableVersion);
        REQUIRE(parse(options, {"tool"_el, "--version"_el}).status() == OptionResultStatus::DisplayVersion);
    }

    void testParseOrThrow() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto streamRedirect = redirectStandardStreams();
        auto options = Options::create();
        options->addOption(cNameOption).setType(OptionType::Text);

        auto manager = OptionManager{options};
        auto arguments = makeArgs({"tool"_el, cNameOption, "Ada"_el});
        const auto values = manager.parseOrThrow(arguments);
        REQUIRE(values);
        REQUIRE(values->getText(cNameOption) == "Ada"_el);

        arguments = makeArgs({"tool"_el, "--unknown"_el});
        REQUIRE_THROWS_AS(OptionError, manager.parseOrThrow(arguments));

        auto moduleMainCalled = false;
        auto moduleOptions = Options::create();
        auto module = OptionModule::create("run"_el);
        module->setMainFn([&moduleMainCalled](OptionValuesPtr) -> ExitCode {
            moduleMainCalled = true;
            return ExitCode{7};
        });
        module->addOption(cForceOption).setType(OptionType::Flag);
        moduleOptions->addModule(module);

        auto moduleManager = OptionManager{moduleOptions};
        auto moduleArguments = makeArgs({"tool"_el, "run"_el, cForceOption});
        const auto moduleValues = moduleManager.parseOrThrow(moduleArguments);
        REQUIRE(moduleValues);
        REQUIRE(moduleValues->moduleName() == "run"_el);
        REQUIRE(moduleValues->module() == module);
        REQUIRE(moduleValues->getFlag(cForceOption));
        REQUIRE_FALSE(moduleMainCalled);
    }

    void testParseOrThrowRendersDisplayRequests() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto output = el::stream::AnyStringBuilderStream::create();
        const auto error = el::stream::AnyStringBuilderStream::create();
        const auto streamRedirect = el::stream::redirectStandardStreams(output, error);
        auto options = Options::create();
        options->addOption(cVerboseOption)
            .setType(OptionType::Flag)
            .setHelpTitle("Verbose Output"_el)
            .setHelpDescription("Display additional diagnostics."_el);
        auto manager = OptionManager{options};

        auto arguments = makeArgs({"tool"_el, "--help=--verbose"_el});
        REQUIRE_FALSE(manager.parseOrThrow(arguments));
        auto outputText = StringConverter{output->toU8String()}.toStdString();
        REQUIRE_NOT_EQUAL(outputText.find("Verbose Output\n"), std::string::npos);
        REQUIRE_NOT_EQUAL(outputText.find("tool --verbose"), std::string::npos);

        arguments = makeArgs({"tool"_el, "--help=--verbsoe"_el});
        REQUIRE_FALSE(manager.parseOrThrow(arguments));
        outputText = StringConverter{output->toU8String()}.toStdString();
        REQUIRE_NOT_EQUAL(outputText.find("Matching help for"), std::string::npos);
        REQUIRE_NOT_EQUAL(outputText.find("--verbose"), std::string::npos);
        REQUIRE_EQUAL(outputText.find("--help=--verbose"), std::string::npos);

        auto moduleOptions = Options::create();
        moduleOptions->addModule(OptionModule::create("run"_el));
        auto moduleManager = OptionManager{moduleOptions};
        arguments = makeArgs({"tool"_el});
        REQUIRE_FALSE(moduleManager.parseOrThrow(arguments));
        outputText = StringConverter{output->toU8String()}.toStdString();
        REQUIRE_NOT_EQUAL(outputText.find("tool <module> [options]"), std::string::npos);
    }

    void testPositionalSuccess() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cPathOption).setType(OptionType::Text);
        options->addOption(cCountPositionalOption).setType(OptionType::Integer);
        options->addOption(cModePositionalOption)
            .setType(OptionType::Choice)
            .setChoices(OptionChoices::create({"fast"_el, "safe"_el}));

        const auto result = parse(options, {"tool"_el, "file.txt"_el, "42"_el, "SAFE"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText(cPathOption) == "file.txt"_el);
        REQUIRE_EQUAL(result.values()->getInteger(cCountPositionalOption), OptionInteger{42});
        REQUIRE(result.values()->getText(cModePositionalOption) == "safe"_el);
    }

    void testMixedOptionsAndTerminator() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cVerboseOption).setType(OptionType::Flag);
        options->addOption(cPathOption).setType(OptionType::Text);
        options->addOption(cArgumentsOption).setType(OptionType::Text).setMaximum(ArgumentCount{3U});

        const auto result = parse(options, {"tool"_el, cVerboseOption, "input.txt"_el, "--"_el, cHelpOption, "-x"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getFlag(cVerboseOption));
        REQUIRE(result.values()->getText(cPathOption) == "input.txt"_el);
        const auto arguments = result.values()->getTextList(cArgumentsOption);
        REQUIRE_EQUAL(arguments.count(), el::unit::ItemCount{2U});
        REQUIRE(arguments.get(el::unit::ItemIndex::zero()) == cHelpOption);
        REQUIRE(arguments.get(el::unit::ItemIndex{1U}) == "-x"_el);
    }

    void testModulePositionals() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cVerboseOption).setType(OptionType::Flag);
        auto module = OptionModule::create("copy"_el);
        module->addOption(cSourceOption).setType(OptionType::Text);
        module->addOption(cTargetOption).setType(OptionType::Text);
        options->addModule(module);

        const auto result = parse(options, {"tool"_el, "COPY"_el, cVerboseOption, "a.txt"_el, "b.txt"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->moduleName() == "copy"_el);
        REQUIRE(result.values()->module() == module);
        REQUIRE(result.values()->getFlag(cVerboseOption));
        REQUIRE(result.values()->getText(cSourceOption) == "a.txt"_el);
        REQUIRE(result.values()->getText(cTargetOption) == "b.txt"_el);
    }

    void testPositionalListReservationAndGreedy() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cInputsOption).setType(OptionType::Text).setMaximum(ArgumentCount{10U});
        options->addOption(cOutputOption).setType(OptionType::Text).setFlag(OptionFlag::Required);

        auto result = parse(options, {"tool"_el, "one"_el, "two"_el, "out"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        const auto inputs = result.values()->getTextList(cInputsOption);
        REQUIRE_EQUAL(inputs.count(), el::unit::ItemCount{2U});
        REQUIRE(inputs.get(el::unit::ItemIndex::zero()) == "one"_el);
        REQUIRE(inputs.get(el::unit::ItemIndex{1U}) == "two"_el);
        REQUIRE(result.values()->getText(cOutputOption) == "out"_el);

        options = Options::create();
        options->addOption(cInputsOption)
            .setType(OptionType::Text)
            .setMaximum(ArgumentCount{10U})
            .setFlag(OptionFlag::Greedy);
        options->addOption(cOutputOption).setType(OptionType::Text).setFlag(OptionFlag::Required);

        assertError(options, {"tool"_el, "one"_el, "two"_el, "out"_el}, OptionErrorReason::UnexpectedValueType);
    }

    void testPositionalErrorCases() {
        auto applicationScope = ApplicationTestScope<>{};
        assertError(makeOptionsWithTextOption(), {"tool"_el, "file.txt"_el}, OptionErrorReason::SyntaxError);

        auto options = Options::create();
        options->addOption(cPathOption).setType(OptionType::Text);
        assertError(options, {"tool"_el, "one"_el, "two"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        REQUIRE_THROWS_AS(el::err::ParameterFailure, options->addOption("1path"_el));

        options = Options::create();
        options->addOption(cPathOption).setType(OptionType::Text);
        options->addOption("PATH"_el).setType(OptionType::Text);
        assertError(options, {"tool"_el, "file.txt"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        options->addOption("verbose"_el).setType(OptionType::Flag);
        assertError(options, {"tool"_el, "true"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        options->addOption(cCountPositionalOption).setType(OptionType::Integer);
        assertError(options, {"tool"_el, "abc"_el}, OptionErrorReason::UnexpectedValueType);

        options = Options::create();
        options->addOption(cModePositionalOption)
            .setType(OptionType::Choice)
            .setChoices(OptionChoices::create({"fast"_el, "safe"_el}));
        assertError(options, {"tool"_el, "slow"_el}, OptionErrorReason::UnexpectedValueType);

        options = Options::create();
        options->addOption(cPathOption).setType(OptionType::Text).setFlag(OptionFlag::Required);
        const auto result = parse(options, {"tool"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
    }

    void testHelpBeforeAndAfterTerminator() {
        auto applicationScope = ApplicationTestScope<>{};
        auto preCallbackCalled = false;
        auto validatorCalled = false;
        auto options = Options::create();
        options->addOption(cPathOption)
            .setType(OptionType::Text)
            .setValidateFn([&validatorCalled](OptionValuePtr, OptionValuesPtr) -> void { validatorCalled = true; });
        options->optionSets().front()->setPreParsingFn(
            [&preCallbackCalled](OptionSetPtr) -> void { preCallbackCalled = true; });

        auto result = parse(options, {"tool"_el, cHelpOption});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
        REQUIRE_FALSE(preCallbackCalled);
        REQUIRE_FALSE(validatorCalled);

        result = parse(options, {"tool"_el, "--"_el, cHelpOption});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText(cPathOption) == cHelpOption);
        REQUIRE(preCallbackCalled);
        REQUIRE(validatorCalled);
    }

    void testErrorCases() {
        auto applicationScope = ApplicationTestScope<>{};
        assertError(makeOptionsWithTextOption(), {"tool"_el, "--missing"_el}, OptionErrorReason::UnknownName);
        assertError(makeDisabledOptions(), {"tool"_el, cOffOption}, OptionErrorReason::UnknownName);
        assertError(makeOptionsWithTextOption(), {"tool"_el, cNameOption}, OptionErrorReason::UnexpectedValueType);
        assertError(
            makeOptionsWithTextOption(),
            {"tool"_el, cNameOption, "--other"_el},
            OptionErrorReason::UnexpectedValueType);
        assertError(
            makeOptionsWithIntegerOption(),
            {"tool"_el, cCountOption, "abc"_el},
            OptionErrorReason::UnexpectedValueType);
        assertError(
            makeOptionsWithChoiceOption(), {"tool"_el, cModeOption, "slow"_el}, OptionErrorReason::UnexpectedValueType);
        assertError(
            makeOptionsWithTextOption(),
            {"tool"_el, cNameOption, "Ada"_el, cNameOption, "Bob"_el},
            OptionErrorReason::UnexpectedValueType);
        assertError(
            makeOptionsWithTextListOption(),
            {"tool"_el, cIncludeOption, "src"_el, cIncludeOption, "test"_el, cIncludeOption, "doc"_el},
            OptionErrorReason::SyntaxError);
        assertError(makeOptionsWithTextOption(), {"tool"_el, "file.txt"_el}, OptionErrorReason::SyntaxError);
        assertError(makeOptionsWithDuplicateNames(), {"tool"_el}, OptionErrorReason::SyntaxError);
    }

    void testFuzzySuggestionsForOptionsModulesAndChoices() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cVerboseOption);
        options->addOption("-x"_el);
        auto result = parse(options, {"tool"_el, "--verbsoe"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext()->suggestions().count() >= ItemCount::one());
        REQUIRE(result.errorContext()->suggestions().get(el::unit::ItemIndex::zero()) == cVerboseOption);

        result = parse(options, {"tool"_el, "--q"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext()->suggestions().isEmpty());

        options = Options::create();
        options->addOption(cModeOption).setChoices(OptionChoices::create({"fast"_el, "safe"_el}));
        result = parse(options, {"tool"_el, "--mode=fats"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE_EQUAL(result.errorContext()->suggestions().count(), ItemCount::one());
        REQUIRE(result.errorContext()->suggestions().get(el::unit::ItemIndex::zero()) == "fast"_el);

        options = Options::create();
        options->addModule(OptionModule::create("remove"_el));
        result = parse(options, {"tool"_el, "remvoe"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE_EQUAL(result.errorContext()->suggestions().count(), ItemCount::one());
        REQUIRE(result.errorContext()->suggestions().get(el::unit::ItemIndex::zero()) == "remove"_el);
    }

    void testDisabledOptionSetsAreAbsentAndHiddenOptionsParse() {
        auto applicationScope = ApplicationTestScope<>{};
        auto disabledPreCalled = false;
        auto options = Options::create();
        options->addOption(cVisibleOption).setType(OptionType::Flag);

        auto hiddenSet = OptionSet::create();
        hiddenSet->addOption(cSecretOption).setType(OptionType::Flag).setHelpVisibility(OptionHelpVisibility::Hidden);
        options->addSet(hiddenSet);

        auto disabledSet = OptionSet::create();
        disabledSet->setFlags(OptionFlag::Disabled);
        disabledSet->setPreParsingFn([&disabledPreCalled](OptionSetPtr) -> void { disabledPreCalled = true; });
        disabledSet->addOption(cOffOption).setType(OptionType::Flag);
        disabledSet->addOption(cVisibleOption).setType(OptionType::Flag);
        options->addSet(disabledSet);

        auto result = parse(options, {"tool"_el, cVisibleOption, cSecretOption});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getFlag(cVisibleOption));
        REQUIRE(result.values()->getFlag(cSecretOption));
        REQUIRE_FALSE(disabledPreCalled);

        result = parse(options, {"tool"_el, cOffOption});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::UnknownName);
        REQUIRE_FALSE(disabledPreCalled);
    }

    void testRequiredAndNegativeValues() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cRequiredOption).setType(OptionType::Text).setFlag(OptionFlag::Required);
        options->addOption(cCountOption).setType(OptionType::Integer);

        assertError(options, {"tool"_el}, OptionErrorReason::UnexpectedValueType);

        const auto result = parse(options, {"tool"_el, cRequiredOption, "value"_el, "--count=-1"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText(cRequiredOption) == "value"_el);
        REQUIRE_EQUAL(result.values()->getInteger(cCountOption), OptionInteger{-1});
    }

    void testEmptyRequiredPositionalDisplaysHelp() {
        auto applicationScope = ApplicationTestScope<>{};
        auto preParsingCalled = false;
        auto options = Options::create();
        options->addOption(cSourceOption).setType(OptionType::Text).setFlag(OptionFlag::Required);
        options->optionSets().front()->setPreParsingFn(
            [&preParsingCalled](OptionSetPtr) -> void { preParsingCalled = true; });

        auto result = parse(options, {"tool"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
        REQUIRE_FALSE(result.errorContext().has_value());
        REQUIRE_FALSE(preParsingCalled);

        options->setParserFlag(OptionParserFlag::ErrorOnEmptyRequiredPositionals);
        result = parse(options, {"tool"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::UnexpectedValueType);
        REQUIRE(preParsingCalled);
    }

    void testModuleOptionSuccess() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptionsWithGlobalAndModuleOptions();

        const auto result = parse(options, {"tool"_el, "REMOVE"_el, cVerboseOption, cForceOption, "--count=3"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->moduleName() == "remove"_el);
        REQUIRE(result.values()->module() == options->optionModules().front());
        REQUIRE(result.values());
        REQUIRE(result.values()->getFlag(cVerboseOption));
        REQUIRE(result.values()->getFlag(cForceOption));
        REQUIRE_EQUAL(result.values()->getInteger(cCountOption), OptionInteger{3});
    }

    void testModuleHelpAndVersionStatus() {
        auto applicationScope = ApplicationTestScope<>{};
        auto callbackCalled = false;
        auto options = makeOptionsWithGlobalAndModuleOptions();
        options->optionModules().front()->setPreParsingFn(
            [&callbackCalled](OptionModulePtr) -> void { callbackCalled = true; });

        auto result = parse(options, {"tool"_el, cHelpOption});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
        REQUIRE(result.values()->moduleName().isEmpty());
        REQUIRE_FALSE(result.values()->module());

        result = parse(options, {"tool"_el, "remove"_el, cHelpOption});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
        REQUIRE(result.values()->moduleName() == "remove"_el);
        REQUIRE(result.values()->module() == options->optionModules().front());

        result = parse(options, {"tool"_el, "REMOVE"_el, "-h"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
        REQUIRE(result.values()->moduleName() == "remove"_el);
        REQUIRE(result.values()->module() == options->optionModules().front());

        result = parse(options, {"tool"_el, "remove"_el, "--version"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayVersion);
        REQUIRE(result.values()->moduleName() == "remove"_el);
        REQUIRE(result.values()->module() == options->optionModules().front());
        REQUIRE_FALSE(callbackCalled);
    }

    void testModuleErrorCases() {
        auto applicationScope = ApplicationTestScope<>{};
        auto result = parse(makeOptionsWithGlobalAndModuleOptions(), {"tool"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayModuleOverview);
        REQUIRE_FALSE(result.values()->module());
        REQUIRE_FALSE(result.errorContext().has_value());

        assertError(makeOptionsWithGlobalAndModuleOptions(), {"tool"_el, "missing"_el}, OptionErrorReason::UnknownName);
        assertError(
            makeOptionsWithGlobalAndModuleOptions(), {"tool"_el, cVerboseOption}, OptionErrorReason::SyntaxError);
        assertError(
            makeOptionsWithGlobalAndModuleOptions(), {"tool"_el, "--"_el, "remove"_el}, OptionErrorReason::SyntaxError);
        assertError(makeOptionsWithDuplicateModuleNames(), {"tool"_el, "remove"_el}, OptionErrorReason::SyntaxError);

        auto disabledGlobalOptions = makeOptionsWithDisabledGlobalOption();
        result = parse(disabledGlobalOptions, {"tool"_el, "remove"_el, cVerboseOption});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.values()->module() == disabledGlobalOptions->optionModules().front());
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::UnknownName);

        assertError(
            makeOptionsWithDisabledModuleOption(),
            {"tool"_el, "remove"_el, cForceOption},
            OptionErrorReason::UnknownName);
        assertError(
            makeOptionsWithGlobalAndModuleOptions(),
            {"tool"_el, "remove"_el, "file.txt"_el},
            OptionErrorReason::SyntaxError);
    }

    void testMissingModuleOverviewAndStrictModeSuppressCallbacks() {
        auto applicationScope = ApplicationTestScope<>{};
        auto callbackCalled = false;
        auto options = Options::create();
        auto module = OptionModule::create("run"_el);
        module->setPreParsingFn([&callbackCalled](const OptionModulePtr &) -> void { callbackCalled = true; });
        options->addModule(module);

        auto result = parse(options, {"tool"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayModuleOverview);
        REQUIRE_FALSE(callbackCalled);

        options->setParserFlag(OptionParserFlag::ErrorOnMissingModule);
        result = parse(options, {"tool"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::SyntaxError);
        REQUIRE_FALSE(callbackCalled);
    }

    void testModulePreCallbackCanEditGlobalOptions() {
        auto applicationScope = ApplicationTestScope<>{};
        auto normalOptions = makeOptionsWithGlobalAndModuleOptions();
        auto normalResult = parse(normalOptions, {"tool"_el, "remove"_el, cVOption});
        REQUIRE(normalResult.status() == OptionResultStatus::Success);
        REQUIRE(normalResult.values()->getFlag(cVerboseOption));

        auto callbackCalled = false;
        auto specialOptions = makeOptionsWithGlobalAndModuleOptions("special"_el);
        specialOptions->optionModules().front()->setPreParsingFn(
            [specialOptions, &callbackCalled](OptionModulePtr) -> void {
                callbackCalled = true;
                specialOptions->editOption(cVerboseOption).setFlag(OptionFlag::Disabled);
            });

        assertError(specialOptions, {"tool"_el, "special"_el, cVOption}, OptionErrorReason::UnknownName);
        REQUIRE(callbackCalled);
    }

    void testModulePreCallbackErrorContext() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = makeOptionsWithGlobalAndModuleOptions();
        options->optionModules().front()->setPreParsingFn([](OptionModulePtr) -> void {
            auto context = OptionErrorContext{};
            context.setReason(OptionErrorReason::SyntaxError).setDescription("Rejected module"_el);
            throw OptionError{context};
        });

        const auto result = parse(options, {"tool"_el, "remove"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.values()->moduleName() == "remove"_el);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::SyntaxError);
        REQUIRE(result.errorContext()->module() == options->optionModules().front());
        REQUIRE(result.errorContext()->description() == "Rejected module"_el);
        REQUIRE(result.errorContext()->argumentIndex().isNoIndex());
    }

    void testOptionSetPreCallbackCanEditOptions() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cLateOption).setType(OptionType::Flag).setFlag(OptionFlag::Disabled);
        options->optionSets().front()->setPreParsingFn(
            [options](OptionSetPtr) -> void { options->editOption(cLateOption).clearFlag(OptionFlag::Disabled); });

        const auto result = parse(options, {"tool"_el, cLateOption});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getFlag(cLateOption));
    }

    void testCallbackOrder() {
        auto applicationScope = ApplicationTestScope<>{};
        auto calls = std::vector<StringEditor>{};
        auto options = Options::create();
        options->addOption({cVOption, cVerboseOption}).setType(OptionType::Flag);
        const auto rootSet = options->optionSets().front();
        rootSet->setPreParsingFn([&calls](OptionSetPtr) -> void { calls.emplace_back("root-pre"_el); });
        rootSet->setPostParsingFn([&calls](OptionValuesPtr) -> void { calls.emplace_back("root-post"_el); });

        const auto module = OptionModule::create("run"_el);
        module->setPreParsingFn([&calls](OptionModulePtr) -> void { calls.emplace_back("module-pre"_el); });
        module->setPostParsingFn([&calls](OptionValuesPtr) -> void { calls.emplace_back("module-post"_el); });
        module->addOption(cForceOption).setType(OptionType::Flag);
        const auto moduleSet = module->optionSets().front();
        moduleSet->setPreParsingFn([&calls](OptionSetPtr) -> void { calls.emplace_back("module-set-pre"_el); });
        moduleSet->setPostParsingFn([&calls](OptionValuesPtr) -> void { calls.emplace_back("module-set-post"_el); });
        options->addModule(module);

        const auto result = parse(options, {"tool"_el, "run"_el, cVerboseOption, cForceOption});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE_EQUAL(calls.size(), 6U);
        REQUIRE(calls.at(0) == "module-pre"_el);
        REQUIRE(calls.at(1) == "root-pre"_el);
        REQUIRE(calls.at(2) == "module-set-pre"_el);
        REQUIRE(calls.at(3) == "root-post"_el);
        REQUIRE(calls.at(4) == "module-set-post"_el);
        REQUIRE(calls.at(5) == "module-post"_el);
    }

    void testValidators() {
        auto applicationScope = ApplicationTestScope<>{};
        auto parsedValidatorCalled = false;
        auto parsedValidatorGotValue = false;
        auto parsedValidatorGotValues = false;
        auto parsedValidatorGotIndex = false;
        auto parsedValidatorGotFlagCount = false;
        auto defaultValidatorCalled = false;
        auto defaultValidatorGotValue = false;
        auto defaultValidatorGotValues = false;
        auto defaultValidatorGotNoIndex = false;
        auto skippedOptionalValidatorCalled = false;
        auto skippedDisabledValidatorCalled = false;
        auto options = Options::create();
        options->addOption(cNameOption)
            .setType(OptionType::Text)
            .setValidateFn(
                [&parsedValidatorCalled, &parsedValidatorGotValue, &parsedValidatorGotValues, &parsedValidatorGotIndex](
                    OptionValuePtr value, OptionValuesPtr values) -> void {
                    parsedValidatorCalled = true;
                    parsedValidatorGotValue = value == values->value(cNameOption);
                    parsedValidatorGotValues =
                        values->getText(cNameOption) == "Ada"_el && values->getText(cModeOption) == "auto"_el;
                    parsedValidatorGotIndex = value->argumentIndex() == ArgumentIndex{2U};
                });
        options->addOption({cVOption, cVerboseOption})
            .setType(OptionType::Flag)
            .setValidateFn([&parsedValidatorGotFlagCount](OptionValuePtr value, OptionValuesPtr values) -> void {
                parsedValidatorGotFlagCount = value->flagCount() == ArgumentCount{2U} &&
                    values->getFlagCount(cVerboseOption) == ArgumentCount{2U};
            });
        options->addOption(cModeOption)
            .setType(OptionType::Text)
            .setDefaultValue(StringEditor{"auto"_el})
            .setValidateFn(
                [&defaultValidatorCalled,
                    &defaultValidatorGotValue,
                    &defaultValidatorGotValues,
                    &defaultValidatorGotNoIndex](OptionValuePtr value, OptionValuesPtr values) -> void {
                    defaultValidatorCalled = true;
                    defaultValidatorGotValue = value == values->value(cModeOption);
                    defaultValidatorGotValues = values->getText(cModeOption) == "auto"_el;
                    defaultValidatorGotNoIndex = value->argumentIndex().isNoIndex();
                });
        options->addOption("--optional"_el)
            .setType(OptionType::Text)
            .setValidateFn([&skippedOptionalValidatorCalled](OptionValuePtr, OptionValuesPtr) -> void {
                skippedOptionalValidatorCalled = true;
            });
        options->addOption("--disabled"_el)
            .setType(OptionType::Flag)
            .setFlag(OptionFlag::Disabled)
            .setDefaultValue(true)
            .setValidateFn([&skippedDisabledValidatorCalled](OptionValuePtr, OptionValuesPtr) -> void {
                skippedDisabledValidatorCalled = true;
            });

        const auto result = parse(options, {"tool"_el, cNameOption, "Ada"_el, "-vv"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(parsedValidatorCalled);
        REQUIRE(parsedValidatorGotValue);
        REQUIRE(parsedValidatorGotValues);
        REQUIRE(parsedValidatorGotIndex);
        REQUIRE(parsedValidatorGotFlagCount);
        REQUIRE(defaultValidatorCalled);
        REQUIRE(defaultValidatorGotValue);
        REQUIRE(defaultValidatorGotValues);
        REQUIRE(defaultValidatorGotNoIndex);
        REQUIRE_FALSE(skippedOptionalValidatorCalled);
        REQUIRE_FALSE(skippedDisabledValidatorCalled);
    }

    void testValidatorErrorContext() {
        auto applicationScope = ApplicationTestScope<>{};
        auto postCalled = false;
        auto options = Options::create();
        auto module = OptionModule::create("run"_el);
        const auto editor = module->addOption(cNameOption)
                                .setType(OptionType::Text)
                                .setValidateFn([](OptionValuePtr, OptionValuesPtr) -> void {
                                    auto context = OptionErrorContext{};
                                    context.setDescription("Invalid name"_el);
                                    throw OptionError{context};
                                });
        const auto option = editor.option();
        const auto optionSet = module->optionSets().front();
        optionSet->setPostParsingFn([&postCalled](OptionValuesPtr) -> void { postCalled = true; });
        options->addModule(module);

        const auto result = parse(options, {"tool"_el, "run"_el, cNameOption, "Ada"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::ValidationError);
        REQUIRE(result.errorContext()->module() == module);
        REQUIRE(result.errorContext()->option() == option);
        REQUIRE(result.errorContext()->optionSet() == optionSet);
        REQUIRE(result.errorContext()->argumentIndex() == ArgumentIndex{3U});
        REQUIRE_FALSE(postCalled);
    }

    void testCallbackErrorDefaults() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption(cNameOption).setType(OptionType::Text);
        const auto optionSet = options->optionSets().front();
        optionSet->setPreParsingFn([](OptionSetPtr) -> void { throw OptionError{OptionErrorContext{}}; });

        auto result = parse(options, {"tool"_el, cNameOption, "Ada"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::None);
        REQUIRE(result.errorContext()->optionSet() == optionSet);

        options = Options::create();
        options->addOption(cNameOption).setType(OptionType::Text);
        const auto postOptionSet = options->optionSets().front();
        postOptionSet->setPostParsingFn([](OptionValuesPtr) -> void { throw OptionError{OptionErrorContext{}}; });

        result = parse(options, {"tool"_el, cNameOption, "Ada"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::ValidationError);
        REQUIRE(result.errorContext()->optionSet() == postOptionSet);
    }

    void testNoCallbacksForHelpAndNoPostAfterErrors() {
        auto applicationScope = ApplicationTestScope<>{};
        auto callbackCalled = false;
        auto postCalled = false;
        auto validatorCalled = false;
        auto options = Options::create();
        options->addOption(cNameOption)
            .setType(OptionType::Text)
            .setValidateFn([&validatorCalled](OptionValuePtr, OptionValuesPtr) -> void { validatorCalled = true; });
        const auto optionSet = options->optionSets().front();
        optionSet->setPreParsingFn([&callbackCalled](OptionSetPtr) -> void { callbackCalled = true; });
        optionSet->setPostParsingFn([&postCalled](OptionValuesPtr) -> void { postCalled = true; });

        auto result = parse(options, {"tool"_el, cHelpOption});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
        REQUIRE_FALSE(callbackCalled);
        REQUIRE_FALSE(postCalled);
        REQUIRE_FALSE(validatorCalled);

        result = parse(options, {"tool"_el, cNameOption});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(callbackCalled);
        REQUIRE_FALSE(postCalled);
        REQUIRE_FALSE(validatorCalled);
    }

    void testParseOrThrowReportsValidationErrors() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto streamRedirect = redirectStandardStreams();
        auto options = Options::create();
        options->addOption(cNameOption)
            .setType(OptionType::Text)
            .setValidateFn([](OptionValuePtr, OptionValuesPtr) -> void {
                auto context = OptionErrorContext{};
                context.setDescription("Invalid name"_el);
                throw OptionError{context};
            });

        auto manager = OptionManager{options};

        auto arguments = makeArgs({"tool"_el, cNameOption, "Ada"_el});
        REQUIRE_THROWS_AS(OptionError, manager.parseOrThrow(arguments));

        arguments = makeArgs({"tool"_el, cNameOption, "Ada"_el});
        const auto result = manager.parse(arguments);
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::ValidationError);
        REQUIRE_FALSE(manager.errorDocument(result.errorContext().value()).isEmpty());
    }

private:
    [[nodiscard]] static auto redirectStandardStreams() -> el::stream::StandardStreamRedirect {
        return el::stream::redirectStandardStreams(
            el::stream::AnyStringBuilderStream::create(), el::stream::AnyStringBuilderStream::create());
    }

    [[nodiscard]] static auto makeArgs(std::initializer_list<String> args) -> CommandLineArguments {
        auto result = CommandLineArguments{};
        result.reserve(ItemCount::fromInteger(args.size()));
        for (const auto &arg : args) {
            result.append(arg.copy());
        }
        return result;
    }

    [[nodiscard]] static auto parse(const OptionsPtr &options, std::initializer_list<String> args) -> OptionResult {
        auto manager = OptionManager{options};
        auto arguments = makeArgs(args);
        return manager.parse(arguments);
    }

    void assertError(const OptionsPtr &options, std::initializer_list<String> args, const OptionErrorReason reason) {
        const auto result = parse(options, args);
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == reason);
    }

    void requireArgumentIndexes(const OptionValuePtr &value, std::initializer_list<ArgumentIndex> indexes) {
        REQUIRE(value);
        REQUIRE_EQUAL(value->argumentIndexes().size(), indexes.size());
        auto index = std::size_t{0};
        for (const auto expectedIndex : indexes) {
            REQUIRE(value->argumentIndexes().at(index) == expectedIndex);
            ++index;
        }
        if (indexes.size() == 0U) {
            REQUIRE(value->argumentIndex().isNoIndex());
        } else {
            REQUIRE(value->argumentIndex() == *indexes.begin());
        }
    }

    [[nodiscard]] static auto makeOptionsWithTextOption() -> OptionsPtr {
        auto options = Options::create();
        options->addOption(cNameOption).setType(OptionType::Text);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithIntegerOption() -> OptionsPtr {
        auto options = Options::create();
        options->addOption(cCountOption).setType(OptionType::Integer);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithChoiceOption() -> OptionsPtr {
        auto options = Options::create();
        options->addOption(cModeOption)
            .setType(OptionType::Choice)
            .setChoices(OptionChoices::create({"fast"_el, "safe"_el}));
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithTextListOption() -> OptionsPtr {
        auto options = Options::create();
        options->addOption(cIncludeOption).setType(OptionType::Text).setMaximum(ArgumentCount{2U});
        return options;
    }

    [[nodiscard]] static auto makeDisabledOptions() -> OptionsPtr {
        auto options = Options::create();
        options->addOption(cOffOption).setType(OptionType::Flag).setFlag(OptionFlag::Disabled);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithDuplicateNames() -> OptionsPtr {
        auto options = Options::create();
        options->addOption("--path"_el).setType(OptionType::Text);
        options->addOption("--PATH"_el).setType(OptionType::Text);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithGlobalAndModuleOptions(String moduleName = "remove"_el) -> OptionsPtr {
        auto options = Options::create();
        options->addOption({cVOption, cVerboseOption}).setType(OptionType::Flag);
        auto module = OptionModule::create(moduleName);
        module->addOption(cForceOption).setType(OptionType::Flag);
        module->addOption(cCountOption).setType(OptionType::Integer);
        options->addModule(module);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithDuplicateModuleNames() -> OptionsPtr {
        auto options = Options::create();
        options->addOption(cForceOption).setType(OptionType::Flag);
        auto module = OptionModule::create("remove"_el);
        module->addOption("--FORCE"_el).setType(OptionType::Flag);
        options->addModule(module);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithDisabledGlobalOption() -> OptionsPtr {
        auto options = makeOptionsWithGlobalAndModuleOptions();
        options->editOption(cVerboseOption).setFlag(OptionFlag::Disabled);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithDisabledModuleOption() -> OptionsPtr {
        auto options = makeOptionsWithGlobalAndModuleOptions();
        options->optionModules().front()->editOption(cForceOption).setFlag(OptionFlag::Disabled);
        return options;
    }
};
