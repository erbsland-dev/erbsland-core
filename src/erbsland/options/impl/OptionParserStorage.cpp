// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParserStorage.hpp"

#include "OptionDisplayModel.hpp"
#include "OptionSuggestion.hpp"

#include "../Option.hpp"
#include "../OptionChoice.hpp"
#include "../OptionChoices.hpp"
#include "../OptionFlag.hpp"
#include "../OptionHelpVisibility.hpp"
#include "../OptionSet.hpp"
#include "../OptionType.hpp"
#include "../OptionValue.hpp"
#include "../OptionValues.hpp"

#include "../../err/Exception.hpp"
#include "../../err/ParseError.hpp"
#include "../../i18n/DisplayText.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringFormat.hpp"

#include <utility>
#include <variant>

namespace erbsland::options::impl {

using namespace text;
using namespace text::literals;
using text::EscapeFormat;
using text::String;
using text::StringFormat;
using unit::ArgumentCount;
using unit::ArgumentIndex;

auto OptionParserStorage::storeFlag(const OptionPtr &option, const ArgumentIndex index) -> bool {
    auto parsedValue = findParsedValue(option);
    if (parsedValue == nullptr) {
        _parsedValues.emplace_back(
            OptionParsedValue::create(option, std::monostate{}, ArgumentCount::one(), std::vector{index}));
        return true;
    }
    if (option->type() != OptionType::Flag) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.too_many_option_values_title"_el),
            i18n::displayFormat("options.too_many_flag_occurrences_description"_el).build(optionTitleForError(option)),
            index,
            option);
    }
    if (!std::holds_alternative<std::monostate>(parsedValue->storage)) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.option_processing_failed_title"_el),
            i18n::displayFormat("options.stored_flag_type_mismatch_description"_el).build(optionTitleForError(option)),
            index,
            option);
    }
    ++parsedValue->count;
    parsedValue->argumentIndexes.emplace_back(index);
    return true;
}

auto OptionParserStorage::storeBooleanValue(const OptionPtr &option, const bool value, const ArgumentIndex index)
    -> bool {
    auto parsedValue = findParsedValue(option);
    if (option->maximum().isZero()) {
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.too_many_option_values_title"_el),
            i18n::displayFormat("options.option_values_not_accepted_description"_el).build(optionTitleForError(option)),
            index,
            option);
    }
    if (parsedValue == nullptr) {
        _parsedValues.emplace_back(OptionParsedValue::create(option, value, ArgumentCount::one(), std::vector{index}));
        return true;
    }
    if (parsedValue->count >= option->maximum()) {
        return makeError(
            option->maximum().isOne() ? OptionErrorReason::UnexpectedValueType : OptionErrorReason::SyntaxError,
            i18n::displayText("options.too_many_option_values_title"_el),
            i18n::displayFormat("options.too_many_option_values_description"_el)
                .build(optionTitleForError(option), option->maximum().toSizeT()),
            index,
            option);
    }
    if (const auto boolean = std::get_if<bool>(&parsedValue->storage)) {
        parsedValue->storage = std::vector<bool>{*boolean, value};
    } else if (auto booleanList = std::get_if<std::vector<bool>>(&parsedValue->storage)) {
        booleanList->emplace_back(value);
    } else {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.option_processing_failed_title"_el),
            i18n::displayFormat("options.stored_boolean_type_mismatch_description"_el)
                .build(optionTitleForError(option)),
            index,
            option);
    }
    ++parsedValue->count;
    parsedValue->argumentIndexes.emplace_back(index);
    return true;
}

auto OptionParserStorage::storeValue(const OptionPtr &option, String value, const ArgumentIndex index) -> bool {
    switch (option->type().type()) {
    case OptionType::Flag:
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.flag_does_not_accept_a_value_title"_el),
            i18n::displayFormat("options.flag_value_description"_el).build(optionTitleForError(option)),
            index,
            option);
    case OptionType::Boolean:
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.invalid_boolean_value_title"_el),
            i18n::displayFormat("options.unsupported_boolean_description"_el)
                .build(value.toEscaped(EscapeFormat::Display), optionTitleForError(option)),
            index,
            option);
    case OptionType::Integer:
        try {
            return storeIntegerValue(option, value.toIntegerOrThrow<OptionInteger>(), index);
        } catch (const err::ParseError &) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                i18n::displayText("options.invalid_integer_value_title"_el),
                i18n::displayFormat("options.invalid_integer_description"_el)
                    .build(value.toEscaped(EscapeFormat::Display), optionTitleForError(option)),
                index,
                option);
        }
    case OptionType::Text:
        return storeTextValue(option, value.copy(), index);
    case OptionType::SensitiveText: {
        value.markAsSensitive();
        return storeSensitiveTextValue(option, value.copy(), index);
    }
    case OptionType::Choice: {
        if (const auto choiceText = option->matchingChoiceText(value)) {
            return storeTextValue(option, choiceText.value().copy(), index);
        }
        makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.invalid_choice_title"_el),
            i18n::displayFormat("options.invalid_choice_description"_el)
                .build(
                    value.toEscaped(EscapeFormat::Display),
                    optionTitleForError(option),
                    OptionDisplayModel::optionDetails(option)),
            index,
            option);
        auto candidates = StringList{};
        if (option->choices() != nullptr && option->help().visibility() != OptionHelpVisibility::Hidden) {
            for (const auto &choice : option->choices()->choices()) {
                if (choice != nullptr && choice->help().visibility() != OptionHelpVisibility::Hidden) {
                    candidates.append(choice->text());
                }
            }
        }
        _error->setSuggestions(findOptionSuggestions(value, candidates));
        return false;
    }
    }
    return makeError(
        OptionErrorReason::UnexpectedValueType,
        i18n::displayText("options.unsupported_option_type_title"_el),
        i18n::displayFormat("options.unsupported_option_type_description"_el).build(optionTitleForError(option)),
        index,
        option);
}

auto OptionParserStorage::applyDefaults(const std::vector<OptionSetPtr> &optionSets) -> bool {
    for (const auto &optionSet : optionSets) {
        for (const auto &option : optionSet->options()) {
            if (option->isDisabled() || hasValue(option) || !option->hasDefaultValue()) {
                continue;
            }
            if (!storeDefaultValue(option)) {
                return false;
            }
        }
    }
    return true;
}

auto OptionParserStorage::checkRequiredOptions(const std::vector<OptionSetPtr> &optionSets) -> bool {
    for (const auto &optionSet : optionSets) {
        for (const auto &option : optionSet->options()) {
            if (option->isDisabled()) {
                continue;
            }
            if (option->flags().isSet(OptionFlag::Required) && !hasValue(option)) {
                return makeError(
                    OptionErrorReason::UnexpectedValueType,
                    i18n::displayText("options.required_option_is_missing_title"_el),
                    i18n::displayFormat("options.missing_required_option_description"_el)
                        .build(optionTitleForError(option)),
                    ArgumentIndex::noIndex(),
                    option);
            }
        }
    }
    return true;
}

auto OptionParserStorage::findParsedValue(const OptionPtr &option) -> OptionParsedValuePtr {
    for (auto &parsedValue : _parsedValues) {
        if (parsedValue->option == option) {
            return parsedValue;
        }
    }
    return nullptr;
}

auto OptionParserStorage::hasValue(const OptionPtr &option) const -> bool {
    for (const auto &parsedValue : _parsedValues) {
        if (parsedValue->option == option) {
            return true;
        }
    }
    return false;
}

auto OptionParserStorage::values() const -> OptionValuesPtr {
    auto values = OptionValues::create();
    for (const auto &parsedValue : _parsedValues) {
        const auto optionValue =
            OptionValue::create(parsedValue->option, parsedValue->storage, parsedValue->argumentIndexes);
        for (const auto &name : parsedValue->option->names()) {
            values->setValue(name, optionValue);
        }
    }
    return values;
}

auto OptionParserStorage::storeIntegerValue(
    const OptionPtr &option, const OptionInteger value, const ArgumentIndex index) -> bool {
    auto parsedValue = findParsedValue(option);
    if (option->maximum().isZero()) {
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.too_many_option_values_title"_el),
            i18n::displayFormat("options.option_values_not_accepted_description"_el).build(optionTitleForError(option)),
            index,
            option);
    }
    if (parsedValue == nullptr) {
        _parsedValues.emplace_back(OptionParsedValue::create(option, value, ArgumentCount::one(), std::vector{index}));
        return true;
    }
    if (parsedValue->count >= option->maximum()) {
        const auto reason =
            option->maximum().isOne() ? OptionErrorReason::UnexpectedValueType : OptionErrorReason::SyntaxError;
        return makeError(
            reason,
            i18n::displayText("options.too_many_option_values_title"_el),
            i18n::displayFormat("options.too_many_option_values_description"_el)
                .build(optionTitleForError(option), option->maximum().toSizeT()),
            index,
            option);
    }
    if (const auto integer = std::get_if<OptionInteger>(&parsedValue->storage)) {
        parsedValue->storage = std::vector<OptionInteger>{*integer, value};
    } else if (auto integerList = std::get_if<std::vector<OptionInteger>>(&parsedValue->storage)) {
        integerList->emplace_back(value);
    } else {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.option_processing_failed_title"_el),
            i18n::displayFormat("options.stored_integer_type_mismatch_description"_el)
                .build(optionTitleForError(option)),
            index,
            option);
    }
    ++parsedValue->count;
    parsedValue->argumentIndexes.emplace_back(index);
    return true;
}

auto OptionParserStorage::storeTextValue(const OptionPtr &option, String value, const ArgumentIndex index) -> bool {
    auto parsedValue = findParsedValue(option);
    if (option->maximum().isZero()) {
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.too_many_option_values_title"_el),
            i18n::displayFormat("options.option_values_not_accepted_description"_el).build(optionTitleForError(option)),
            index,
            option);
    }
    if (parsedValue == nullptr) {
        _parsedValues.emplace_back(
            OptionParsedValue::create(option, std::move(value), ArgumentCount::one(), std::vector{index}));
        return true;
    }
    if (parsedValue->count >= option->maximum()) {
        const auto reason =
            option->maximum().isOne() ? OptionErrorReason::UnexpectedValueType : OptionErrorReason::SyntaxError;
        return makeError(
            reason,
            i18n::displayText("options.too_many_option_values_title"_el),
            i18n::displayFormat("options.too_many_option_values_description"_el)
                .build(optionTitleForError(option), option->maximum().toSizeT()),
            index,
            option);
    }
    if (const auto text = std::get_if<String>(&parsedValue->storage)) {
        parsedValue->storage = StringList{*text, std::move(value)};
    } else if (auto textList = std::get_if<StringList>(&parsedValue->storage)) {
        textList->append(std::move(value));
    } else {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.option_processing_failed_title"_el),
            i18n::displayFormat("options.stored_text_type_mismatch_description"_el).build(optionTitleForError(option)),
            index,
            option);
    }
    ++parsedValue->count;
    parsedValue->argumentIndexes.emplace_back(index);
    return true;
}

auto OptionParserStorage::storeSensitiveTextValue(const OptionPtr &option, String value, const ArgumentIndex index)
    -> bool {
    if (findParsedValue(option) != nullptr) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.sensitive_option_cannot_be_repeated_title"_el),
            i18n::displayFormat("options.sensitive_value_count_description"_el).build(optionTitleForError(option)),
            index,
            option);
    }
    _parsedValues.emplace_back(
        OptionParsedValue::create(option, std::move(value), ArgumentCount::one(), std::vector{index}));
    return true;
}

auto OptionParserStorage::storeDefaultValue(const OptionPtr &option) -> bool {
    const auto &defaultValue = option->defaultValue();
    if (!defaultValue.has_value()) {
        return true;
    }
    switch (option->type().type()) {
    case OptionType::Flag:
        break;
    case OptionType::Boolean:
        if (const auto boolean = std::get_if<bool>(&defaultValue.value())) {
            _parsedValues.emplace_back(OptionParsedValue::create(option, *boolean, ArgumentCount::one()));
            return true;
        }
        if (const auto booleans = std::get_if<std::vector<bool>>(&defaultValue.value())) {
            if (ArgumentCount::fromSizeT(booleans->size()) > option->maximum()) {
                break;
            }
            _parsedValues.emplace_back(
                OptionParsedValue::create(option, *booleans, ArgumentCount::fromSizeT(booleans->size())));
            return true;
        }
        break;
    case OptionType::Integer:
        if (const auto integer = std::get_if<OptionInteger>(&defaultValue.value())) {
            return storeDefaultIntegerValue(option, *integer);
        }
        if (const auto integers = std::get_if<std::vector<OptionInteger>>(&defaultValue.value())) {
            if (ArgumentCount::fromSizeT(integers->size()) > option->maximum()) {
                break;
            }
            _parsedValues.emplace_back(
                OptionParsedValue::create(option, *integers, ArgumentCount::fromSizeT(integers->size())));
            return true;
        }
        break;
    case OptionType::Text:
        if (const auto text = std::get_if<String>(&defaultValue.value())) {
            return storeDefaultTextValue(option, *text);
        }
        if (const auto textList = std::get_if<StringList>(&defaultValue.value())) {
            const auto valueCount = ArgumentCount::fromSizeT(textList->count().toSizeT());
            if (valueCount > option->maximum()) {
                break;
            }
            _parsedValues.emplace_back(OptionParsedValue::create(option, *textList, valueCount));
            return true;
        }
        break;
    case OptionType::SensitiveText:
        break;
    case OptionType::Choice:
        if (const auto text = std::get_if<String>(&defaultValue.value())) {
            if (const auto choiceText = option->matchingChoiceText(*text)) {
                return storeDefaultTextValue(option, choiceText.value().copy());
            }
            break;
        }
        if (const auto textList = std::get_if<StringList>(&defaultValue.value())) {
            auto canonicalValues = StringList{};
            for (const auto &text : *textList) {
                const auto choiceText = option->matchingChoiceText(text);
                if (!choiceText.has_value()) {
                    break;
                }
                canonicalValues.append(choiceText.value().copy());
            }
            const auto valueCount = ArgumentCount::fromSizeT(canonicalValues.count().toSizeT());
            if (canonicalValues.count() == textList->count() && valueCount <= option->maximum()) {
                _parsedValues.emplace_back(OptionParsedValue::create(option, canonicalValues, valueCount));
                return true;
            }
        }
        break;
    }
    return makeError(
        OptionErrorReason::UnexpectedValueType,
        i18n::displayText("options.invalid_default_value_title"_el),
        i18n::displayFormat("options.invalid_default_value_description"_el).build(optionTitleForError(option)),
        ArgumentIndex::noIndex(),
        option);
}

auto OptionParserStorage::storeDefaultIntegerValue(const OptionPtr &option, const OptionInteger value) -> bool {
    if (option->maximum().isZero()) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.invalid_default_value_title"_el),
            i18n::displayFormat("options.default_value_not_accepted_description"_el).build(optionTitleForError(option)),
            ArgumentIndex::noIndex(),
            option);
    }
    _parsedValues.emplace_back(OptionParsedValue::create(option, value, ArgumentCount::one()));
    return true;
}

auto OptionParserStorage::storeDefaultTextValue(const OptionPtr &option, String value) -> bool {
    if (option->maximum().isZero()) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.invalid_default_value_title"_el),
            i18n::displayFormat("options.default_value_not_accepted_description"_el).build(optionTitleForError(option)),
            ArgumentIndex::noIndex(),
            option);
    }
    _parsedValues.emplace_back(OptionParsedValue::create(option, std::move(value), ArgumentCount::one()));
    return true;
}

}
