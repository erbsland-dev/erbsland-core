// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/CommandLineArguments.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Option.hpp>
#include <erbsland/OptionChoices.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionErrorContext.hpp>
#include <erbsland/OptionFlag.hpp>
#include <erbsland/OptionManager.hpp>
#include <erbsland/OptionModule.hpp>
#include <erbsland/OptionResult.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionSet.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValue.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

using el::core::CommandLineArguments;
using el::options::OptionError;
using el::text::String;
using el::text::StringConverter;
using el::text::StringEditor;
using el::unit::ArgumentCount;
using el::unit::ArgumentIndex;
using el::unit::ItemCount;
using el::unit::ItemIndex;
using namespace el::options;
using namespace el::text::literals;
namespace th = erbsland::unittest::th;

TESTED_TARGETS(OptionEditor OptionManager OptionParser)
class OptionsUsageTest final : public el::UnitTest {
private:
    static constexpr auto cColorOption = "--color"_el;
    static constexpr auto cCountOption = "--count"_el;
    static constexpr auto cDemoOption = "--demo"_el;
    static constexpr auto cModeOption = "--mode"_el;
    static constexpr auto cNameOption = "--name"_el;
    static constexpr auto cVerboseOption = "--verbose"_el;
    static constexpr auto cDOption = "-d"_el;
    static constexpr auto cNOption = "-n"_el;
    static constexpr auto cDemoAliasOption = "demo"_el;
    static constexpr auto cModePositionalOption = "mode"_el;
    static constexpr auto cNameAliasOption = "name"_el;
    static constexpr auto cPathOption = "path"_el;

public:
    void testValueErrorsRetainOptionAndArgumentIndex() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto option = Option::create({cCountOption});
        const auto value = OptionValue::create(option, OptionInteger{0}, {ArgumentIndex{3}});
        for (
            const auto &error :
            {OptionError{"Invalid Count"_el, "Must be positive."_el, value},
                OptionError{"Must be positive."_el, value}}) {
            REQUIRE_EQUAL(error.context().option(), option);
            REQUIRE_EQUAL(error.context().argumentIndex(), ArgumentIndex{3});
            REQUIRE_EQUAL(error.context().reason(), OptionErrorReason::ValidationError);
        }
    }

    void testChoicesPromoteOptionTypeForNamedAndPositionalUse() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        const auto demoChoices = OptionChoices::create({"basic"_el, "full"_el});
        const auto option = options->addOption({cDOption, cDemoOption, cDemoAliasOption})
                                .setChoices(demoChoices)
                                .setHelp("Specify the demo to run."_el)
                                .option();

        REQUIRE(option);
        REQUIRE(option->type() == OptionType::Choice);

        auto result = parse(options, {"tool"_el, cDOption, "FULL"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText(cDOption) == "full"_el);
        REQUIRE(result.values()->getText(cDemoOption) == "full"_el);
        REQUIRE(result.values()->getText(cDemoAliasOption) == "full"_el);
        requireArgumentIndexes(result.values()->value(cDemoOption), {ArgumentIndex{2U}});

        result = parse(options, {"tool"_el, "--demo=basic"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText(cDemoAliasOption) == "basic"_el);

        result = parse(options, {"tool"_el, "FULL"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Unexpected argument"_el, {}, {}, ArgumentIndex{1U});

        result = parse(options, {"tool"_el, cDOption, "missing"_el});
        requireError(
            result, OptionErrorReason::UnexpectedValueType, "Invalid choice"_el, option, {}, ArgumentIndex{2U});
    }

    void testAddChoicePromotesOptionTypeAndAcceptsDefaults() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        const auto option = options->addOption(cColorOption)
                                .addChoice("auto"_el)
                                .addChoice("never"_el)
                                .setDefaultValue(StringEditor{"AUTO"_el})
                                .option();

        REQUIRE(option);
        REQUIRE(option->type() == OptionType::Choice);
        REQUIRE_EQUAL(option->choices()->choiceCount(), ArgumentCount{2U});

        auto result = parse(options, {"tool"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText(cColorOption) == "auto"_el);
        requireArgumentIndexes(result.values()->value(cColorOption), {});

        result = parse(options, {"tool"_el, cColorOption, "NEVER"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText(cColorOption) == "never"_el);
    }

    void testRegularOptionAliasesAreValueLookupNamesOnly() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        const auto demoOption = options->addOption({cDemoOption, cDemoAliasOption}).option();
        const auto nameOption =
            options->addOption({cNOption, cNameOption, cNameAliasOption}).setType(OptionType::Text).option();

        REQUIRE(demoOption);
        REQUIRE(demoOption->isRegularOption());
        REQUIRE(demoOption->type() == OptionType::Flag);
        REQUIRE(nameOption);
        REQUIRE(nameOption->isRegularOption());
        REQUIRE(nameOption->type() == OptionType::Text);

        auto result = parse(options, {"tool"_el, cDemoOption, cNameOption, "Ada"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getFlag(cDemoOption));
        REQUIRE(result.values()->getFlag(cDemoAliasOption));
        REQUIRE(result.values()->value(cDemoOption) == result.values()->value(cDemoAliasOption));
        REQUIRE(result.values()->getText(cNameOption) == "Ada"_el);
        REQUIRE(result.values()->getText(cNameAliasOption) == "Ada"_el);
        REQUIRE(result.values()->value(cNOption) == result.values()->value(cNameAliasOption));

        result = parse(options, {"tool"_el, cDemoAliasOption});
        requireError(result, OptionErrorReason::SyntaxError, "Unexpected argument"_el, {}, {}, ArgumentIndex{1U});

        result = parse(options, {"tool"_el, "Ada"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Unexpected argument"_el, {}, {}, ArgumentIndex{1U});
    }

    void testDashlessDefinitionsArePositionalTextByDefault() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        const auto pathOption = options->addOption(cPathOption).option();
        const auto modeOption = options->addOption(cModePositionalOption)
                                    .setChoices(OptionChoices::create({"fast"_el, "safe"_el}))
                                    .option();

        REQUIRE(pathOption);
        REQUIRE(pathOption->isPositionalArgument());
        REQUIRE(pathOption->type() == OptionType::Text);
        REQUIRE(modeOption);
        REQUIRE(modeOption->isPositionalArgument());
        REQUIRE(modeOption->type() == OptionType::Choice);

        auto result = parse(options, {"tool"_el, "input.txt"_el, "SAFE"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText(cPathOption) == "input.txt"_el);
        REQUIRE(result.values()->getText(cModePositionalOption) == "safe"_el);

        result = parse(options, {"tool"_el, "--path"_el, "input.txt"_el});
        requireError(result, OptionErrorReason::UnknownName, "Unknown option"_el, {}, {}, ArgumentIndex{1U});
    }

    void testChoiceDefinitionErrorsPointToTheOptionDefinition() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        auto option = options->addOption(cModeOption)
                          .setChoices(OptionChoices::create({"fast"_el, "safe"_el}))
                          .setType(OptionType::Flag)
                          .option();

        auto result = parse(options, {"tool"_el, cModeOption, "fast"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Invalid option definition"_el, option);

        options = Options::create();
        option = options->addOption(cModeOption).setType(OptionType::Choice).option();

        result = parse(options, {"tool"_el, cModeOption, "fast"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Invalid option definition"_el, option);

        options = Options::create();
        option = options->addOption(cModeOption).setChoices(OptionChoices::create()).option();

        result = parse(options, {"tool"_el, cModeOption, "fast"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Invalid option definition"_el, option);
    }

    void testOptionTypesDefaultsAndValidatorsWorkTogether() {
        auto applicationScope = ApplicationTestScope<>{};
        auto calls = std::vector<StringEditor>{};
        auto options = Options::create();
        const auto flagOption =
            options->addOption({"-v"_el, cVerboseOption})
                .setType(OptionType::Flag)
                .setValidateFn([this, &calls](OptionValuePtr value, OptionValuesPtr values) -> void {
                    REQUIRE(value->flagCount() == ArgumentCount{3U});
                    REQUIRE(values->getFlagCount(cVerboseOption) == ArgumentCount{3U});
                    calls.emplace_back("flag"_el);
                })
                .option();
        const auto integerOption = options->addOption(cCountOption)
                                       .setType(OptionType::Integer)
                                       .setMaximum(ArgumentCount{2U})
                                       .setValidateFn([this, &calls](OptionValuePtr value, OptionValuesPtr) -> void {
                                           const auto values = value->getIntegerList();
                                           REQUIRE_EQUAL(values.size(), 2U);
                                           REQUIRE_EQUAL(values.at(0), OptionInteger{7});
                                           REQUIRE_EQUAL(values.at(1), OptionInteger{-2});
                                           calls.emplace_back("integer"_el);
                                       })
                                       .option();
        const auto textOption = options->addOption(cNameOption)
                                    .setType(OptionType::Text)
                                    .setDefaultValue(StringEditor{"Ada"_el})
                                    .setValidateFn([this, &calls](OptionValuePtr value, OptionValuesPtr) -> void {
                                        REQUIRE(value->getText() == "Ada"_el);
                                        REQUIRE(value->argumentIndex().isNoIndex());
                                        calls.emplace_back("text"_el);
                                    })
                                    .option();
        const auto choiceOption = options->addOption(cModeOption)
                                      .setChoices(OptionChoices::create({"fast"_el, "safe"_el}))
                                      .setMaximum(ArgumentCount{2U})
                                      .setValidateFn([this, &calls](OptionValuePtr value, OptionValuesPtr) -> void {
                                          const auto values = value->getTextList();
                                          REQUIRE_EQUAL(values.count(), el::unit::ItemCount{2U});
                                          REQUIRE(values.get(el::unit::ItemIndex::zero()) == "fast"_el);
                                          REQUIRE(values.get(el::unit::ItemIndex{1U}) == "safe"_el);
                                          calls.emplace_back("choice"_el);
                                      })
                                      .option();

        const auto result = parse(
            options,
            {"tool"_el,
                "-vv"_el,
                cVerboseOption,
                cCountOption,
                "7"_el,
                "--count=-2"_el,
                cModeOption,
                "FAST"_el,
                "--mode=safe"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->value(cVerboseOption)->option().lock() == flagOption);
        REQUIRE(result.values()->value(cCountOption)->option().lock() == integerOption);
        REQUIRE(result.values()->value(cNameOption)->option().lock() == textOption);
        REQUIRE(result.values()->value(cModeOption)->option().lock() == choiceOption);
        REQUIRE_EQUAL(calls.size(), 4U);
        REQUIRE(calls.at(0) == "flag"_el);
        REQUIRE(calls.at(1) == "integer"_el);
        REQUIRE(calls.at(2) == "text"_el);
        REQUIRE(calls.at(3) == "choice"_el);
    }

    void testOptionTypeErrorContextsStaySpecific() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        auto option = options->addOption("--flag"_el).setType(OptionType::Flag).option();
        requireError(
            parse(options, {"tool"_el, "--flag=value"_el}),
            OptionErrorReason::UnexpectedValueType,
            "This option does not accept a value"_el,
            option,
            {},
            ArgumentIndex{1U});

        options = Options::create();
        option = options->addOption(cCountOption).setType(OptionType::Integer).option();
        requireError(
            parse(options, {"tool"_el, cCountOption, "abc"_el}),
            OptionErrorReason::UnexpectedValueType,
            "Invalid integer value"_el,
            option,
            {},
            ArgumentIndex{2U});
        auto result = parse(options, {"tool"_el, cCountOption, "abc"_el});
        REQUIRE(result.errorContext()->options() == options);
        auto manager = OptionManager{options};
        const auto text =
            StringConverter{manager.errorDocument(result.errorContext().value()).toString()}.toStdString();
        REQUIRE(text.find("Usage:\n  tool [options]\n") != std::string::npos);
        REQUIRE(text.find("3 │ abc\n") != std::string::npos);
        REQUIRE(text.find("View Full Help:\n  tool --help") != std::string::npos);

        options = Options::create();
        option = options->addOption(cNameOption).setType(OptionType::Text).option();
        requireError(
            parse(options, {"tool"_el, cNameOption, "Ada"_el, cNameOption, "Bob"_el}),
            OptionErrorReason::UnexpectedValueType,
            "Too many option values"_el,
            option,
            {},
            ArgumentIndex{4U});

        options = Options::create();
        option = options->addOption(cModeOption).setChoices(OptionChoices::create({"fast"_el, "safe"_el})).option();
        requireError(
            parse(options, {"tool"_el, cModeOption, "slow"_el}),
            OptionErrorReason::UnexpectedValueType,
            "Invalid choice"_el,
            option,
            {},
            ArgumentIndex{2U});

        options = Options::create();
        option = options->addOption("--required"_el).setType(OptionType::Text).setFlag(OptionFlag::Required).option();
        requireError(
            parse(options, {"tool"_el}),
            OptionErrorReason::UnexpectedValueType,
            "Required option is missing"_el,
            option);
    }

    void testBuiltInMessagesEscapeAllExternalOptionText() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto esc = th::stdStringFromHex("1B");
        const auto unsafe = [&esc](const std::string_view prefix) -> StringEditor {
            auto bytes = std::string{prefix};
            bytes.append(esc);
            return StringEditor{std::string_view{bytes}};
        };
        const auto requireSafeDescription = [this, &esc](const OptionResult &result) -> void {
            REQUIRE(result.errorContext().has_value());
            const auto description = StringConverter{result.errorContext()->description()}.toStdString();
            REQUIRE(description.find(esc) == std::string::npos);
        };

        auto options = Options::create();
        requireSafeDescription(parse(options, {"tool"_el, unsafe("--bad")}));
        requireSafeDescription(parse(options, {"tool"_el, unsafe("-x")}));
        requireSafeDescription(parse(options, {"tool"_el, unsafe("value")}));

        options = Options::create();
        options->addOption(cCountOption).setType(OptionType::Integer);
        requireSafeDescription(parse(options, {"tool"_el, cCountOption, unsafe("12")}));

        options = Options::create();
        options->addOption(cModeOption).setChoices(OptionChoices::create({"fast"_el, "safe"_el}));
        requireSafeDescription(parse(options, {"tool"_el, cModeOption, unsafe("slow")}));

        options = Options::create();
        options->addModule(OptionModule::create("run"_el));
        requireSafeDescription(parse(options, {"tool"_el, unsafe("missing")}));
    }

    void testOptionErrorContextCarriesDiagnosticDetails() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        options->addOption("-t"_el).setType(OptionType::Integer);
        auto result = parse(options, {"file_size_monitor"_el, "-t"_el, "5"_el, "-x"_el, "-y"_el});
        requireError(result, OptionErrorReason::UnknownName, "Unknown option"_el, {}, {}, ArgumentIndex{3U});
        REQUIRE_EQUAL(result.errorContext()->arguments().count().toSizeT(), std::size_t{5U});
        REQUIRE(result.errorContext()->arguments().get(ItemIndex{0U}) == "file_size_monitor"_el);
        REQUIRE(result.errorContext()->arguments().get(ItemIndex{3U}) == "-x"_el);
        REQUIRE(result.errorContext()->arguments().get(ItemIndex{3U}) == "-x"_el);
        REQUIRE(result.errorContext()->options() == options);

        options = Options::create();
        const auto fileOption = options->addOption("file"_el)
                                    .setType(OptionType::Text)
                                    .setFlag(OptionFlag::Required)
                                    .setHelp("The path to the file to monitor."_el)
                                    .option();
        result = parse(options, {"file_size_monitor"_el});
        REQUIRE(result.status() == OptionResultStatus::DisplayHelp);

        options->setParserFlag(OptionParserFlag::ErrorOnEmptyRequiredPositionals);
        result = parse(options, {"file_size_monitor"_el});
        requireError(result, OptionErrorReason::UnexpectedValueType, "Required argument is missing"_el, fileOption);
        REQUIRE(result.errorContext()->options() == options);
        REQUIRE(result.errorContext()->option() == fileOption);
    }

    void testCallbackErrorsCompleteMissingContext() {
        auto applicationScope = ApplicationTestScope<>{};
        auto options = Options::create();
        const auto option =
            options->addOption(cNameOption)
                .setType(OptionType::Text)
                .setValidateFn([](OptionValuePtr, OptionValuesPtr) -> void { throw OptionError{OptionErrorContext{}}; })
                .option();
        const auto optionSet = options->optionSets().front();
        auto postCalled = false;
        optionSet->setPostParsingFn([&postCalled](OptionValuesPtr) -> void { postCalled = true; });

        auto result = parse(options, {"tool"_el, cNameOption, "Ada"_el});
        requireError(result, OptionErrorReason::ValidationError, {}, option, optionSet, ArgumentIndex{2U});
        REQUIRE_FALSE(postCalled);

        options = Options::create();
        options->addOption(cNameOption).setType(OptionType::Text);
        const auto postOptionSet = options->optionSets().front();
        postOptionSet->setPostParsingFn([](OptionValuesPtr) -> void { throw OptionError{OptionErrorContext{}}; });

        result = parse(options, {"tool"_el, cNameOption, "Ada"_el});
        requireError(result, OptionErrorReason::ValidationError, {}, {}, postOptionSet);

        options = Options::create();
        auto module = OptionModule::create("run"_el);
        module->addOption("--force"_el).setType(OptionType::Flag);
        module->setPostParsingFn([](OptionValuesPtr) -> void {
            auto context = OptionErrorContext{};
            context.setDescription("Module rejected options"_el);
            throw OptionError{context};
        });
        options->addModule(module);

        result = parse(options, {"tool"_el, "run"_el, "--force"_el});
        requireError(result, OptionErrorReason::ValidationError);
        REQUIRE(result.errorContext()->description() == "Module rejected options"_el);
        REQUIRE(result.errorContext()->module() == module);
    }

private:
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

    void requireError(
        const OptionResult &result,
        const OptionErrorReason reason,
        const String &title = {},
        const OptionPtr &option = {},
        const OptionSetPtr &optionSet = {},
        const ArgumentIndex argumentIndex = ArgumentIndex::noIndex()) {
        REQUIRE(result.status() == OptionResultStatus::Error);
        REQUIRE(result.errorContext().has_value());
        const auto &context = result.errorContext().value();
        REQUIRE(context.reason() == reason);
        REQUIRE(context.title() == title);
        REQUIRE(context.option() == option);
        REQUIRE(context.optionSet() == optionSet);
        REQUIRE(context.argumentIndex() == argumentIndex);
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
};
