// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "AsciiOptionText.hpp"
#include "OptionLiterals.hpp"

#include "../Option.hpp"
#include "../OptionFlag.hpp"
#include "../OptionModule.hpp"
#include "../OptionParserFlag.hpp"
#include "../Options.hpp"
#include "../OptionType.hpp"

#include "../../i18n/DisplayText.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringFormat.hpp"
#include "../../text/StringSide.hpp"
#include "../../text/u8/U8StringConstIterator.hpp"

namespace erbsland::options::impl {

using namespace text::literals;
using text::EscapeFormat;
using text::String;
using text::StringFormat;
using text::StringSide;
using unit::ArgumentIndex;
using unit::ByteCount;
using unit::ByteIndex;
using unit::CpIndex;

auto OptionParser::prepareModuleParsing() -> bool {
    _argumentIndex = ArgumentIndex::one();
    if (!isIndexInArgs(_argumentIndex)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.module_name_is_missing_title"_el),
            i18n::displayText("options.module_name_is_missing_description"_el),
            _argumentIndex);
    }

    const auto &argument = getArgAt(_argumentIndex);
    const auto builtInFlag = builtInFlagAt(_argumentIndex);
    if (builtInFlag.has_value() && builtInFlag->validValue) {
        return true;
    }
    if (argument.startsWith(option_literal::cShortNamePrefix)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.module_name_must_come_first_title"_el),
            i18n::displayText("options.module_name_must_come_first_description"_el),
            _argumentIndex);
    }
    if (!OptionModule::isValidName(argument)) {
        makeError(
            OptionErrorReason::UnknownName,
            i18n::displayText("options.unknown_module_title"_el),
            i18n::displayFormat("options.unknown_module_description"_el)
                .build(argument.toEscaped(EscapeFormat::Display)),
            _argumentIndex);
        _error->setSuggestions(suggestModules(normalizeOptionName(argument)));
        return false;
    }

    _selectedModule = findModule(normalizeOptionName(argument));
    if (_selectedModule == nullptr) {
        makeError(
            OptionErrorReason::UnknownName,
            i18n::displayText("options.unknown_module_title"_el),
            i18n::displayFormat("options.unknown_module_description"_el)
                .build(argument.toEscaped(EscapeFormat::Display)),
            _argumentIndex);
        _error->setSuggestions(suggestModules(normalizeOptionName(argument)));
        return false;
    }
    _moduleName = _selectedModule->name();
    _moduleArgumentIndex = _argumentIndex;
    _argumentIndex = _argumentIndex.incremented();
    return true;
}

auto OptionParser::parseActiveOptions() -> bool {
    while (isIndexInArgs(_argumentIndex)) {
        const auto &argument = getArgAt(_argumentIndex);
        if (argument == option_literal::cEndOfOptions) {
            ++_argumentIndex;
            while (isIndexInArgs(_argumentIndex)) {
                if (!collectPositionalArgument(getArgAt(_argumentIndex), _argumentIndex)) {
                    return false;
                }
                ++_argumentIndex;
            }
            return true;
        }
        if (argument.startsWith(option_literal::cLongNamePrefix)) {
            if (!parseLongOption(argument, _argumentIndex)) {
                return false;
            }
        } else if (argument.startsWith(option_literal::cShortNamePrefix)) {
            if (!parseShortOption(argument, _argumentIndex)) {
                return false;
            }
        } else if (!collectPositionalArgument(argument, _argumentIndex)) {
            return false;
        }
        ++_argumentIndex;
    }
    return true;
}

auto OptionParser::parseLongOption(const String &argument, const ArgumentIndex index) -> bool {
    const auto parts = argument.splitAtFirst(option_literal::cValueSeparator);
    auto rawName = argument;
    if (parts) {
        rawName = parts->first;
    }
    if (!Option::isValidLongName(rawName)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.malformed_long_option_title"_el),
            i18n::displayFormat("options.malformed_long_option_description"_el)
                .build(argument.toEscaped(EscapeFormat::Display)),
            index);
    }
    const auto name = normalizeOptionName(rawName);
    const auto match = findLongOption(name);
    if (match.option == nullptr || match.disabled) {
        makeError(
            OptionErrorReason::UnknownName,
            i18n::displayText("options.unknown_option_title"_el),
            i18n::displayFormat("options.unknown_option_description"_el).build(name.toEscaped(EscapeFormat::Display)),
            index);
        _error->setSuggestions(suggestLongOptions(name));
        return false;
    }

    const auto builtInHelpIsFlag = match.option->hasLongName(option_literal::cHelpLongName) &&
        _options->parserFlags().isSet(OptionParserFlag::NoHelpDetails);
    const auto optionType = builtInHelpIsFlag ? OptionType{OptionType::Flag} : match.option->type();
    if (optionType == OptionType::Flag) {
        if (parts) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                i18n::displayText("options.flag_does_not_accept_a_value_title"_el),
                i18n::displayFormat("options.named_flag_value_description"_el)
                    .build(name.toEscaped(EscapeFormat::Display)),
                index,
                match.option);
        }
        return acceptStorageResult(_storage.storeFlag(match.option, index));
    }

    auto value = String{};
    auto valueIndex = index;
    auto valueStartIndex = ByteIndex::zero();
    if (parts) {
        value = parts->second;
        valueStartIndex = ByteIndex::end(argument.length() - value.length());
    } else {
        const auto nextIndex = _argumentIndex.incremented();
        const auto acceptAsFlag = match.option->flags().isSet(OptionFlag::AcceptAsFlag);
        if (acceptAsFlag &&
            (!isIndexInArgs(nextIndex) || getArgAt(nextIndex).startsWith(option_literal::cShortNamePrefix))) {
            return acceptStorageResult(_storage.storeFlag(match.option, index));
        }
        if (acceptAsFlag && optionType == OptionType::Boolean) {
            auto ignored = false;
            if (!parseBooleanLiteral(getArgAt(nextIndex), ignored)) {
                return acceptStorageResult(_storage.storeFlag(match.option, index));
            }
        }
        if (!consumeFollowingValue(value, index, match.option)) {
            return false;
        }
        valueIndex = _argumentIndex;
    }
    return storeValue(match.option, value, valueIndex, valueStartIndex);
}

auto OptionParser::parseShortOption(const String &argument, const ArgumentIndex index) -> bool {
    if (const auto parts = argument.splitAtFirst(option_literal::cValueSeparator)) {
        const auto &name = parts->first;
        if (!Option::isValidShortName(name)) {
            return makeError(
                OptionErrorReason::SyntaxError,
                i18n::displayText("options.malformed_short_option_title"_el),
                i18n::displayFormat("options.malformed_short_option_description"_el)
                    .build(argument.toEscaped(EscapeFormat::Display)),
                index);
        }
        const auto shortName = name.charAt(CpIndex::one());
        const auto match = findShortOption(shortName);
        if (match.option == nullptr || match.disabled) {
            return makeError(
                OptionErrorReason::UnknownName,
                i18n::displayText("options.unknown_option_title"_el),
                i18n::displayFormat("options.unknown_option_description"_el)
                    .build(name.toEscaped(EscapeFormat::Display)),
                index);
        }
        if (match.option->hasLongName(option_literal::cHelpLongName)) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                i18n::displayText("options.short_help_does_not_accept_a_value_title"_el),
                i18n::displayFormat("options.short_help_value_description"_el).build(option_literal::cHelpLongName),
                index,
                match.option);
        }
        const auto isFlag = match.option->type() == OptionType::Flag ||
            (match.option->hasLongName(option_literal::cHelpLongName) &&
                _options->parserFlags().isSet(OptionParserFlag::NoHelpDetails));
        if (isFlag) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                i18n::displayText("options.flag_does_not_accept_a_value_title"_el),
                i18n::displayFormat("options.named_flag_value_description"_el)
                    .build(name.toEscaped(EscapeFormat::Display)),
                index,
                match.option);
        }
        return storeValue(
            match.option, parts->second, index, ByteIndex::end(argument.length() - parts->second.length()));
    }

    if (!argument.startsWith(option_literal::cShortNamePrefix)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.malformed_short_option_title"_el),
            i18n::displayFormat("options.malformed_short_option_description"_el)
                .build(argument.toEscaped(EscapeFormat::Display)),
            index);
    }

    const auto shortNames = argument.slice({ByteIndex::one(), ByteCount::infinite()});
    const auto firstCharacter = shortNames.charAt(StringSide::Front);
    const auto remainingNames = shortNames.slice({shortNames.indexAt(CpIndex::one()), ByteCount::infinite()});
    if (firstCharacter.isNull()) {
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.malformed_short_option_title"_el),
            i18n::displayFormat("options.malformed_short_option_description"_el)
                .build(argument.toEscaped(EscapeFormat::Display)),
            index);
    }
    const auto firstMatch = findShortOption(firstCharacter);
    if (firstMatch.option == nullptr || firstMatch.disabled) {
        return makeError(
            OptionErrorReason::UnknownName,
            i18n::displayText("options.unknown_option_title"_el),
            i18n::displayFormat("options.unknown_short_option_description"_el)
                .build(String::fromCharacter(firstCharacter).toEscaped(EscapeFormat::Display)),
            index);
    }
    if (remainingNames.isEmpty()) {
        const auto isFlag = firstMatch.option->type() == OptionType::Flag ||
            (firstMatch.option->hasLongName(option_literal::cHelpLongName) &&
                _options->parserFlags().isSet(OptionParserFlag::NoHelpDetails));
        if (isFlag) {
            return acceptStorageResult(_storage.storeFlag(firstMatch.option, index));
        }
        const auto nextIndex = _argumentIndex.incremented();
        if (firstMatch.option->flags().isSet(OptionFlag::AcceptAsFlag) &&
            (!isIndexInArgs(nextIndex) || getArgAt(nextIndex).startsWith(option_literal::cShortNamePrefix))) {
            return acceptStorageResult(_storage.storeFlag(firstMatch.option, index));
        }
        if (firstMatch.option->flags().isSet(OptionFlag::AcceptAsFlag) &&
            firstMatch.option->type() == OptionType::Boolean) {
            auto ignored = false;
            if (!parseBooleanLiteral(getArgAt(nextIndex), ignored)) {
                return acceptStorageResult(_storage.storeFlag(firstMatch.option, index));
            }
        }
        auto value = String{};
        if (!consumeFollowingValue(value, index, firstMatch.option)) {
            return false;
        }
        return storeValue(firstMatch.option, value, _argumentIndex, ByteIndex::zero());
    }
    if (firstMatch.option->type() != OptionType::Flag) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.option_cannot_be_grouped_title"_el),
            i18n::displayFormat("options.grouped_short_option_requires_value_description"_el).build(firstCharacter),
            index,
            firstMatch.option);
    }
    if (!acceptStorageResult(_storage.storeFlag(firstMatch.option, index))) {
        return false;
    }
    for (const auto character : remainingNames) {
        const auto match = findShortOption(character);
        if (match.option == nullptr || match.disabled) {
            return makeError(
                OptionErrorReason::UnknownName,
                i18n::displayText("options.unknown_option_title"_el),
                i18n::displayFormat("options.unknown_short_option_description"_el)
                    .build(String::fromCharacter(character).toEscaped(EscapeFormat::Display)),
                index);
        }
        if (match.option->type() != OptionType::Flag) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                i18n::displayText("options.option_cannot_be_grouped_title"_el),
                i18n::displayFormat("options.grouped_short_option_requires_value_description"_el).build(character),
                index,
                match.option);
        }
        if (!acceptStorageResult(_storage.storeFlag(match.option, index))) {
            return false;
        }
    }
    return true;
}

auto OptionParser::consumeFollowingValue(String &value, const ArgumentIndex optionIndex, const OptionPtr &option)
    -> bool {
    const auto optionName = option == nullptr || option->names().isEmpty() ? String{} : option->names().first();
    const auto valueIndex = _argumentIndex.incremented();
    if (!isIndexInArgs(valueIndex)) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.option_value_is_missing_title"_el),
            i18n::displayFormat("options.missing_option_value_description"_el).build(optionName),
            optionIndex,
            option);
    }
    const auto &candidate = getArgAt(valueIndex);
    if (candidate.startsWith(option_literal::cShortNamePrefix)) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            i18n::displayText("options.option_value_is_missing_title"_el),
            i18n::displayFormat("options.option_value_before_next_option_description"_el).build(optionName),
            optionIndex,
            option);
    }
    value = candidate;
    _argumentIndex = valueIndex;
    return true;
}

}
