// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/CommandLineArguments.hpp>
#include <erbsland/options/Option.hpp>
#include <erbsland/options/OptionChoices.hpp>
#include <erbsland/options/OptionError.hpp>
#include <erbsland/options/OptionErrorContext.hpp>
#include <erbsland/options/OptionFlag.hpp>
#include <erbsland/options/OptionManager.hpp>
#include <erbsland/options/OptionModule.hpp>
#include <erbsland/options/OptionResult.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionSet.hpp>
#include <erbsland/options/OptionType.hpp>
#include <erbsland/options/OptionValue.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
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
using el::text::StringView;
using el::unit::ArgumentCount;
using el::unit::ArgumentIndex;
using el::unit::ElementCount;
using el::unit::ElementIndex;
using namespace el::options;
using namespace el::text::literals;
namespace th = erbsland::unittest::th;

TESTED_TARGETS(OptionEditor OptionManager OptionParser)
class OptionsUsageTest final : public el::UnitTest {
public:
    void testChoicesPromoteOptionTypeForNamedAndPositionalUse() {
        auto options = Options::create();
        const auto demoChoices = OptionChoices::create({"basic"_el, "full"_el});
        const auto option = options->addOption({"-d"_el, "--demo"_el, "demo"_el})
                                .setChoices(demoChoices)
                                .setHelp("Specify the demo to run."_el)
                                .option();

        REQUIRE(option != nullptr);
        REQUIRE(option->type() == OptionType::Choice);

        auto result = parse(options, {"tool"_el, "-d"_el, "FULL"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText("-d"_el) == "full"_el);
        REQUIRE(result.values()->getText("--demo"_el) == "full"_el);
        REQUIRE(result.values()->getText("demo"_el) == "full"_el);
        requireArgumentIndexes(result.values()->value("--demo"_el), {ArgumentIndex{2U}});

        result = parse(options, {"tool"_el, "--demo=basic"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText("demo"_el) == "basic"_el);

        result = parse(options, {"tool"_el, "FULL"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Unexpected argument"_el, {}, {}, ArgumentIndex{1U});

        result = parse(options, {"tool"_el, "-d"_el, "missing"_el});
        requireError(
            result, OptionErrorReason::UnexpectedValueType, "Invalid choice"_el, option, {}, ArgumentIndex{2U});
    }

    void testAddChoicePromotesOptionTypeAndAcceptsDefaults() {
        auto options = Options::create();
        const auto option = options->addOption("--color"_el)
                                .addChoice("auto"_el)
                                .addChoice("never"_el)
                                .setDefaultValue(String{"AUTO"_el})
                                .option();

        REQUIRE(option != nullptr);
        REQUIRE(option->type() == OptionType::Choice);
        REQUIRE_EQUAL(option->choices()->choiceCount(), ArgumentCount{2U});

        auto result = parse(options, {"tool"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText("--color"_el) == "auto"_el);
        requireArgumentIndexes(result.values()->value("--color"_el), {});

        result = parse(options, {"tool"_el, "--color"_el, "NEVER"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText("--color"_el) == "never"_el);
    }

    void testRegularOptionAliasesAreValueLookupNamesOnly() {
        auto options = Options::create();
        const auto demoOption = options->addOption({"--demo"_el, "demo"_el}).option();
        const auto nameOption =
            options->addOption({"-n"_el, "--name"_el, "name"_el}).setType(OptionType::Text).option();

        REQUIRE(demoOption != nullptr);
        REQUIRE(demoOption->isRegularOption());
        REQUIRE(demoOption->type() == OptionType::Flag);
        REQUIRE(nameOption != nullptr);
        REQUIRE(nameOption->isRegularOption());
        REQUIRE(nameOption->type() == OptionType::Text);

        auto result = parse(options, {"tool"_el, "--demo"_el, "--name"_el, "Ada"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getFlag("--demo"_el));
        REQUIRE(result.values()->getFlag("demo"_el));
        REQUIRE(result.values()->value("--demo"_el) == result.values()->value("demo"_el));
        REQUIRE(result.values()->getText("--name"_el) == "Ada"_el);
        REQUIRE(result.values()->getText("name"_el) == "Ada"_el);
        REQUIRE(result.values()->value("-n"_el) == result.values()->value("name"_el));

        result = parse(options, {"tool"_el, "demo"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Unexpected argument"_el, {}, {}, ArgumentIndex{1U});

        result = parse(options, {"tool"_el, "Ada"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Unexpected argument"_el, {}, {}, ArgumentIndex{1U});
    }

    void testDashlessDefinitionsArePositionalTextByDefault() {
        auto options = Options::create();
        const auto pathOption = options->addOption("path"_el).option();
        const auto modeOption =
            options->addOption("mode"_el).setChoices(OptionChoices::create({"fast"_el, "safe"_el})).option();

        REQUIRE(pathOption != nullptr);
        REQUIRE(pathOption->isPositionalArgument());
        REQUIRE(pathOption->type() == OptionType::Text);
        REQUIRE(modeOption != nullptr);
        REQUIRE(modeOption->isPositionalArgument());
        REQUIRE(modeOption->type() == OptionType::Choice);

        auto result = parse(options, {"tool"_el, "input.txt"_el, "SAFE"_el});
        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->getText("path"_el) == "input.txt"_el);
        REQUIRE(result.values()->getText("mode"_el) == "safe"_el);

        result = parse(options, {"tool"_el, "--path"_el, "input.txt"_el});
        requireError(result, OptionErrorReason::UnknownName, "Unknown option"_el, {}, {}, ArgumentIndex{1U});
    }

    void testChoiceDefinitionErrorsPointToTheOptionDefinition() {
        auto options = Options::create();
        auto option = options->addOption("--mode"_el)
                          .setChoices(OptionChoices::create({"fast"_el, "safe"_el}))
                          .setType(OptionType::Flag)
                          .option();

        auto result = parse(options, {"tool"_el, "--mode"_el, "fast"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Invalid option definition"_el, option);

        options = Options::create();
        option = options->addOption("--mode"_el).setType(OptionType::Choice).option();

        result = parse(options, {"tool"_el, "--mode"_el, "fast"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Invalid option definition"_el, option);

        options = Options::create();
        option = options->addOption("--mode"_el).setChoices(OptionChoices::create()).option();

        result = parse(options, {"tool"_el, "--mode"_el, "fast"_el});
        requireError(result, OptionErrorReason::SyntaxError, "Invalid option definition"_el, option);
    }

    void testOptionTypesDefaultsAndValidatorsWorkTogether() {
        auto calls = std::vector<String>{};
        auto options = Options::create();
        const auto flagOption =
            options->addOption({"-v"_el, "--verbose"_el})
                .setType(OptionType::Flag)
                .setValidateFn([this, &calls](OptionValuePtr value, OptionValuesPtr values) -> void {
                    REQUIRE(value->flagCount() == ArgumentCount{3U});
                    REQUIRE(values->getFlagCount("--verbose"_el) == ArgumentCount{3U});
                    calls.emplace_back("flag"_el);
                })
                .option();
        const auto integerOption = options->addOption("--count"_el)
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
        const auto textOption = options->addOption("--name"_el)
                                    .setType(OptionType::Text)
                                    .setDefaultValue(String{"Ada"_el})
                                    .setValidateFn([this, &calls](OptionValuePtr value, OptionValuesPtr) -> void {
                                        REQUIRE(value->getText() == "Ada"_el);
                                        REQUIRE(value->argumentIndex().isNoIndex());
                                        calls.emplace_back("text"_el);
                                    })
                                    .option();
        const auto choiceOption = options->addOption("--mode"_el)
                                      .setChoices(OptionChoices::create({"fast"_el, "safe"_el}))
                                      .setMaximum(ArgumentCount{2U})
                                      .setValidateFn([this, &calls](OptionValuePtr value, OptionValuesPtr) -> void {
                                          const auto values = value->getTextList();
                                          REQUIRE_EQUAL(values.size(), 2U);
                                          REQUIRE(values.at(0) == "fast"_el);
                                          REQUIRE(values.at(1) == "safe"_el);
                                          calls.emplace_back("choice"_el);
                                      })
                                      .option();

        const auto result = parse(
            options,
            {"tool"_el,
                "-vv"_el,
                "--verbose"_el,
                "--count"_el,
                "7"_el,
                "--count=-2"_el,
                "--mode"_el,
                "FAST"_el,
                "--mode=safe"_el});

        REQUIRE(result.status() == OptionResultStatus::Success);
        REQUIRE(result.values()->value("--verbose"_el)->option().lock() == flagOption);
        REQUIRE(result.values()->value("--count"_el)->option().lock() == integerOption);
        REQUIRE(result.values()->value("--name"_el)->option().lock() == textOption);
        REQUIRE(result.values()->value("--mode"_el)->option().lock() == choiceOption);
        REQUIRE_EQUAL(calls.size(), 4U);
        REQUIRE(calls.at(0) == "flag"_el);
        REQUIRE(calls.at(1) == "integer"_el);
        REQUIRE(calls.at(2) == "text"_el);
        REQUIRE(calls.at(3) == "choice"_el);
    }

    void testOptionTypeErrorContextsStaySpecific() {
        auto options = Options::create();
        auto option = options->addOption("--flag"_el).setType(OptionType::Flag).option();
        requireError(
            parse(options, {"tool"_el, "--flag=value"_el}),
            OptionErrorReason::UnexpectedValueType,
            "Flag does not accept a value"_el,
            option,
            {},
            ArgumentIndex{1U});

        options = Options::create();
        option = options->addOption("--count"_el).setType(OptionType::Integer).option();
        requireError(
            parse(options, {"tool"_el, "--count"_el, "abc"_el}),
            OptionErrorReason::UnexpectedValueType,
            "Invalid integer value"_el,
            option,
            {},
            ArgumentIndex{2U});
        auto result = parse(options, {"tool"_el, "--count"_el, "abc"_el});
        REQUIRE(result.errorContext()->options() == options);
        auto manager = OptionManager{options};
        const auto text =
            StringConverter{manager.errorDocument(result.errorContext().value()).toString()}.toStdString();
        REQUIRE(text.find("Usage:\n  tool [options]\n") != std::string::npos);
        REQUIRE(text.find("2 │ abc\n") != std::string::npos);
        REQUIRE(text.find("View Full Help:\n  tool --help") != std::string::npos);

        options = Options::create();
        option = options->addOption("--name"_el).setType(OptionType::Text).option();
        requireError(
            parse(options, {"tool"_el, "--name"_el, "Ada"_el, "--name"_el, "Bob"_el}),
            OptionErrorReason::UnexpectedValueType,
            "Too many option values"_el,
            option,
            {},
            ArgumentIndex{4U});

        options = Options::create();
        option = options->addOption("--mode"_el).setChoices(OptionChoices::create({"fast"_el, "safe"_el})).option();
        requireError(
            parse(options, {"tool"_el, "--mode"_el, "slow"_el}),
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
        const auto esc = th::stdStringFromHex("1B");
        const auto unsafe = [&esc](const std::string_view prefix) -> String {
            auto bytes = std::string{prefix};
            bytes.append(esc);
            return String{std::string_view{bytes}};
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
        options->addOption("--count"_el).setType(OptionType::Integer);
        requireSafeDescription(parse(options, {"tool"_el, "--count"_el, unsafe("12")}));

        options = Options::create();
        options->addOption("--mode"_el).setChoices(OptionChoices::create({"fast"_el, "safe"_el}));
        requireSafeDescription(parse(options, {"tool"_el, "--mode"_el, unsafe("slow")}));

        options = Options::create();
        options->addModule(OptionModule::create("run"_el));
        requireSafeDescription(parse(options, {"tool"_el, unsafe("missing")}));
    }

    void testOptionErrorContextCarriesDiagnosticDetails() {
        auto options = Options::create();
        options->addOption("-t"_el).setType(OptionType::Integer);
        auto result = parse(options, {"file_size_monitor"_el, "-t"_el, "5"_el, "-x"_el, "-y"_el});
        requireError(result, OptionErrorReason::UnknownName, "Unknown option"_el, {}, {}, ArgumentIndex{3U});
        REQUIRE_EQUAL(result.errorContext()->arguments().count().toSizeT(), std::size_t{5U});
        REQUIRE(result.errorContext()->arguments().get(ElementIndex{0U}) == "file_size_monitor"_el);
        REQUIRE(result.errorContext()->arguments().get(ElementIndex{3U}) == "-x"_el);
        REQUIRE(result.errorContext()->arguments().get(ElementIndex{3U}) == "-x"_el);
        REQUIRE(result.errorContext()->options() == options);

        options = Options::create();
        const auto fileOption = options->addOption("file"_el)
                                    .setType(OptionType::Text)
                                    .setFlag(OptionFlag::Required)
                                    .setHelp("The path to the file to monitor."_el)
                                    .option();
        result = parse(options, {"file_size_monitor"_el});
        requireError(result, OptionErrorReason::UnexpectedValueType, "Required argument is missing"_el, fileOption);
        REQUIRE(result.errorContext()->options() == options);
        REQUIRE(result.errorContext()->option() == fileOption);
    }

    void testCallbackErrorsCompleteMissingContext() {
        auto options = Options::create();
        const auto option =
            options->addOption("--name"_el)
                .setType(OptionType::Text)
                .setValidateFn([](OptionValuePtr, OptionValuesPtr) -> void { throw OptionError{OptionErrorContext{}}; })
                .option();
        const auto optionSet = options->optionSets().front();
        auto postCalled = false;
        optionSet->setPostParsingFn([&postCalled](OptionValuesPtr) -> void { postCalled = true; });

        auto result = parse(options, {"tool"_el, "--name"_el, "Ada"_el});
        requireError(result, OptionErrorReason::ValidationError, {}, option, optionSet, ArgumentIndex{2U});
        REQUIRE_FALSE(postCalled);

        options = Options::create();
        options->addOption("--name"_el).setType(OptionType::Text);
        const auto postOptionSet = options->optionSets().front();
        postOptionSet->setPostParsingFn([](OptionValuesPtr) -> void { throw OptionError{OptionErrorContext{}}; });

        result = parse(options, {"tool"_el, "--name"_el, "Ada"_el});
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
    [[nodiscard]] static auto makeArgs(std::initializer_list<StringView> args) -> CommandLineArguments {
        auto result = CommandLineArguments{};
        result.reserve(ElementCount{args.size()});
        for (const auto &arg : args) {
            result.append(arg.copy());
        }
        return result;
    }

    [[nodiscard]] static auto parse(const OptionsPtr &options, std::initializer_list<StringView> args) -> OptionResult {
        auto manager = OptionManager{options};
        return manager.parse(makeArgs(args));
    }

    void requireError(
        const OptionResult &result,
        const OptionErrorReason reason,
        const StringView title = {},
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
};
