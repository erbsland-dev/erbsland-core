// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParserStorage.hpp"

#include "OptionDisplayModel.hpp"

#include "../Option.hpp"
#include "../OptionFlag.hpp"
#include "../OptionSet.hpp"
#include "../OptionType.hpp"
#include "../OptionValue.hpp"
#include "../OptionValues.hpp"

#include "../../err/Exception.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringFormat.hpp"

#include <utility>
#include <variant>

namespace erbsland::options::impl {

using namespace text::literals;
using text::EscapeFormat;
using text::String;
using text::StringFormat;
using unit::ArgumentCount;
using unit::ArgumentIndex;

auto OptionParserStorage::optionTitleForError(const OptionPtr &option) -> String {
    return OptionDisplayModel::optionTitle(option);
}

auto OptionParserStorage::storeFlag(const OptionPtr &option, const ArgumentIndex index) -> bool {
    auto parsedValue = findParsedValue(option);
    if (parsedValue == nullptr) {
        _parsedValues.emplace_back(OptionParsedValue::create(option, true, ArgumentCount::one(), std::vector{index}));
        return true;
    }
    if (!std::holds_alternative<bool>(parsedValue->storage)) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Option processing failed"_el,
            StringFormat{"The stored value for {} does not match its declared flag type."}.build(
                optionTitleForError(option)),
            index,
            option);
    }
    parsedValue->storage = true;
    parsedValue->count = ArgumentCount::one();
    parsedValue->argumentIndexes.emplace_back(index);
    return true;
}

auto OptionParserStorage::storeValue(const OptionPtr &option, const String &value, const ArgumentIndex index) -> bool {
    switch (option->type().type()) {
    case OptionType::Flag:
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Flag does not accept a value"_el,
            StringFormat{"{} is a flag and must be specified without a value."}.build(optionTitleForError(option)),
            index,
            option);
    case OptionType::Integer:
        try {
            return storeIntegerValue(option, value.toIntegerOrThrow<OptionInteger>(), index);
        } catch (const err::Exception &) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                "Invalid integer value"_el,
                StringFormat{"\"{}\" is not a valid integer for {}."}.build(
                    value.toEscaped(EscapeFormat::Display), optionTitleForError(option)),
                index,
                option);
        }
    case OptionType::Text:
        return storeTextValue(option, value.copy(), index);
    case OptionType::Choice:
        if (const auto choiceText = option->matchingChoiceText(value)) {
            return storeTextValue(option, choiceText.value().copy(), index);
        }
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Invalid choice"_el,
            StringFormat{"\"{}\" is not accepted for {}. {}"}.build(
                value.toEscaped(EscapeFormat::Display),
                optionTitleForError(option),
                OptionDisplayModel::optionDetails(option)),
            index,
            option);
    }
    return makeError(
        OptionErrorReason::UnexpectedValueType,
        "Unsupported option type"_el,
        StringFormat{"{} uses an option type that this parser cannot store."}.build(optionTitleForError(option)),
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
                    "Required option is missing"_el,
                    StringFormat{"Specify {} before running this command."}.build(optionTitleForError(option)),
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
            "Too many option values"_el,
            StringFormat{"{} does not accept any values."}.build(optionTitleForError(option)),
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
            "Too many option values"_el,
            StringFormat{"{} accepts at most {} value(s)."}.build(
                optionTitleForError(option), option->maximum().toSizeT()),
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
            "Option processing failed"_el,
            StringFormat{"The stored value for {} does not match its declared integer type."}.build(
                optionTitleForError(option)),
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
            "Too many option values"_el,
            StringFormat{"{} does not accept any values."}.build(optionTitleForError(option)),
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
            "Too many option values"_el,
            StringFormat{"{} accepts at most {} value(s)."}.build(
                optionTitleForError(option), option->maximum().toSizeT()),
            index,
            option);
    }
    if (const auto text = std::get_if<String>(&parsedValue->storage)) {
        parsedValue->storage = std::vector<String>{*text, std::move(value)};
    } else if (auto textList = std::get_if<std::vector<String>>(&parsedValue->storage)) {
        textList->emplace_back(std::move(value));
    } else {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Option processing failed"_el,
            StringFormat{"The stored value for {} does not match its declared text type."}.build(
                optionTitleForError(option)),
            index,
            option);
    }
    ++parsedValue->count;
    parsedValue->argumentIndexes.emplace_back(index);
    return true;
}

auto OptionParserStorage::storeDefaultValue(const OptionPtr &option) -> bool {
    const auto &defaultValue = option->defaultValue();
    if (!defaultValue.has_value()) {
        return true;
    }
    switch (option->type().type()) {
    case OptionType::Flag:
        if (const auto flag = std::get_if<bool>(&defaultValue.value())) {
            _parsedValues.emplace_back(OptionParsedValue::create(option, *flag, ArgumentCount::one()));
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
        if (const auto textList = std::get_if<std::vector<String>>(&defaultValue.value())) {
            if (ArgumentCount::fromSizeT(textList->size()) > option->maximum()) {
                break;
            }
            _parsedValues.emplace_back(
                OptionParsedValue::create(option, *textList, ArgumentCount::fromSizeT(textList->size())));
            return true;
        }
        break;
    case OptionType::Choice:
        if (const auto text = std::get_if<String>(&defaultValue.value())) {
            if (const auto choiceText = option->matchingChoiceText(*text)) {
                return storeDefaultTextValue(option, choiceText.value().copy());
            }
            break;
        }
        if (const auto textList = std::get_if<std::vector<String>>(&defaultValue.value())) {
            auto canonicalValues = std::vector<String>{};
            canonicalValues.reserve(textList->size());
            for (const auto &text : *textList) {
                const auto choiceText = option->matchingChoiceText(text);
                if (!choiceText.has_value()) {
                    break;
                }
                canonicalValues.emplace_back(choiceText.value().copy());
            }
            if (canonicalValues.size() == textList->size() &&
                ArgumentCount::fromSizeT(canonicalValues.size()) <= option->maximum()) {
                _parsedValues.emplace_back(
                    OptionParsedValue::create(
                        option, canonicalValues, ArgumentCount::fromSizeT(canonicalValues.size())));
                return true;
            }
        }
        break;
    }
    return makeError(
        OptionErrorReason::UnexpectedValueType,
        "Invalid default value"_el,
        StringFormat{"The configured default value for {} does not match its definition."}.build(
            optionTitleForError(option)),
        ArgumentIndex::noIndex(),
        option);
}

auto OptionParserStorage::storeDefaultIntegerValue(const OptionPtr &option, const OptionInteger value) -> bool {
    if (option->maximum().isZero()) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Invalid default value"_el,
            StringFormat{"{} does not accept a default value."}.build(optionTitleForError(option)),
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
            "Invalid default value"_el,
            StringFormat{"{} does not accept a default value."}.build(optionTitleForError(option)),
            ArgumentIndex::noIndex(),
            option);
    }
    _parsedValues.emplace_back(OptionParsedValue::create(option, std::move(value), ArgumentCount::one()));
    return true;
}

auto OptionParserStorage::makeError(const OptionErrorReason reason, String description, const ArgumentIndex index)
    -> bool {
    _error = OptionErrorContext{}.setReason(reason).setTitle(std::move(description)).setArgumentIndex(index);
    return false;
}

auto OptionParserStorage::makeError(
    const OptionErrorReason reason,
    String title,
    String description,
    const ArgumentIndex index,
    const OptionPtr &option) -> bool {
    _error = OptionErrorContext{}
                 .setReason(reason)
                 .setTitle(std::move(title))
                 .setDescription(std::move(description))
                 .setArgumentIndex(index)
                 .setOption(option);
    return false;
}

auto OptionParserStorage::makeError(
    const OptionErrorReason reason, String description, const ArgumentIndex index, const OptionPtr &option) -> bool {
    _error = OptionErrorContext{}
                 .setReason(reason)
                 .setTitle(std::move(description))
                 .setArgumentIndex(index)
                 .setOption(option);
    return false;
}

}
