// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "../Option.hpp"
#include "../OptionFlag.hpp"
#include "../OptionModule.hpp"
#include "../OptionParserFlag.hpp"
#include "../Options.hpp"
#include "../OptionType.hpp"

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
using unit::ByteIndex;
using unit::ByteLength;
using unit::CpIndex;

auto OptionParser::prepareModuleParsing() -> bool {
    _argumentIndex = ArgumentIndex::one();
    if (!isIndexInArgs(_argumentIndex)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            "Module name is missing"_el,
            "Specify a module before any module options."_el,
            _argumentIndex);
    }

    const auto &argument = getArgAt(_argumentIndex);
    const auto builtInFlag = builtInFlagAt(_argumentIndex);
    if (builtInFlag.has_value() && builtInFlag->validValue) {
        return true;
    }
    if (argument.startsWith("-"_el)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            "Module name must come first"_el,
            "Specify a module before any command-line options."_el,
            _argumentIndex);
    }
    if (!OptionModule::isValidName(argument)) {
        makeError(
            OptionErrorReason::UnknownName,
            "Unknown module"_el,
            StringFormat{"\"{}\" is not an available module. Choose one of the modules listed below."}.build(
                argument.toEscaped(EscapeFormat::Display)),
            _argumentIndex);
        _error->setSuggestions(suggestModules(argument));
        return false;
    }

    _selectedModule = findModule(argument);
    if (_selectedModule == nullptr) {
        makeError(
            OptionErrorReason::UnknownName,
            "Unknown module"_el,
            StringFormat{"\"{}\" is not an available module. Choose one of the modules listed below."}.build(
                argument.toEscaped(EscapeFormat::Display)),
            _argumentIndex);
        _error->setSuggestions(suggestModules(argument));
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
        if (argument == "--"_el) {
            ++_argumentIndex;
            while (isIndexInArgs(_argumentIndex)) {
                if (!collectPositionalArgument(getArgAt(_argumentIndex), _argumentIndex)) {
                    return false;
                }
                ++_argumentIndex;
            }
            return true;
        }
        if (argument.startsWith("--"_el)) {
            if (!parseLongOption(argument, _argumentIndex)) {
                return false;
            }
        } else if (argument.startsWith("-"_el)) {
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
    const auto equalsIndex = argument.find("="_el);
    const auto name = argument.slice({ByteIndex::zero(), equalsIndex});
    if (!Option::isValidLongName(name)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            "Malformed long option"_el,
            StringFormat{"\"{}\" is not a valid long option. Use two dashes followed by an option name."}.build(
                argument.toEscaped(EscapeFormat::Display)),
            index);
    }
    const auto match = findLongOption(name);
    if (match.option == nullptr || match.disabled) {
        makeError(
            OptionErrorReason::UnknownName,
            "Unknown option"_el,
            StringFormat{"\"{}\" is not available for this command."}.build(name.toEscaped(EscapeFormat::Display)),
            index);
        _error->setSuggestions(suggestLongOptions(name));
        return false;
    }

    const auto builtInHelpIsFlag =
        match.option->hasLongName("--help"_el) && _options->parserFlags().isSet(OptionParserFlag::NoHelpDetails);
    const auto optionType = builtInHelpIsFlag ? OptionType{OptionType::Flag} : match.option->type();
    if (optionType == OptionType::Flag) {
        if (!equalsIndex.isNoIndex()) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                "Flag does not accept a value"_el,
                StringFormat{"\"{}\" is a flag and must be specified without a value."}.build(
                    name.toEscaped(EscapeFormat::Display)),
                index,
                match.option);
        }
        return acceptStorageResult(_storage.storeFlag(match.option, index));
    }

    auto value = String{};
    auto valueIndex = index;
    auto valueStartIndex = ByteIndex::zero();
    if (!equalsIndex.isNoIndex()) {
        valueStartIndex = equalsIndex.incremented();
        value = argument.slice({valueStartIndex, ByteLength::infinite()});
    } else {
        const auto nextIndex = _argumentIndex.incremented();
        const auto acceptAsFlag = match.option->flags().isSet(OptionFlag::AcceptAsFlag);
        if (acceptAsFlag && (!isIndexInArgs(nextIndex) || getArgAt(nextIndex).startsWith("-"_el))) {
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
    const auto equalsIndex = argument.find("="_el);
    if (!equalsIndex.isNoIndex()) {
        const auto name = argument.slice(StringSide::Front, equalsIndex.distanceFromZero());
        if (!Option::isValidShortName(name)) {
            return makeError(
                OptionErrorReason::SyntaxError,
                "Malformed short option"_el,
                StringFormat{"\"{}\" is not a valid short option. Use one dash followed by an option letter."}.build(
                    argument.toEscaped(EscapeFormat::Display)),
                index);
        }
        const auto shortName = name.charAt(CpIndex::one());
        const auto match = findShortOption(shortName);
        if (match.option == nullptr || match.disabled) {
            return makeError(
                OptionErrorReason::UnknownName,
                "Unknown option"_el,
                StringFormat{"\"{}\" is not available for this command."}.build(name.toEscaped(EscapeFormat::Display)),
                index);
        }
        if (match.option->hasLongName("--help"_el)) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                "Short help does not accept a value"_el,
                "Use the attached --help=<name> syntax to request detailed help."_el,
                index,
                match.option);
        }
        const auto isFlag = match.option->type() == OptionType::Flag ||
            (match.option->hasLongName("--help"_el) && _options->parserFlags().isSet(OptionParserFlag::NoHelpDetails));
        if (isFlag) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                "Flag does not accept a value"_el,
                StringFormat{"\"{}\" is a flag and must be specified without a value."}.build(
                    name.toEscaped(EscapeFormat::Display)),
                index,
                match.option);
        }
        return storeValue(
            match.option,
            argument.slice({equalsIndex.incremented(), ByteLength::infinite()}),
            index,
            equalsIndex.incremented());
    }

    if (!argument.startsWith("-"_el)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            "Malformed short option"_el,
            StringFormat{"\"{}\" is not a valid short option. Use one dash followed by an option letter."}.build(
                argument.toEscaped(EscapeFormat::Display)),
            index);
    }

    const auto shortNames = argument.slice({ByteIndex::one(), ByteLength::infinite()});
    const auto firstCharacter = shortNames.charAt(StringSide::Front);
    const auto remainingNames = shortNames.slice({shortNames.indexAt(CpIndex::one()), ByteLength::infinite()});
    if (firstCharacter.isNull()) {
        return makeError(
            OptionErrorReason::SyntaxError,
            "Malformed short option"_el,
            StringFormat{"\"{}\" is not a valid short option. Use one dash followed by an option letter."}.build(
                argument.toEscaped(EscapeFormat::Display)),
            index);
    }
    const auto firstMatch = findShortOption(firstCharacter);
    if (firstMatch.option == nullptr || firstMatch.disabled) {
        return makeError(
            OptionErrorReason::UnknownName,
            "Unknown option"_el,
            StringFormat{"\"-{}\" is not available for this command."}.build(
                String::fromCharacter(firstCharacter).toEscaped(EscapeFormat::Display)),
            index);
    }
    if (remainingNames.isEmpty()) {
        const auto isFlag = firstMatch.option->type() == OptionType::Flag ||
            (firstMatch.option->hasLongName("--help"_el) &&
                _options->parserFlags().isSet(OptionParserFlag::NoHelpDetails));
        if (isFlag) {
            return acceptStorageResult(_storage.storeFlag(firstMatch.option, index));
        }
        const auto nextIndex = _argumentIndex.incremented();
        if (firstMatch.option->flags().isSet(OptionFlag::AcceptAsFlag) &&
            (!isIndexInArgs(nextIndex) || getArgAt(nextIndex).startsWith("-"_el))) {
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
            "Option cannot be grouped"_el,
            StringFormat{"\"-{}\" requires a value and must be specified separately."}.build(firstCharacter),
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
                "Unknown option"_el,
                StringFormat{"\"-{}\" is not available for this command."}.build(
                    String::fromCharacter(character).toEscaped(EscapeFormat::Display)),
                index);
        }
        if (match.option->type() != OptionType::Flag) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                "Option cannot be grouped"_el,
                StringFormat{"\"-{}\" requires a value and must be specified separately."}.build(character),
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
    const auto optionName = option == nullptr || option->names().empty() ? String{} : option->names().front();
    const auto valueIndex = _argumentIndex.incremented();
    if (!isIndexInArgs(valueIndex)) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Option value is missing"_el,
            StringFormat{"\"{}\" requires a value."}.build(optionName),
            optionIndex,
            option);
    }
    const auto &candidate = getArgAt(valueIndex);
    if (candidate.startsWith("-"_el)) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Option value is missing"_el,
            StringFormat{"\"{}\" requires a value before the next option."}.build(optionName),
            optionIndex,
            option);
    }
    value = candidate;
    _argumentIndex = valueIndex;
    return true;
}

}
