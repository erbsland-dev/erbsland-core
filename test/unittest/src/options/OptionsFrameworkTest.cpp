// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/CommandLineArguments.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Option.hpp>
#include <erbsland/OptionChoice.hpp>
#include <erbsland/OptionChoiceEditor.hpp>
#include <erbsland/OptionChoices.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/OptionModule.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionSet.hpp>
#include <erbsland/OptionValue.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <vector>

using el::core::CommandLineArguments;
using el::options::OptionError;
using el::text::String;
using el::text::StringEditor;
using el::unit::ArgumentCount;
using el::unit::ArgumentIndex;
using el::unit::ExitCode;
using el::unit::ItemIndex;
using namespace el::options;
using namespace el::text::literals;

TESTED_TARGETS(
    Application Option OptionChoice OptionChoiceEditor OptionChoices OptionEditor OptionManager OptionModule
        OptionResult OptionSet OptionValue OptionValues Options)
class OptionsFrameworkTest final : public el::UnitTest {
private:
    static constexpr auto cCountOption = "--count"_el;
    static constexpr auto cEnabledOption = "--enabled"_el;
    static constexpr auto cVerboseOption = "--verbose"_el;
    static constexpr auto cAOption = "-A"_el;
    static constexpr auto cVeryVerboseShortOption = "-V"_el;
    static constexpr auto cVerboseShortOption = "-v"_el;
    static constexpr auto cCountsOption = "counts"_el;
    static constexpr auto cIndexedOption = "indexed"_el;
    static constexpr auto cMissingOption = "missing"_el;
    static constexpr auto cNameOption = "name"_el;
    static constexpr auto cNamesOption = "names"_el;
    static constexpr auto cOriginalOption = "original"_el;

public:
    void testCanonicalNamesAndDeveloperLookups() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        const auto option = options->addOption({cAOption, "--One_Two_3"_el, "One_Two_3"_el}).option();
        REQUIRE_EQUAL(option->names().getRefOrThrow(el::ItemIndex{0}), cAOption);
        REQUIRE_EQUAL(option->names().getRefOrThrow(el::ItemIndex{1}), "--one-two-3"_el);
        REQUIRE_EQUAL(option->names().getRefOrThrow(el::ItemIndex{2}), "one-two-3"_el);
        REQUIRE(option->hasLongName("--ONE_TWO_3"_el));
        REQUIRE(option->hasPositionalName("ONE_TWO_3"_el));
        REQUIRE(option->hasShortName(U'A'));
        REQUIRE_FALSE(option->hasShortName(U'a'));
        REQUIRE(options->editOption("--ONE_TWO_3"_el).option() == option);
        REQUIRE(options->editOption("ONE_TWO_3"_el).option() == option);
        REQUIRE(option->hasConflictingOptionName(*Option::create({"--one-two-3"_el})));
        auto module = OptionModule::create("Run_One_2"_el);
        REQUIRE_EQUAL(module->name(), "run-one-2"_el);
        REQUIRE(module->hasName("RUN_ONE_2"_el));
        auto values = OptionValues::create();
        const auto value = OptionValue::create(OptionInteger{42});
        values->setValue({cAOption, "--ONE_TWO_3"_el, "One_Two_3"_el}, value);
        REQUIRE(values->value("--one-two-3"_el) == value);
        REQUIRE(values->value("ONE_TWO_3"_el) == value);
        REQUIRE(values->value(cAOption) == value);
        REQUIRE_FALSE(values->value("-a"_el));
        REQUIRE_EQUAL(values->valueCount("--ONE_TWO_3"_el), ArgumentCount::one());
        values->setModuleName("Run_One_2"_el);
        REQUIRE_EQUAL(values->moduleName(), "run-one-2"_el);
        values->setModuleName({});
        REQUIRE(values->moduleName().isEmpty());
    }

    void testInvalidNamesFailImmediatelyAndPreserveState() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto option = Option::create({"--original"_el});
        const auto module = OptionModule::create(cOriginalOption);
        auto values = OptionValues::create();
        const auto originalValue = OptionValue::create(OptionInteger{42});
        values->setValue(cOriginalOption, originalValue);
        for (
            const auto &name : el::text::StringList{
                ""_el,
                "-"_el,
                "--"_el,
                "_a"_el,
                "__name"_el,
                "-_"_el,
                "-_a"_el,
                "--one__two"_el,
                "--one_-two"_el,
                "--one-"_el,
                "--1name"_el,
                "one--two"_el,
                "one_"_el,
                "a/b"_el,
                "über"_el}) {
            runWithContext(SOURCE_LOCATION(), [&]() -> void {
                REQUIRE_THROWS_AS(el::err::ParameterFailure, Option::create({name}));
                REQUIRE_THROWS_AS(el::err::ParameterFailure, option->addName(name));
                REQUIRE_THROWS_AS(el::err::ParameterFailure, option->setNames({"--replacement"_el, name}));
                REQUIRE_EQUAL(option->names().count().toSizeT(), 1U);
                REQUIRE_EQUAL(option->names().first(), "--original"_el);
                REQUIRE_EQUAL(option->type(), OptionType::Flag);
                REQUIRE_THROWS_AS(el::err::ParameterFailure, OptionModule::create(name));
                REQUIRE_THROWS_AS(el::err::ParameterFailure, module->setName(name));
                REQUIRE_EQUAL(module->name(), cOriginalOption);
                REQUIRE_THROWS_AS(el::err::ParameterFailure, values->setValue(name, originalValue));
                REQUIRE_THROWS_AS(
                    el::err::ParameterFailure,
                    values->setValue({cOriginalOption, name}, OptionValue::create(OptionInteger{99})));
                REQUIRE(values->value(cOriginalOption) == originalValue);
                REQUIRE_FALSE(values->value(name));
            });
        }
        REQUIRE_THROWS_AS(el::err::ParameterFailure, module->setName("--valid-option"_el));
        values->setModuleName(cOriginalOption);
        REQUIRE_THROWS_AS(el::err::ParameterFailure, values->setModuleName("--invalid-module"_el));
        REQUIRE_EQUAL(values->moduleName(), cOriginalOption);
        option->setNames({});
        REQUIRE(option->names().isEmpty());
        REQUIRE(Option::create()->names().isEmpty());
        REQUIRE(OptionModule::create()->name().isEmpty());
    }

    void testNameGrammarAndLengthBoundaries() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto maximum = String{StringEditor{}.append(U'a', el::unit::CpCount{100U})};
        const auto excessive = el::join(maximum, "a"_el);
        REQUIRE(OptionModule::isValidName(maximum));
        REQUIRE(Option::isValidPositionalName(maximum));
        REQUIRE(Option::isValidLongName(el::join("--"_el, maximum)));
        REQUIRE_FALSE(OptionModule::isValidName(excessive));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, Option::create({excessive}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, OptionModule::create(excessive));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, Option::create({el::join("--"_el, excessive)}));
        for (const auto &name : el::text::StringList{"a"_el, "a1"_el, "a-1"_el, "One_Two_3"_el}) {
            REQUIRE(Option::isValidPositionalName(name));
            REQUIRE(OptionModule::isValidName(name));
        }
        REQUIRE(Option::isValidShortName(cAOption));
        REQUIRE(Option::isValidShortName("-1"_el));
        REQUIRE_FALSE(Option::isValidShortName("-_"_el));
        REQUIRE_FALSE(Option::isValidLongName("__name"_el));
        REQUIRE_FALSE(Option::isValidLongName("--one__two"_el));
        REQUIRE_FALSE(Option::isValidPositionalName("one-_two"_el));
    }

    void testOptionEditing() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();

        auto editor = options->addOption({cVerboseShortOption, "--Verbose"_el, "Verbose"_el})
                          .setHelp("Enable verbose output"_el)
                          .setHelpTitle("Verbosity"_el)
                          .setHelpDescription("Enable verbose output"_el)
                          .setHelpEpilog("Use twice for trace output."_el)
                          .setHelpVisibility(OptionHelpVisibility::Usage)
                          .setValueName("level"_el)
                          .setType(OptionType::Flag)
                          .setFlag(OptionFlag::Required)
                          .setMaximum(ArgumentCount{3U});

        REQUIRE(editor.isValid());
        REQUIRE(options->builtInOptionSet());
        REQUIRE_EQUAL(options->builtInOptionSet()->options().size(), 2U);
        REQUIRE_EQUAL(options->optionSets().size(), 1U);

        const auto option = editor.option();
        REQUIRE(option);
        REQUIRE_EQUAL(option->names().count().toSizeT(), 3U);
        REQUIRE(option->names().getRefOrThrow(el::ItemIndex{0}) == cVerboseShortOption);
        REQUIRE(option->names().getRefOrThrow(el::ItemIndex{1}) == cVerboseOption);
        REQUIRE(option->names().getRefOrThrow(el::ItemIndex{2}) == "verbose"_el);
        REQUIRE(option->help().description() == "Enable verbose output"_el);
        REQUIRE(option->help().title() == "Verbosity"_el);
        REQUIRE(option->help().epilog() == "Use twice for trace output."_el);
        REQUIRE(option->help().visibility() == OptionHelpVisibility::Usage);
        REQUIRE(option->valueName() == "level"_el);
        option->setValueName("trace-level"_el);
        REQUIRE(option->valueName() == "trace-level"_el);
        options->editOption(cVerboseOption).setValueName({});
        REQUIRE(option->valueName().isEmpty());
        REQUIRE(option->type() == OptionType::Flag);
        REQUIRE(option->type().toString() == "flag"_el);
        REQUIRE(option->flags().isSet(OptionFlag::Required));
        REQUIRE(option->maximum() == ArgumentCount{3U});
        REQUIRE(Option::isValidShortName(cVerboseShortOption));
        REQUIRE(Option::isValidLongName(cVerboseOption));
        REQUIRE(Option::isValidOptionName(cVerboseOption));
        REQUIRE_FALSE(Option::isValidLongName("--1invalid"_el));
        REQUIRE(Option::isValidPositionalName("verbose"_el));
        REQUIRE_FALSE(Option::isValidPositionalName("1invalid"_el));
        REQUIRE(Option::isPositionalName("verbose"_el));
        REQUIRE(option->hasOptionName());
        REQUIRE(option->isRegularOption());
        REQUIRE_FALSE(option->isPositionalArgument());
        REQUIRE(option->hasShortName(U'v'));
        REQUIRE(option->hasLongName("--VERBOSE"_el));
        REQUIRE(option->hasPositionalName());
        REQUIRE(option->hasPositionalName("VERBOSE"_el));
        REQUIRE(option->hasValidOptionNames());
        REQUIRE(option->hasConflictingOptionName(*Option::create({"--VERBOSE"_el})));
        REQUIRE(option->hasConflictingOptionName(*Option::create({"VERBOSE"_el})));

        const auto positionalOption = Option::create({"Path"_el});
        REQUIRE_FALSE(positionalOption->hasOptionName());
        REQUIRE_FALSE(positionalOption->isRegularOption());
        REQUIRE(positionalOption->isPositionalArgument());
        REQUIRE(positionalOption->type() == OptionType::Text);
        REQUIRE(positionalOption->names().getRefOrThrow(el::ItemIndex{0}) == "path"_el);

        options->editOption(cVerboseOption).setFlag(OptionFlag::Disabled).setFlag(OptionFlag::Greedy);
        REQUIRE(option->flags().isSet(OptionFlag::Disabled));
        REQUIRE(option->flags().isSet(OptionFlag::Greedy));
        REQUIRE(option->isDisabled());

        options->editOption(cVerboseOption).setDefaultValue(true);
        REQUIRE(option->hasDefaultValue());
        options->editOption(cVerboseOption).clearDefaultValue();
        REQUIRE_FALSE(option->hasDefaultValue());

        auto validatorCalled = false;
        options->editOption(cVerboseOption).setValidateFn([&validatorCalled](OptionValuePtr, OptionValuesPtr) -> void {
            validatorCalled = true;
        });
        REQUIRE(option->validateFn());
        option->validateFn()(OptionValue::create(true), OptionValues::create());
        REQUIRE(validatorCalled);
        REQUIRE(OptionErrorReason::ValidationError != OptionErrorReason::None);
    }

    void testChoices() {
        auto applicationScope = ApplicationTestScope<>{};
        auto invalidEditor = OptionChoiceEditor{};
        REQUIRE_FALSE(invalidEditor.isValid());
        REQUIRE_FALSE(invalidEditor.choice());
        invalidEditor.setHelp("Ignored"_el)
            .setHelp(OptionHelp{"Ignored"_el})
            .setHelpTitle("Ignored"_el)
            .setHelpDescription("Ignored"_el)
            .setHelpEpilog("Ignored"_el)
            .setHelpExample("Ignored"_el)
            .setHelpVisibility(OptionHelpVisibility::Hidden);

        auto choices = OptionChoices::create();
        auto fastEditor = choices->addChoice("fast"_el);
        REQUIRE(fastEditor.isValid());
        REQUIRE(fastEditor.choice() == choices->choices().at(0));
        fastEditor.setHelp("Prefer speed"_el);
        REQUIRE(fastEditor.choice()->help().title() == "Prefer speed"_el);
        fastEditor.setHelpTitle("Performance"_el)
            .setHelpDescription("Fast mode"_el)
            .setHelpEpilog("Use for quick runs"_el)
            .setHelpExample("--mode fast"_el)
            .setHelpVisibility(OptionHelpVisibility::Overview);

        auto choiceHelp = OptionHelp{"Safer mode"_el};
        choiceHelp.setVisibility(OptionHelpVisibility::Normal);
        choiceHelp.setTitle("Safety"_el);
        choiceHelp.setDescription("Safer mode"_el);
        choiceHelp.setEpilog("Use for protected runs"_el);
        auto safeEditor = choices->addChoice(OptionChoice::create("SafeMode"_el));
        safeEditor.setHelp(choiceHelp);
        choices->addChoice(u8"Ä"_el);

        REQUIRE_EQUAL(choices->choiceCount(), ArgumentCount{3U});
        REQUIRE(choices->choices().at(0)->text() == "fast"_el);
        REQUIRE(choices->choices().at(0)->help().title() == "Performance"_el);
        REQUIRE(choices->choices().at(0)->help().description() == "Fast mode"_el);
        REQUIRE(choices->choices().at(0)->help().epilog() == "Use for quick runs"_el);
        REQUIRE(choices->choices().at(0)->help().example() == "--mode fast"_el);
        REQUIRE(choices->choices().at(0)->help().visibility() == OptionHelpVisibility::Overview);
        REQUIRE(choices->choices().at(1)->text() == "SafeMode"_el);
        REQUIRE(choices->choices().at(1)->help().description() == "Safer mode"_el);
        REQUIRE(choices->choices().at(1)->help().visibility() == OptionHelpVisibility::Normal);
        REQUIRE(choices->choices().at(1)->help().title() == "Safety"_el);
        REQUIRE(choices->choices().at(1)->help().epilog() == "Use for protected runs"_el);

        auto options = Options::create();
        auto editor =
            options->addOption("--mode"_el).setType(OptionType::Choice).setChoices(choices).addChoice("quiet"_el);

        REQUIRE(editor.option()->choices() == choices);
        REQUIRE_EQUAL(editor.option()->choices()->choiceCount(), ArgumentCount{4U});
        const auto choiceText = editor.option()->matchingChoiceText("safemode"_el);
        REQUIRE(choiceText.has_value());
        REQUIRE(choiceText.value() == "SafeMode"_el);
        REQUIRE_FALSE(editor.option()->matchingChoiceText(u8"ä"_el).has_value());
        REQUIRE_FALSE(editor.option()->matchingChoiceText(cMissingOption).has_value());
    }

    void testSetsAndModules() {
        auto applicationScope = ApplicationTestScope<>{};
        auto preSetCalled = false;
        auto postSetCalled = false;
        auto preModuleCalled = false;
        auto postModuleCalled = false;
        auto moduleMainCalled = false;

        auto optionSet = OptionSet::create();
        optionSet->setHelpTitle("Input"_el);
        optionSet->setHelpDescription("Input options."_el);
        optionSet->setHelpEpilog("Input epilog."_el);
        optionSet->setHelpVisibility(OptionHelpVisibility::Overview);
        optionSet->setPreParsingFn([&preSetCalled](OptionSetPtr set) -> void { preSetCalled = set != nullptr; });
        optionSet->setPostParsingFn(
            [&postSetCalled](OptionValuesPtr values) -> void { postSetCalled = values != nullptr; });
        optionSet->addOption("path"_el).setType(OptionType::Text);

        auto module = OptionModule::create("Run"_el);
        module->setHelpTitle("Run module"_el);
        module->setHelpDescription("Run one action."_el);
        module->setHelpEpilog("Run epilog."_el);
        module->setHelpVisibility(OptionHelpVisibility::Overview);
        REQUIRE(module->name() == "run"_el);
        REQUIRE(OptionModule::isValidName("run"_el));
        REQUIRE_FALSE(OptionModule::isValidName("1remove"_el));
        REQUIRE(module->hasName("RUN"_el));
        module->setPreParsingFn(
            [&preModuleCalled](OptionModulePtr optionModule) -> void { preModuleCalled = optionModule != nullptr; });
        module->setPostParsingFn(
            [&postModuleCalled](OptionValuesPtr values) -> void { postModuleCalled = values != nullptr; });
        module->setMainFn([&moduleMainCalled](OptionValuesPtr values) -> ExitCode {
            moduleMainCalled = values != nullptr;
            return ExitCode{7};
        });
        module->addOption("--force"_el).setType(OptionType::Flag);

        auto options = Options::create();
        options->setHelpTitle("Root"_el);
        options->setHelpDescription("Root help."_el);
        options->setHelpEpilog("Root epilog."_el);
        options->setHelpVisibility(OptionHelpVisibility::Normal);
        options->addSet(optionSet);
        options->addModule(module);
        optionSet->setFlags(OptionFlag::Required);

        REQUIRE(options->builtInOptionSet());
        REQUIRE_EQUAL(options->optionSets().size(), 1U);
        REQUIRE_EQUAL(options->optionModules().size(), 1U);
        REQUIRE_EQUAL(module->optionSets().size(), 1U);
        REQUIRE(optionSet->flags().isSet(OptionFlag::Required));
        REQUIRE(optionSet->help().title() == "Input"_el);
        REQUIRE(optionSet->help().description() == "Input options."_el);
        REQUIRE(optionSet->help().epilog() == "Input epilog."_el);
        REQUIRE(optionSet->help().visibility() == OptionHelpVisibility::Overview);
        REQUIRE(module->help().title() == "Run module"_el);
        REQUIRE(module->help().description() == "Run one action."_el);
        REQUIRE(module->help().epilog() == "Run epilog."_el);
        REQUIRE(module->help().visibility() == OptionHelpVisibility::Overview);
        REQUIRE(options->help().title() == "Root"_el);
        REQUIRE(options->help().description() == "Root help."_el);
        REQUIRE(options->help().epilog() == "Root epilog."_el);
        REQUIRE(options->help().visibility() == OptionHelpVisibility::Normal);
        REQUIRE(module->editOption("--force"_el).isValid());
        REQUIRE(options->editOption("path"_el).isValid());

        const auto values = OptionValues::create();
        optionSet->preParsingFn()(optionSet);
        optionSet->postParsingFn()(values);
        module->preParsingFn()(module);
        module->postParsingFn()(values);
        REQUIRE_EQUAL(module->mainFn()(values), 7);

        REQUIRE(preSetCalled);
        REQUIRE(postSetCalled);
        REQUIRE(preModuleCalled);
        REQUIRE(postModuleCalled);
        REQUIRE(moduleMainCalled);
    }

    void testValuesAndTypedGetters() {
        auto applicationScope = ApplicationTestScope<>{};
        auto values = OptionValues::create();
        auto sourceOption = Option::create({cVerboseShortOption, cVerboseOption});
        values->setValue({cVerboseShortOption, cVerboseOption}, OptionValue::create(sourceOption, std::monostate{}));
        values->setValue(cEnabledOption, OptionValue::create(true));
        values->setValue(cCountOption, OptionValue::create(OptionInteger{42}));
        values->setValue(cNameOption, OptionValue::create(StringEditor{"Ada"_el}));
        values->setValue(cNamesOption, OptionValue::create(el::text::StringList{"Ada"_el, "Bjarne"_el}));
        values->setValue(
            cCountsOption, OptionValue::create(std::vector<OptionInteger>{OptionInteger{1}, OptionInteger{2}}));
        auto indexedValue = OptionValue::create(
            StringEditor{"Indexed"_el}, std::vector<ArgumentIndex>{ArgumentIndex{3U}, ArgumentIndex{5U}});
        auto indexedSourceValue =
            OptionValue::create(sourceOption, std::monostate{}, std::vector<ArgumentIndex>{ArgumentIndex{7U}});
        auto countedFlagValue = OptionValue::create(
            sourceOption, std::monostate{}, std::vector<ArgumentIndex>{ArgumentIndex{2U}, ArgumentIndex{4U}});
        auto mutableIndexesValue = OptionValue::create(StringEditor{"Mutable"_el});
        mutableIndexesValue->setArgumentIndexes(std::vector<ArgumentIndex>{ArgumentIndex{11U}});
        auto mutableFlagIndexesValue = OptionValue::create(std::monostate{});
        mutableFlagIndexesValue->setArgumentIndexes(std::vector<ArgumentIndex>{ArgumentIndex{13U}, ArgumentIndex{17U}});
        values->setValue({cIndexedOption, "INDEXED"_el}, indexedValue);
        values->setValue({cVeryVerboseShortOption, "--very-verbose"_el}, countedFlagValue);

        REQUIRE(values->getFlag(cVerboseOption));
        REQUIRE(values->value(cVerboseOption)->option().lock() == sourceOption);
        REQUIRE(values->value(cVerboseOption)->type() == OptionValueType::Flag);
        REQUIRE(values->value(cVerboseOption)->getFlag());
        REQUIRE(values->value(cEnabledOption)->type() == OptionValueType::Boolean);
        REQUIRE(values->getBoolean(cEnabledOption));
        REQUIRE(values->value(cCountOption)->type() == OptionValueType::Integer);
        REQUIRE_EQUAL(values->value(cCountOption)->getInteger(), OptionInteger{42});
        REQUIRE(values->value(cCountOption)->getFlag(true));
        REQUIRE(values->value(cNameOption)->type() == OptionValueType::Text);
        REQUIRE(values->value(cNameOption)->getText() == "Ada"_el);
        REQUIRE(values->value(cNamesOption)->type() == OptionValueType::TextList);
        REQUIRE(values->value(cCountsOption)->type() == OptionValueType::IntegerList);
        REQUIRE_EQUAL(values->valueCount(cVerboseOption), ArgumentCount::one());
        REQUIRE_EQUAL(values->getFlagCount(cVerboseOption), ArgumentCount::zero());
        REQUIRE_EQUAL(values->getFlagCount("--very-verbose"_el), ArgumentCount{2U});
        REQUIRE_EQUAL(values->getFlagCount(cVeryVerboseShortOption), ArgumentCount{2U});
        REQUIRE_EQUAL(values->getFlagCount(cMissingOption), ArgumentCount::zero());
        REQUIRE_EQUAL(values->getFlagCount(cMissingOption, ArgumentCount{9U}), ArgumentCount{9U});
        REQUIRE_EQUAL(values->getFlagCount(cNameOption), ArgumentCount::zero());
        REQUIRE_EQUAL(values->getInteger(cCountOption), OptionInteger{42});
        REQUIRE(values->getText(cNameOption) == "Ada"_el);
        REQUIRE(values->getText(cMissingOption, "fallback"_el) == "fallback"_el);

        const auto textList = values->getTextList(cNamesOption);
        REQUIRE_EQUAL(textList.count(), el::unit::ItemCount{2U});
        REQUIRE(textList.get(el::unit::ItemIndex::zero()) == "Ada"_el);
        REQUIRE(textList.get(el::unit::ItemIndex{1U}) == "Bjarne"_el);
        const auto singleTextList = values->value(cNameOption)->getTextList();
        REQUIRE_EQUAL(singleTextList.count(), el::unit::ItemCount::one());
        REQUIRE(singleTextList.get(el::unit::ItemIndex::zero()) == "Ada"_el);

        const auto integerList = values->getIntegerList(cCountsOption);
        REQUIRE_EQUAL(integerList.size(), 2U);
        REQUIRE_EQUAL(integerList.at(0), OptionInteger{1});
        REQUIRE_EQUAL(integerList.at(1), OptionInteger{2});
        const auto singleIntegerList = values->value(cCountOption)->getIntegerList();
        REQUIRE_EQUAL(singleIntegerList.size(), 1U);
        REQUIRE_EQUAL(singleIntegerList.at(0), OptionInteger{42});

        REQUIRE(OptionValue::create(true)->argumentIndex().isNoIndex());
        REQUIRE_EQUAL(OptionValue::create(true)->flagCount(), ArgumentCount::zero());
        REQUIRE(indexedValue->argumentIndex() == ArgumentIndex{3U});
        REQUIRE_EQUAL(indexedValue->argumentIndexes().size(), 2U);
        REQUIRE(indexedValue->argumentIndexes().at(1) == ArgumentIndex{5U});
        REQUIRE_EQUAL(indexedValue->flagCount(), ArgumentCount::zero());
        REQUIRE(indexedSourceValue->option().lock() == sourceOption);
        REQUIRE(indexedSourceValue->argumentIndex() == ArgumentIndex{7U});
        REQUIRE_EQUAL(indexedSourceValue->flagCount(), ArgumentCount::one());
        REQUIRE_EQUAL(countedFlagValue->flagCount(), ArgumentCount{2U});
        REQUIRE_EQUAL(mutableFlagIndexesValue->flagCount(), ArgumentCount{2U});
        REQUIRE(mutableIndexesValue->argumentIndex() == ArgumentIndex{11U});
        REQUIRE_EQUAL(mutableIndexesValue->flagCount(), ArgumentCount::zero());
        REQUIRE(values->value(cIndexedOption) == values->value("INDEXED"_el));
        REQUIRE(values->value(cIndexedOption)->argumentIndexes().at(0) == ArgumentIndex{3U});
    }

    void testManagerPlaceholderAndDocuments() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        auto manager = OptionManager{options};

        auto args = CommandLineArguments{StringEditor{"tool"_el}};
        const auto result = manager.parse(args);

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values());

        auto module = OptionModule::create("remove"_el);
        result.values()->setModuleName("remove"_el);
        result.values()->setModule(module);
        REQUIRE(result.values()->moduleName() == "remove"_el);
        REQUIRE(result.values()->module() == module);

        REQUIRE_FALSE(manager.helpDocument({}).isEmpty());
        REQUIRE_FALSE(manager.versionDocument({}).isEmpty());

        auto errorArgs = CommandLineArguments{StringEditor{"tool"_el}, StringEditor{"--unknown"_el}};
        const auto errorResult = manager.parse(errorArgs);
        REQUIRE(errorResult.status() == OptionResultStatus::Error);
        REQUIRE(errorResult.errorContext().has_value());
        REQUIRE(errorResult.errorContext()->reason() == OptionErrorReason::UnknownName);
        REQUIRE_FALSE(manager.errorDocument(errorResult.errorContext().value()).isEmpty());
    }
};
