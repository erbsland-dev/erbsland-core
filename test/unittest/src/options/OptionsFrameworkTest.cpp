// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/CommandLineArguments.hpp>
#include <erbsland/options/Option.hpp>
#include <erbsland/options/OptionChoice.hpp>
#include <erbsland/options/OptionChoices.hpp>
#include <erbsland/options/OptionError.hpp>
#include <erbsland/options/OptionManager.hpp>
#include <erbsland/options/OptionModule.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionSet.hpp>
#include <erbsland/options/OptionValue.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <vector>

using el::core::CommandLineArguments;
using el::options::OptionError;
using el::text::String;
using el::text::StringView;
using el::unit::ArgumentCount;
using el::unit::ArgumentIndex;
using el::unit::ElementIndex;
using el::unit::ExitCode;
using namespace el::options;

using namespace el::text::literals;

TESTED_TARGETS(
    Application Option OptionChoice OptionChoices OptionEditor OptionManager OptionModule OptionResult OptionSet
        OptionValue OptionValues Options)
class OptionsFrameworkTest final : public el::UnitTest {
public:
    void testOptionEditing() {
        auto options = Options::create();

        auto editor = options->addOption({"-v"_el, "--Verbose"_el, "Verbose"_el})
                          .setHelp("Enable verbose output"_el)
                          .setHelpTitle("Verbosity"_el)
                          .setHelpEpilog("Use twice for trace output."_el)
                          .setHelpVisibility(OptionHelpVisibility::Usage)
                          .setValueName("level"_el)
                          .setType(OptionType::Flag)
                          .setFlag(OptionFlag::Required)
                          .setMaximum(ArgumentCount{3U});

        REQUIRE(editor.isValid());
        REQUIRE(options->builtInOptionSet() != nullptr);
        REQUIRE_EQUAL(options->builtInOptionSet()->options().size(), 2U);
        REQUIRE_EQUAL(options->optionSets().size(), 1U);

        const auto option = editor.option();
        REQUIRE(option != nullptr);
        REQUIRE_EQUAL(option->names().size(), 3U);
        REQUIRE(option->names().at(0) == "-v"_el);
        REQUIRE(option->names().at(1) == "--verbose"_el);
        REQUIRE(option->names().at(2) == "verbose"_el);
        REQUIRE(option->help().description() == "Enable verbose output"_el);
        REQUIRE(option->help().title() == "Verbosity"_el);
        REQUIRE(option->help().epilog() == "Use twice for trace output."_el);
        REQUIRE(option->help().visibility() == OptionHelpVisibility::Usage);
        REQUIRE(option->valueName() == "level"_el);
        option->setValueName("trace-level"_el);
        REQUIRE(option->valueName() == "trace-level"_el);
        options->editOption("--verbose"_el).setValueName({});
        REQUIRE(option->valueName().isEmpty());
        REQUIRE(option->type() == OptionType::Flag);
        REQUIRE(option->type().toString() == "flag"_el);
        REQUIRE(option->flags().isSet(OptionFlag::Required));
        REQUIRE(option->maximum() == ArgumentCount{3U});
        REQUIRE(Option::isValidShortName("-v"_el));
        REQUIRE(Option::isValidLongName("--verbose"_el));
        REQUIRE(Option::isValidOptionName("--verbose"_el));
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
        REQUIRE(positionalOption->names().at(0) == "path"_el);

        options->editOption("--verbose"_el).setFlag(OptionFlag::Disabled).setFlag(OptionFlag::Greedy);
        REQUIRE(option->flags().isSet(OptionFlag::Disabled));
        REQUIRE(option->flags().isSet(OptionFlag::Greedy));
        REQUIRE(option->isDisabled());

        options->editOption("--verbose"_el).setDefaultValue(true);
        REQUIRE(option->hasDefaultValue());
        options->editOption("--verbose"_el).clearDefaultValue();
        REQUIRE_FALSE(option->hasDefaultValue());

        auto validatorCalled = false;
        options->editOption("--verbose"_el).setValidateFn([&validatorCalled](OptionValuePtr, OptionValuesPtr) -> void {
            validatorCalled = true;
        });
        REQUIRE(option->validateFn());
        option->validateFn()(OptionValue::create(true), OptionValues::create());
        REQUIRE(validatorCalled);
        REQUIRE(OptionErrorReason::ValidationError != OptionErrorReason::None);
    }

    void testChoices() {
        auto choices = OptionChoices::create();
        auto choiceHelp = OptionHelp{"Safer mode"_el};
        choiceHelp.setVisibility(OptionHelpVisibility::Normal);
        choiceHelp.setTitle("Safety"_el);
        choiceHelp.setEpilog("Use for protected runs"_el);
        choices->addChoice("fast"_el).addChoice(OptionChoice::create("SafeMode"_el, choiceHelp)).addChoice(u8"Ä"_el);

        REQUIRE_EQUAL(choices->choiceCount(), ArgumentCount{3U});
        REQUIRE(choices->choices().at(0)->text() == "fast"_el);
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
        REQUIRE_FALSE(editor.option()->matchingChoiceText("missing"_el).has_value());
    }

    void testSetsAndModules() {
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

        REQUIRE(options->builtInOptionSet() != nullptr);
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
        auto values = OptionValues::create();
        auto sourceOption = Option::create({"-v"_el, "--verbose"_el});
        values->setValue({"-v"_el, "--verbose"_el}, OptionValue::create(sourceOption, true));
        values->setValue("--count"_el, OptionValue::create(OptionInteger{42}));
        values->setValue("name"_el, OptionValue::create(String{"Ada"_el}));
        values->setValue("names"_el, OptionValue::create(std::vector<String>{String{"Ada"_el}, String{"Bjarne"_el}}));
        values->setValue(
            "counts"_el, OptionValue::create(std::vector<OptionInteger>{OptionInteger{1}, OptionInteger{2}}));
        auto indexedValue =
            OptionValue::create(String{"Indexed"_el}, std::vector<ArgumentIndex>{ArgumentIndex{3U}, ArgumentIndex{5U}});
        auto indexedSourceValue =
            OptionValue::create(sourceOption, true, std::vector<ArgumentIndex>{ArgumentIndex{7U}});
        auto countedFlagValue =
            OptionValue::create(sourceOption, true, std::vector<ArgumentIndex>{ArgumentIndex{2U}, ArgumentIndex{4U}});
        auto mutableIndexesValue = OptionValue::create(String{"Mutable"_el});
        mutableIndexesValue->setArgumentIndexes(std::vector<ArgumentIndex>{ArgumentIndex{11U}});
        auto mutableFlagIndexesValue = OptionValue::create(true);
        mutableFlagIndexesValue->setArgumentIndexes(std::vector<ArgumentIndex>{ArgumentIndex{13U}, ArgumentIndex{17U}});
        values->setValue({"indexed"_el, "INDEXED"_el}, indexedValue);
        values->setValue({"-V"_el, "--very-verbose"_el}, countedFlagValue);

        REQUIRE(values->getFlag("--verbose"_el));
        REQUIRE(values->value("--verbose"_el)->option().lock() == sourceOption);
        REQUIRE(values->value("--verbose"_el)->type() == OptionValueType::Flag);
        REQUIRE(values->value("--verbose"_el)->getFlag());
        REQUIRE(values->value("--count"_el)->type() == OptionValueType::Integer);
        REQUIRE_EQUAL(values->value("--count"_el)->getInteger(), OptionInteger{42});
        REQUIRE(values->value("--count"_el)->getFlag(true));
        REQUIRE(values->value("name"_el)->type() == OptionValueType::Text);
        REQUIRE(values->value("name"_el)->getText() == "Ada"_el);
        REQUIRE(values->value("names"_el)->type() == OptionValueType::TextList);
        REQUIRE(values->value("counts"_el)->type() == OptionValueType::IntegerList);
        REQUIRE_EQUAL(values->valueCount("--verbose"_el), ArgumentCount::one());
        REQUIRE_EQUAL(values->getFlagCount("--verbose"_el), ArgumentCount::zero());
        REQUIRE_EQUAL(values->getFlagCount("--very-verbose"_el), ArgumentCount{2U});
        REQUIRE_EQUAL(values->getFlagCount("-V"_el), ArgumentCount{2U});
        REQUIRE_EQUAL(values->getFlagCount("missing"_el), ArgumentCount::zero());
        REQUIRE_EQUAL(values->getFlagCount("missing"_el, ArgumentCount{9U}), ArgumentCount{9U});
        REQUIRE_EQUAL(values->getFlagCount("name"_el), ArgumentCount::zero());
        REQUIRE_EQUAL(values->getInteger("--count"_el), OptionInteger{42});
        REQUIRE(values->getText("name"_el) == "Ada"_el);
        REQUIRE(values->getText("missing"_el, "fallback"_el) == "fallback"_el);

        const auto textList = values->getTextList("names"_el);
        REQUIRE_EQUAL(textList.size(), 2U);
        REQUIRE(textList.at(0) == "Ada"_el);
        REQUIRE(textList.at(1) == "Bjarne"_el);
        const auto singleTextList = values->value("name"_el)->getTextList();
        REQUIRE_EQUAL(singleTextList.size(), 1U);
        REQUIRE(singleTextList.at(0) == "Ada"_el);

        const auto integerList = values->getIntegerList("counts"_el);
        REQUIRE_EQUAL(integerList.size(), 2U);
        REQUIRE_EQUAL(integerList.at(0), OptionInteger{1});
        REQUIRE_EQUAL(integerList.at(1), OptionInteger{2});
        const auto singleIntegerList = values->value("--count"_el)->getIntegerList();
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
        REQUIRE(values->value("indexed"_el) == values->value("INDEXED"_el));
        REQUIRE(values->value("indexed"_el)->argumentIndexes().at(0) == ArgumentIndex{3U});
    }

    void testManagerPlaceholderAndDocuments() {
        auto options = Options::create();
        auto manager = OptionManager{options};

        auto args = CommandLineArguments{String{"tool"_el}};
        const auto result = manager.parse(args);

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values() != nullptr);

        auto module = OptionModule::create("remove"_el);
        result.values()->setModuleName("remove"_el);
        result.values()->setModule(module);
        REQUIRE(result.values()->moduleName() == "remove"_el);
        REQUIRE(result.values()->module() == module);

        REQUIRE_FALSE(manager.helpDocument({}).isEmpty());
        REQUIRE_FALSE(manager.versionDocument({}).isEmpty());

        auto errorArgs = CommandLineArguments{String{"tool"_el}, String{"--unknown"_el}};
        const auto errorResult = manager.parse(errorArgs);
        REQUIRE(errorResult.status() == OptionResultStatus::Error);
        REQUIRE(errorResult.errorContext().has_value());
        REQUIRE(errorResult.errorContext()->reason() == OptionErrorReason::UnknownName);
        REQUIRE_FALSE(manager.errorDocument(errorResult.errorContext().value()).isEmpty());
    }
};
