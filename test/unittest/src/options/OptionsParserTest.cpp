// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/CommandLineArguments.hpp>
#include <erbsland/err/OptionError.hpp>
#include <erbsland/options/OptionChoices.hpp>
#include <erbsland/options/OptionErrorContext.hpp>
#include <erbsland/options/OptionManager.hpp>
#include <erbsland/options/OptionModule.hpp>
#include <erbsland/options/OptionRenderer.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionSet.hpp>
#include <erbsland/options/OptionValue.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>
#include <memory>
#include <vector>

using el::core::CommandLineArguments;
using el::err::OptionError;
using el::text::String;
using el::text::StringConverter;
using el::text::StringView;
using el::unit::ArgumentCount;
using el::unit::ArgumentIndex;
using el::unit::ExitCode;
using namespace el::options;
using namespace el::text::literals;

class ParserTestRenderer final : public OptionRenderer {
public:
    void displayHelp(const OptionsPtr &, StringView) override {}
    void displayVersion(const OptionsPtr &, StringView) override {}
    void displayError(const OptionsPtr &, const OptionErrorContext &errorContext) override {
        errorDisplayed = true;
        errorReason = errorContext.reason();
    }

    bool errorDisplayed{false};
    OptionErrorReason errorReason{OptionErrorReason::None};
};

TESTED_TARGETS(OptionManager OptionParser)
class OptionsParserTest final : public el::UnitTest {
public:
    void testRootOptionSuccess() {
        auto options = Options::create();
        options->addOption({"-v"_el, "--verbose"_el}).setType(OptionType::Flag);
        options->addOption({"-a"_el, "--all"_el}).setType(OptionType::Flag);
        options->addOption({"-b"_el, "--binary"_el}).setType(OptionType::Flag);
        options->addOption({"-c"_el, "--create"_el}).setType(OptionType::Flag);
        options->addOption({"-n"_el, "--name"_el}).setType(OptionType::Text);
        options->addOption({"-i"_el, "--iterations"_el}).setType(OptionType::Integer);
        options->addOption("--mode"_el)
            .setType(OptionType::Choice)
            .setChoices(OptionChoices::create({"fast"_el, "safe"_el}));

        const auto result = parse(
            options,
            {"tool"_el,
                "--VERBOSE"_el,
                "-abc"_el,
                "--name"_el,
                "Ada"_el,
                "--iterations=42"_el,
                "--mode"_el,
                "SAFE"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values() != nullptr);
        REQUIRE(result.values()->getFlag("--verbose"_el));
        REQUIRE(result.values()->getFlag("--all"_el));
        REQUIRE(result.values()->getFlag("--binary"_el));
        REQUIRE(result.values()->getFlag("--create"_el));
        REQUIRE(result.values()->getText("--name"_el) == "Ada"_el);
        REQUIRE_EQUAL(result.values()->getInteger("--iterations"_el), OptionInteger{42});
        REQUIRE(result.values()->getText("--mode"_el) == "safe"_el);
    }

    void testListValuesAndDefaults() {
        auto options = Options::create();
        options->addOption("--include"_el).setType(OptionType::Text).setMaximum(ArgumentCount{3U});
        options->addOption("--count"_el).setType(OptionType::Integer).setMaximum(ArgumentCount{2U});
        options->addOption("--threads"_el).setType(OptionType::Integer).setDefaultValue(OptionInteger{4});
        options->addOption("--color"_el)
            .setType(OptionType::Choice)
            .setChoices(OptionChoices::create({"auto"_el, "never"_el}))
            .setDefaultValue(String{"AUTO"_el});

        const auto result = parse(
            options,
            {"tool"_el, "--include"_el, "src"_el, "--include"_el, "test"_el, "--count"_el, "1"_el, "--count=2"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        const auto includes = result.values()->getTextList("--include"_el);
        REQUIRE_EQUAL(includes.size(), 2U);
        REQUIRE(includes.at(0) == "src"_el);
        REQUIRE(includes.at(1) == "test"_el);
        const auto counts = result.values()->getIntegerList("--count"_el);
        REQUIRE_EQUAL(counts.size(), 2U);
        REQUIRE_EQUAL(counts.at(0), OptionInteger{1});
        REQUIRE_EQUAL(counts.at(1), OptionInteger{2});
        REQUIRE_EQUAL(result.values()->getInteger("--threads"_el), OptionInteger{4});
        REQUIRE(result.values()->getText("--color"_el) == "auto"_el);
    }

    void testValueArgumentIndexes() {
        auto options = Options::create();
        options->addOption("--equals"_el).setType(OptionType::Text);
        options->addOption("--space"_el).setType(OptionType::Text);
        options->addOption("-e"_el).setType(OptionType::Text);
        options->addOption("-s"_el).setType(OptionType::Text);

        auto result = parse(options, {"tool"_el, "--equals=A"_el, "--space"_el, "B"_el, "-e=C"_el, "-s"_el, "D"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        requireArgumentIndexes(result.values()->value("--equals"_el), {ArgumentIndex{1U}});
        requireArgumentIndexes(result.values()->value("--space"_el), {ArgumentIndex{3U}});
        requireArgumentIndexes(result.values()->value("-e"_el), {ArgumentIndex{4U}});
        requireArgumentIndexes(result.values()->value("-s"_el), {ArgumentIndex{6U}});

        options = Options::create();
        options->addOption("path"_el).setType(OptionType::Text);
        options->addOption("literal"_el).setType(OptionType::Text);
        result = parse(options, {"tool"_el, "input.txt"_el, "--"_el, "--help"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        requireArgumentIndexes(result.values()->value("path"_el), {ArgumentIndex{1U}});
        requireArgumentIndexes(result.values()->value("literal"_el), {ArgumentIndex{3U}});
    }

    void testListAndDefaultArgumentIndexes() {
        auto options = Options::create();
        options->addOption("--include"_el).setType(OptionType::Text).setMaximum(ArgumentCount{3U});
        options->addOption("files"_el).setType(OptionType::Text).setMaximum(ArgumentCount{2U});
        options->addOption("--mode"_el).setType(OptionType::Text).setDefaultValue(String{"auto"_el});

        const auto result =
            parse(options, {"tool"_el, "--include"_el, "src"_el, "--include=test"_el, "one"_el, "two"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        requireArgumentIndexes(result.values()->value("--include"_el), {ArgumentIndex{2U}, ArgumentIndex{3U}});
        requireArgumentIndexes(result.values()->value("files"_el), {ArgumentIndex{4U}, ArgumentIndex{5U}});
        REQUIRE(result.values()->value("--mode"_el)->argumentIndex().isNoIndex());
        REQUIRE(result.values()->value("--mode"_el)->argumentIndexes().empty());
    }

    void testFlagArgumentIndexes() {
        auto options = Options::create();
        options->addOption({"-a"_el, "--all"_el}).setType(OptionType::Flag);
        options->addOption("-b"_el).setType(OptionType::Flag);
        options->addOption({"-v"_el, "--verbose"_el}).setType(OptionType::Flag);
        options->addOption("--default-on"_el).setType(OptionType::Flag).setDefaultValue(true);

        const auto result =
            parse(options, {"tool"_el, "-ab"_el, "--verbose"_el, "-avv"_el, "--all"_el, "--verbose"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        requireArgumentIndexes(
            result.values()->value("--all"_el), {ArgumentIndex{1U}, ArgumentIndex{3U}, ArgumentIndex{4U}});
        requireArgumentIndexes(result.values()->value("-b"_el), {ArgumentIndex{1U}});
        requireArgumentIndexes(
            result.values()->value("--verbose"_el),
            {ArgumentIndex{2U}, ArgumentIndex{3U}, ArgumentIndex{3U}, ArgumentIndex{5U}});
        REQUIRE(result.values()->getFlag("--default-on"_el));
        REQUIRE(result.values()->value("--default-on"_el)->argumentIndexes().empty());
        REQUIRE_EQUAL(result.values()->getFlagCount("--all"_el), ArgumentCount{3U});
        REQUIRE_EQUAL(result.values()->getFlagCount("-a"_el), ArgumentCount{3U});
        REQUIRE_EQUAL(result.values()->getFlagCount("-b"_el), ArgumentCount::one());
        REQUIRE_EQUAL(result.values()->getFlagCount("--verbose"_el), ArgumentCount{4U});
        REQUIRE_EQUAL(result.values()->getFlagCount("-v"_el), ArgumentCount{4U});
        REQUIRE_EQUAL(result.values()->getFlagCount("--default-on"_el), ArgumentCount::zero());
        REQUIRE_EQUAL(result.values()->valueCount("--all"_el), ArgumentCount::one());
    }

    void testHelpAndVersionStatus() {
        auto options = Options::create();
        options->addOption("--name"_el).setType(OptionType::Text);
        options->addOption("--verbose"_el).setType(OptionType::Flag);

        auto helpResult = parse(options, {"tool"_el, "-h"_el});
        REQUIRE(helpResult.status() == OptionResultStatus::DisplayHelp);
        REQUIRE_EQUAL(helpResult.values()->getFlagCount("--verbose"_el), ArgumentCount::zero());
        REQUIRE(parse(options, {"tool"_el, "--help"_el}).status() == OptionResultStatus::DisplayHelp);
        REQUIRE(parse(options, {"tool"_el, "--version"_el}).status() == OptionResultStatus::DisplayVersion);
        REQUIRE(parse(options, {"tool"_el, "--unknown"_el, "--help"_el}).status() == OptionResultStatus::DisplayHelp);
    }

    void testParseOrThrow() {
        auto options = Options::create();
        options->addOption("--name"_el).setType(OptionType::Text);

        auto manager = OptionManager{options};
        const auto values = manager.parseOrThrow(makeArgs({"tool"_el, "--name"_el, "Ada"_el}));
        REQUIRE(values != nullptr);
        REQUIRE(values->getText("--name"_el) == "Ada"_el);

        REQUIRE_THROWS_AS(OptionError, manager.parseOrThrow(makeArgs({"tool"_el, "--unknown"_el})));

        auto moduleMainCalled = false;
        auto moduleOptions = Options::create();
        auto module = OptionModule::create("run"_el);
        module->setMainFn([&moduleMainCalled](OptionValuesPtr) -> ExitCode {
            moduleMainCalled = true;
            return ExitCode{7};
        });
        module->addOption("--force"_el).setType(OptionType::Flag);
        moduleOptions->addModule(module);

        auto moduleManager = OptionManager{moduleOptions};
        const auto moduleValues = moduleManager.parseOrThrow(makeArgs({"tool"_el, "run"_el, "--force"_el}));
        REQUIRE(moduleValues != nullptr);
        REQUIRE(moduleValues->moduleName() == "run"_el);
        REQUIRE(moduleValues->module() == module);
        REQUIRE(moduleValues->getFlag("--force"_el));
        REQUIRE_FALSE(moduleMainCalled);
    }

    void testPositionalSuccess() {
        auto options = Options::create();
        options->addOption("path"_el).setType(OptionType::Text);
        options->addOption("count"_el).setType(OptionType::Integer);
        options->addOption("mode"_el)
            .setType(OptionType::Choice)
            .setChoices(OptionChoices::create({"fast"_el, "safe"_el}));

        const auto result = parse(options, {"tool"_el, "file.txt"_el, "42"_el, "SAFE"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText("path"_el) == "file.txt"_el);
        REQUIRE_EQUAL(result.values()->getInteger("count"_el), OptionInteger{42});
        REQUIRE(result.values()->getText("mode"_el) == "safe"_el);
    }

    void testMixedOptionsAndTerminator() {
        auto options = Options::create();
        options->addOption("--verbose"_el).setType(OptionType::Flag);
        options->addOption("path"_el).setType(OptionType::Text);
        options->addOption("arguments"_el).setType(OptionType::Text).setMaximum(ArgumentCount{3U});

        const auto result = parse(options, {"tool"_el, "--verbose"_el, "input.txt"_el, "--"_el, "--help"_el, "-x"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getFlag("--verbose"_el));
        REQUIRE(result.values()->getText("path"_el) == "input.txt"_el);
        const auto arguments = result.values()->getTextList("arguments"_el);
        REQUIRE_EQUAL(arguments.size(), 2U);
        REQUIRE(arguments.at(0) == "--help"_el);
        REQUIRE(arguments.at(1) == "-x"_el);
    }

    void testModulePositionals() {
        auto options = Options::create();
        options->addOption("--verbose"_el).setType(OptionType::Flag);
        auto module = OptionModule::create("copy"_el);
        module->addOption("source"_el).setType(OptionType::Text);
        module->addOption("target"_el).setType(OptionType::Text);
        options->addModule(module);

        const auto result = parse(options, {"tool"_el, "COPY"_el, "--verbose"_el, "a.txt"_el, "b.txt"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->moduleName() == "copy"_el);
        REQUIRE(result.values()->module() == module);
        REQUIRE(result.values()->getFlag("--verbose"_el));
        REQUIRE(result.values()->getText("source"_el) == "a.txt"_el);
        REQUIRE(result.values()->getText("target"_el) == "b.txt"_el);
    }

    void testPositionalListReservationAndGreedy() {
        auto options = Options::create();
        options->addOption("inputs"_el).setType(OptionType::Text).setMaximum(ArgumentCount{10U});
        options->addOption("output"_el).setType(OptionType::Text).setFlag(OptionFlag::Required);

        auto result = parse(options, {"tool"_el, "one"_el, "two"_el, "out"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        const auto inputs = result.values()->getTextList("inputs"_el);
        REQUIRE_EQUAL(inputs.size(), 2U);
        REQUIRE(inputs.at(0) == "one"_el);
        REQUIRE(inputs.at(1) == "two"_el);
        REQUIRE(result.values()->getText("output"_el) == "out"_el);

        options = Options::create();
        options->addOption("inputs"_el)
            .setType(OptionType::Text)
            .setMaximum(ArgumentCount{10U})
            .setFlag(OptionFlag::Greedy);
        options->addOption("output"_el).setType(OptionType::Text).setFlag(OptionFlag::Required);

        assertError(options, {"tool"_el, "one"_el, "two"_el, "out"_el}, OptionErrorReason::UnexpectedValueType);
    }

    void testPositionalErrorCases() {
        assertError(makeOptionsWithTextOption(), {"tool"_el, "file.txt"_el}, OptionErrorReason::SyntaxError);

        auto options = Options::create();
        options->addOption("path"_el).setType(OptionType::Text);
        assertError(options, {"tool"_el, "one"_el, "two"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        options->addOption("1path"_el).setType(OptionType::Text);
        assertError(options, {"tool"_el, "file.txt"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        options->addOption("path"_el).setType(OptionType::Text);
        options->addOption("PATH"_el).setType(OptionType::Text);
        assertError(options, {"tool"_el, "file.txt"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        options->addOption("verbose"_el).setType(OptionType::Flag);
        assertError(options, {"tool"_el, "true"_el}, OptionErrorReason::SyntaxError);

        options = Options::create();
        options->addOption("count"_el).setType(OptionType::Integer);
        assertError(options, {"tool"_el, "abc"_el}, OptionErrorReason::UnexpectedValueType);

        options = Options::create();
        options->addOption("mode"_el)
            .setType(OptionType::Choice)
            .setChoices(OptionChoices::create({"fast"_el, "safe"_el}));
        assertError(options, {"tool"_el, "slow"_el}, OptionErrorReason::UnexpectedValueType);

        options = Options::create();
        options->addOption("path"_el).setType(OptionType::Text).setFlag(OptionFlag::Required);
        assertError(options, {"tool"_el}, OptionErrorReason::UnexpectedValueType);
    }

    void testHelpBeforeAndAfterTerminator() {
        auto preCallbackCalled = false;
        auto validatorCalled = false;
        auto options = Options::create();
        options->addOption("path"_el)
            .setType(OptionType::Text)
            .setValidateFn([&validatorCalled](OptionValuePtr, OptionValuesPtr) -> void { validatorCalled = true; });
        options->optionSets().front()->setPreParsingFn(
            [&preCallbackCalled](OptionSetPtr) -> void { preCallbackCalled = true; });

        auto result = parse(options, {"tool"_el, "--help"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
        REQUIRE_FALSE(preCallbackCalled);
        REQUIRE_FALSE(validatorCalled);

        result = parse(options, {"tool"_el, "--"_el, "--help"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText("path"_el) == "--help"_el);
        REQUIRE(preCallbackCalled);
        REQUIRE(validatorCalled);
    }

    void testErrorCases() {
        assertError(makeOptionsWithTextOption(), {"tool"_el, "--missing"_el}, OptionErrorReason::UnknownName);
        assertError(makeDisabledOptions(), {"tool"_el, "--off"_el}, OptionErrorReason::UnknownName);
        assertError(makeOptionsWithTextOption(), {"tool"_el, "--name"_el}, OptionErrorReason::UnexpectedValueType);
        assertError(
            makeOptionsWithTextOption(),
            {"tool"_el, "--name"_el, "--other"_el},
            OptionErrorReason::UnexpectedValueType);
        assertError(
            makeOptionsWithIntegerOption(),
            {"tool"_el, "--count"_el, "abc"_el},
            OptionErrorReason::UnexpectedValueType);
        assertError(
            makeOptionsWithChoiceOption(), {"tool"_el, "--mode"_el, "slow"_el}, OptionErrorReason::UnexpectedValueType);
        assertError(
            makeOptionsWithTextOption(),
            {"tool"_el, "--name"_el, "Ada"_el, "--name"_el, "Bob"_el},
            OptionErrorReason::UnexpectedValueType);
        assertError(
            makeOptionsWithTextListOption(),
            {"tool"_el, "--include"_el, "src"_el, "--include"_el, "test"_el, "--include"_el, "doc"_el},
            OptionErrorReason::SyntaxError);
        assertError(makeOptionsWithTextOption(), {"tool"_el, "file.txt"_el}, OptionErrorReason::SyntaxError);
        assertError(makeOptionsWithDuplicateNames(), {"tool"_el}, OptionErrorReason::SyntaxError);
    }

    void testRequiredAndNegativeValues() {
        auto options = Options::create();
        options->addOption("--required"_el).setType(OptionType::Text).setFlag(OptionFlag::Required);
        options->addOption("--count"_el).setType(OptionType::Integer);

        assertError(options, {"tool"_el}, OptionErrorReason::UnexpectedValueType);

        const auto result = parse(options, {"tool"_el, "--required"_el, "value"_el, "--count=-1"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText("--required"_el) == "value"_el);
        REQUIRE_EQUAL(result.values()->getInteger("--count"_el), OptionInteger{-1});
    }

    void testModuleOptionSuccess() {
        auto options = makeOptionsWithGlobalAndModuleOptions();

        const auto result = parse(options, {"tool"_el, "REMOVE"_el, "--verbose"_el, "--force"_el, "--count=3"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->moduleName() == "remove"_el);
        REQUIRE(result.values()->module() == options->optionModules().front());
        REQUIRE(result.values() != nullptr);
        REQUIRE(result.values()->getFlag("--verbose"_el));
        REQUIRE(result.values()->getFlag("--force"_el));
        REQUIRE_EQUAL(result.values()->getInteger("--count"_el), OptionInteger{3});
    }

    void testModuleHelpAndVersionStatus() {
        auto callbackCalled = false;
        auto options = makeOptionsWithGlobalAndModuleOptions();
        options->optionModules().front()->setPreParsingFn(
            [&callbackCalled](OptionModulePtr) -> void { callbackCalled = true; });

        auto result = parse(options, {"tool"_el, "--help"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
        REQUIRE(result.values()->moduleName().isEmpty());
        REQUIRE(result.values()->module() == nullptr);

        result = parse(options, {"tool"_el, "remove"_el, "--help"_el});
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
        auto result = parse(makeOptionsWithGlobalAndModuleOptions(), {"tool"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.values()->module() == nullptr);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::SyntaxError);

        assertError(makeOptionsWithGlobalAndModuleOptions(), {"tool"_el, "missing"_el}, OptionErrorReason::UnknownName);
        assertError(
            makeOptionsWithGlobalAndModuleOptions(), {"tool"_el, "--verbose"_el}, OptionErrorReason::SyntaxError);
        assertError(
            makeOptionsWithGlobalAndModuleOptions(), {"tool"_el, "--"_el, "remove"_el}, OptionErrorReason::SyntaxError);
        assertError(makeOptionsWithDuplicateModuleNames(), {"tool"_el, "remove"_el}, OptionErrorReason::SyntaxError);

        auto disabledGlobalOptions = makeOptionsWithDisabledGlobalOption();
        result = parse(disabledGlobalOptions, {"tool"_el, "remove"_el, "--verbose"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.values()->module() == disabledGlobalOptions->optionModules().front());
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::UnknownName);

        assertError(
            makeOptionsWithDisabledModuleOption(),
            {"tool"_el, "remove"_el, "--force"_el},
            OptionErrorReason::UnknownName);
        assertError(
            makeOptionsWithGlobalAndModuleOptions(),
            {"tool"_el, "remove"_el, "file.txt"_el},
            OptionErrorReason::SyntaxError);
    }

    void testModulePreCallbackCanEditGlobalOptions() {
        auto normalOptions = makeOptionsWithGlobalAndModuleOptions();
        auto normalResult = parse(normalOptions, {"tool"_el, "remove"_el, "-v"_el});
        REQUIRE(normalResult.status() == OptionResultStatus::Success);
        REQUIRE(normalResult.values()->getFlag("--verbose"_el));

        auto callbackCalled = false;
        auto specialOptions = makeOptionsWithGlobalAndModuleOptions("special"_el);
        specialOptions->optionModules().front()->setPreParsingFn(
            [specialOptions, &callbackCalled](OptionModulePtr) -> void {
                callbackCalled = true;
                specialOptions->editOption("--verbose"_el).setFlag(OptionFlag::Disabled);
            });

        assertError(specialOptions, {"tool"_el, "special"_el, "-v"_el}, OptionErrorReason::UnknownName);
        REQUIRE(callbackCalled);
    }

    void testModulePreCallbackErrorContext() {
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
        REQUIRE(result.errorContext()->moduleName() == "remove"_el);
        REQUIRE(result.errorContext()->argumentIndex().isNoIndex());
    }

    void testOptionSetPreCallbackCanEditOptions() {
        auto options = Options::create();
        options->addOption("--late"_el).setType(OptionType::Flag).setFlag(OptionFlag::Disabled);
        options->optionSets().front()->setPreParsingFn(
            [options](OptionSetPtr) -> void { options->editOption("--late"_el).clearFlag(OptionFlag::Disabled); });

        const auto result = parse(options, {"tool"_el, "--late"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getFlag("--late"_el));
    }

    void testCallbackOrder() {
        auto calls = std::vector<String>{};
        auto options = Options::create();
        options->addOption({"-v"_el, "--verbose"_el}).setType(OptionType::Flag);
        const auto rootSet = options->optionSets().front();
        rootSet->setPreParsingFn([&calls](OptionSetPtr) -> void { calls.emplace_back("root-pre"_el); });
        rootSet->setPostParsingFn([&calls](OptionValuesPtr) -> void { calls.emplace_back("root-post"_el); });

        const auto module = OptionModule::create("run"_el);
        module->setPreParsingFn([&calls](OptionModulePtr) -> void { calls.emplace_back("module-pre"_el); });
        module->setPostParsingFn([&calls](OptionValuesPtr) -> void { calls.emplace_back("module-post"_el); });
        module->addOption("--force"_el).setType(OptionType::Flag);
        const auto moduleSet = module->optionSets().front();
        moduleSet->setPreParsingFn([&calls](OptionSetPtr) -> void { calls.emplace_back("module-set-pre"_el); });
        moduleSet->setPostParsingFn([&calls](OptionValuesPtr) -> void { calls.emplace_back("module-set-post"_el); });
        options->addModule(module);

        const auto result = parse(options, {"tool"_el, "run"_el, "--verbose"_el, "--force"_el});
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
        options->addOption("--name"_el)
            .setType(OptionType::Text)
            .setValidateFn(
                [&parsedValidatorCalled, &parsedValidatorGotValue, &parsedValidatorGotValues, &parsedValidatorGotIndex](
                    OptionValuePtr value, OptionValuesPtr values) -> void {
                    parsedValidatorCalled = true;
                    parsedValidatorGotValue = value == values->value("--name"_el);
                    parsedValidatorGotValues =
                        values->getText("--name"_el) == "Ada"_el && values->getText("--mode"_el) == "auto"_el;
                    parsedValidatorGotIndex = value->argumentIndex() == ArgumentIndex{2U};
                });
        options->addOption({"-v"_el, "--verbose"_el})
            .setType(OptionType::Flag)
            .setValidateFn([&parsedValidatorGotFlagCount](OptionValuePtr value, OptionValuesPtr values) -> void {
                parsedValidatorGotFlagCount = value->flagCount() == ArgumentCount{2U} &&
                    values->getFlagCount("--verbose"_el) == ArgumentCount{2U};
            });
        options->addOption("--mode"_el)
            .setType(OptionType::Text)
            .setDefaultValue(String{"auto"_el})
            .setValidateFn(
                [&defaultValidatorCalled,
                    &defaultValidatorGotValue,
                    &defaultValidatorGotValues,
                    &defaultValidatorGotNoIndex](OptionValuePtr value, OptionValuesPtr values) -> void {
                    defaultValidatorCalled = true;
                    defaultValidatorGotValue = value == values->value("--mode"_el);
                    defaultValidatorGotValues = values->getText("--mode"_el) == "auto"_el;
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

        const auto result = parse(options, {"tool"_el, "--name"_el, "Ada"_el, "-vv"_el});
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
        auto postCalled = false;
        auto options = Options::create();
        auto module = OptionModule::create("run"_el);
        const auto editor = module->addOption("--name"_el)
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

        const auto result = parse(options, {"tool"_el, "run"_el, "--name"_el, "Ada"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::ValidationError);
        REQUIRE(result.errorContext()->moduleName() == "run"_el);
        REQUIRE(result.errorContext()->option() == option);
        REQUIRE(result.errorContext()->optionSet() == optionSet);
        REQUIRE(result.errorContext()->argumentIndex().isNoIndex());
        REQUIRE_FALSE(postCalled);
    }

    void testCallbackErrorDefaults() {
        auto options = Options::create();
        options->addOption("--name"_el).setType(OptionType::Text);
        const auto optionSet = options->optionSets().front();
        optionSet->setPreParsingFn([](OptionSetPtr) -> void { throw OptionError{OptionErrorContext{}}; });

        auto result = parse(options, {"tool"_el, "--name"_el, "Ada"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::None);
        REQUIRE(result.errorContext()->optionSet() == optionSet);

        options = Options::create();
        options->addOption("--name"_el).setType(OptionType::Text);
        const auto postOptionSet = options->optionSets().front();
        postOptionSet->setPostParsingFn([](OptionValuesPtr) -> void { throw OptionError{OptionErrorContext{}}; });

        result = parse(options, {"tool"_el, "--name"_el, "Ada"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == OptionErrorReason::ValidationError);
        REQUIRE(result.errorContext()->optionSet() == postOptionSet);
    }

    void testNoCallbacksForHelpAndNoPostAfterErrors() {
        auto callbackCalled = false;
        auto postCalled = false;
        auto validatorCalled = false;
        auto options = Options::create();
        options->addOption("--name"_el)
            .setType(OptionType::Text)
            .setValidateFn([&validatorCalled](OptionValuePtr, OptionValuesPtr) -> void { validatorCalled = true; });
        const auto optionSet = options->optionSets().front();
        optionSet->setPreParsingFn([&callbackCalled](OptionSetPtr) -> void { callbackCalled = true; });
        optionSet->setPostParsingFn([&postCalled](OptionValuesPtr) -> void { postCalled = true; });

        auto result = parse(options, {"tool"_el, "--help"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);
        REQUIRE_FALSE(callbackCalled);
        REQUIRE_FALSE(postCalled);
        REQUIRE_FALSE(validatorCalled);

        result = parse(options, {"tool"_el, "--name"_el});
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(callbackCalled);
        REQUIRE_FALSE(postCalled);
        REQUIRE_FALSE(validatorCalled);
    }

    void testParseOrThrowDisplaysValidationErrors() {
        auto options = Options::create();
        options->addOption("--name"_el)
            .setType(OptionType::Text)
            .setValidateFn([](OptionValuePtr, OptionValuesPtr) -> void {
                auto context = OptionErrorContext{};
                context.setDescription("Invalid name"_el);
                throw OptionError{context};
            });

        auto renderer = std::make_shared<ParserTestRenderer>();
        auto manager = OptionManager{options};
        manager.setRenderer(renderer);

        REQUIRE_THROWS_AS(OptionError, manager.parseOrThrow(makeArgs({"tool"_el, "--name"_el, "Ada"_el})));
        REQUIRE(renderer->errorDisplayed);
        REQUIRE(renderer->errorReason == OptionErrorReason::ValidationError);
    }

private:
    [[nodiscard]] static auto makeArgs(std::initializer_list<StringView> args) -> CommandLineArguments {
        auto result = CommandLineArguments{};
        result.reserve(args.size());
        for (const auto &arg : args) {
            result.push_back(StringConverter{arg}.toU8String());
        }
        return result;
    }

    [[nodiscard]] static auto parse(const OptionsPtr &options, std::initializer_list<StringView> args) -> OptionResult {
        auto manager = OptionManager{options};
        return manager.parse(makeArgs(args));
    }

    void assertError(
        const OptionsPtr &options, std::initializer_list<StringView> args, const OptionErrorReason reason) {
        const auto result = parse(options, args);
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        REQUIRE(result.errorContext()->reason() == reason);
    }

    void requireArgumentIndexes(const OptionValuePtr &value, std::initializer_list<ArgumentIndex> indexes) {
        REQUIRE(value != nullptr);
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
        options->addOption("--name"_el).setType(OptionType::Text);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithIntegerOption() -> OptionsPtr {
        auto options = Options::create();
        options->addOption("--count"_el).setType(OptionType::Integer);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithChoiceOption() -> OptionsPtr {
        auto options = Options::create();
        options->addOption("--mode"_el)
            .setType(OptionType::Choice)
            .setChoices(OptionChoices::create({"fast"_el, "safe"_el}));
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithTextListOption() -> OptionsPtr {
        auto options = Options::create();
        options->addOption("--include"_el).setType(OptionType::Text).setMaximum(ArgumentCount{2U});
        return options;
    }

    [[nodiscard]] static auto makeDisabledOptions() -> OptionsPtr {
        auto options = Options::create();
        options->addOption("--off"_el).setType(OptionType::Flag).setFlag(OptionFlag::Disabled);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithDuplicateNames() -> OptionsPtr {
        auto options = Options::create();
        options->addOption("--path"_el).setType(OptionType::Text);
        options->addOption("--PATH"_el).setType(OptionType::Text);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithGlobalAndModuleOptions(StringView moduleName = "remove"_el) -> OptionsPtr {
        auto options = Options::create();
        options->addOption({"-v"_el, "--verbose"_el}).setType(OptionType::Flag);
        auto module = OptionModule::create(moduleName);
        module->addOption("--force"_el).setType(OptionType::Flag);
        module->addOption("--count"_el).setType(OptionType::Integer);
        options->addModule(module);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithDuplicateModuleNames() -> OptionsPtr {
        auto options = Options::create();
        options->addOption("--force"_el).setType(OptionType::Flag);
        auto module = OptionModule::create("remove"_el);
        module->addOption("--FORCE"_el).setType(OptionType::Flag);
        options->addModule(module);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithDisabledGlobalOption() -> OptionsPtr {
        auto options = makeOptionsWithGlobalAndModuleOptions();
        options->editOption("--verbose"_el).setFlag(OptionFlag::Disabled);
        return options;
    }

    [[nodiscard]] static auto makeOptionsWithDisabledModuleOption() -> OptionsPtr {
        auto options = makeOptionsWithGlobalAndModuleOptions();
        options->optionModules().front()->editOption("--force"_el).setFlag(OptionFlag::Disabled);
        return options;
    }
};
