// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "../Option.hpp"
#include "../OptionModule.hpp"
#include "../OptionType.hpp"

#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringFormat.hpp"
#include "../../text/StringSide.hpp"

namespace erbsland::options::impl {

using namespace text::literals;

auto OptionParser::prepareModuleParsing() -> bool {
    _argumentIndex = unit::ArgumentIndex::one();
    if (!isIndexInArgs(_argumentIndex)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            "Module name is missing"_el,
            "Specify a module before any module options."_el,
            _argumentIndex);
    }

    const auto &argument = getArgAt(_argumentIndex);
    if (argument == "-h"_el || argument == "--help"_el || argument == "--version"_el) {
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
        return makeError(
            OptionErrorReason::UnknownName,
            "Unknown module"_el,
            text::StringFormat{"\"{}\" is not an available module. Choose one of the modules listed below."}.build(
                argument.toEscaped(text::EscapeFormat::Display)),
            _argumentIndex);
    }

    _selectedModule = findModule(argument);
    if (_selectedModule == nullptr) {
        return makeError(
            OptionErrorReason::UnknownName,
            "Unknown module"_el,
            text::StringFormat{"\"{}\" is not an available module. Choose one of the modules listed below."}.build(
                argument.toEscaped(text::EscapeFormat::Display)),
            _argumentIndex);
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
        if (argument == "-h"_el) {
            return true;
        }
        if (argument == "--help"_el || argument == "--version"_el) {
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

auto OptionParser::parseLongOption(const text::StringView &argument, const unit::ArgumentIndex index) -> bool {
    const auto equalsIndex = argument.find("="_el);
    const auto name = argument.slice({unit::ByteIndex::zero(), equalsIndex});
    if (!Option::isValidLongName(name)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            "Malformed long option"_el,
            text::StringFormat{"\"{}\" is not a valid long option. Use two dashes followed by an option name."}.build(
                argument.toEscaped(text::EscapeFormat::Display)),
            index);
    }
    const auto match = findLongOption(name);
    if (match.option == nullptr || match.disabled) {
        return makeError(
            OptionErrorReason::UnknownName,
            "Unknown option"_el,
            text::StringFormat{"\"{}\" is not available for this command."}.build(
                name.toEscaped(text::EscapeFormat::Display)),
            index);
    }

    const auto optionType = match.option->type();
    if (optionType == OptionType::Flag) {
        if (!equalsIndex.isNoIndex()) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                "Flag does not accept a value"_el,
                text::StringFormat{"\"{}\" is a flag and must be specified without a value."}.build(
                    name.toEscaped(text::EscapeFormat::Display)),
                index,
                match.option);
        }
        return acceptStorageResult(_storage.storeFlag(match.option, index));
    }

    auto value = text::StringView{};
    auto valueIndex = index;
    if (!equalsIndex.isNoIndex()) {
        value = argument.slice({equalsIndex.incremented(), unit::ByteLength::infinite()});
    } else if (!consumeFollowingValue(value, index, match.option)) {
        return false;
    } else {
        valueIndex = _argumentIndex;
    }
    return acceptStorageResult(_storage.storeValue(match.option, value, valueIndex));
}

auto OptionParser::parseShortOption(const text::StringView &argument, const unit::ArgumentIndex index) -> bool {
    const auto equalsIndex = argument.find("="_elv);
    if (!equalsIndex.isNoIndex()) {
        const auto name = argument.slice(text::StringSide::Front, equalsIndex.distanceFromZero());
        if (!Option::isValidShortName(name)) {
            return makeError(
                OptionErrorReason::SyntaxError,
                "Malformed short option"_el,
                text::StringFormat{"\"{}\" is not a valid short option. Use one dash followed by an option letter."}
                    .build(argument.toEscaped(text::EscapeFormat::Display)),
                index);
        }
        const auto shortName = name.charAt(unit::CpIndex::one());
        const auto match = findShortOption(shortName);
        if (match.option == nullptr || match.disabled) {
            return makeError(
                OptionErrorReason::UnknownName,
                "Unknown option"_el,
                text::StringFormat{"\"{}\" is not available for this command."}.build(
                    name.toEscaped(text::EscapeFormat::Display)),
                index);
        }
        if (match.option->type() == OptionType::Flag) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                "Flag does not accept a value"_el,
                text::StringFormat{"\"{}\" is a flag and must be specified without a value."}.build(
                    name.toEscaped(text::EscapeFormat::Display)),
                index,
                match.option);
        }
        return acceptStorageResult(_storage.storeValue(
            match.option, argument.slice({equalsIndex.incremented(), unit::ByteLength::infinite()}), index));
    }

    if (!argument.startsWith("-"_el)) {
        return makeError(
            OptionErrorReason::SyntaxError,
            "Malformed short option"_el,
            text::StringFormat{"\"{}\" is not a valid short option. Use one dash followed by an option letter."}.build(
                argument.toEscaped(text::EscapeFormat::Display)),
            index);
    }

    const auto shortNames = argument.slice({unit::ByteIndex::one(), unit::ByteLength::infinite()});
    const auto firstCharacter = shortNames.charAt(text::StringSide::Front);
    const auto remainingNames =
        shortNames.slice({shortNames.indexAt(unit::CpIndex::one()), unit::ByteLength::infinite()});
    if (firstCharacter.isNull()) {
        return makeError(
            OptionErrorReason::SyntaxError,
            "Malformed short option"_el,
            text::StringFormat{"\"{}\" is not a valid short option. Use one dash followed by an option letter."}.build(
                argument.toEscaped(text::EscapeFormat::Display)),
            index);
    }
    const auto firstMatch = findShortOption(firstCharacter);
    if (firstMatch.option == nullptr || firstMatch.disabled) {
        return makeError(
            OptionErrorReason::UnknownName,
            "Unknown option"_el,
            text::StringFormat{"\"-{}\" is not available for this command."}.build(
                text::String::fromCharacter(firstCharacter).toEscaped(text::EscapeFormat::Display)),
            index);
    }
    if (remainingNames.isEmpty()) {
        if (firstMatch.option->type() == OptionType::Flag) {
            return acceptStorageResult(_storage.storeFlag(firstMatch.option, index));
        }
        auto value = text::StringView{};
        if (!consumeFollowingValue(value, index, firstMatch.option)) {
            return false;
        }
        return acceptStorageResult(_storage.storeValue(firstMatch.option, value, _argumentIndex));
    }
    if (firstMatch.option->type() != OptionType::Flag) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Option cannot be grouped"_el,
            text::StringFormat{"\"-{}\" requires a value and must be specified separately."}.build(firstCharacter),
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
                text::StringFormat{"\"-{}\" is not available for this command."}.build(
                    text::String::fromCharacter(character).toEscaped(text::EscapeFormat::Display)),
                index);
        }
        if (match.option->type() != OptionType::Flag) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                "Option cannot be grouped"_el,
                text::StringFormat{"\"-{}\" requires a value and must be specified separately."}.build(character),
                index,
                match.option);
        }
        if (!acceptStorageResult(_storage.storeFlag(match.option, index))) {
            return false;
        }
    }
    return true;
}

auto OptionParser::consumeFollowingValue(
    text::StringView &value, const unit::ArgumentIndex optionIndex, const OptionPtr &option) -> bool {
    const auto optionName = option == nullptr || option->names().empty() ? text::StringView{} : option->names().front();
    const auto valueIndex = _argumentIndex.incremented();
    if (!isIndexInArgs(valueIndex)) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Option value is missing"_el,
            text::StringFormat{"\"{}\" requires a value."}.build(optionName),
            optionIndex,
            option);
    }
    const auto &candidate = getArgAt(valueIndex);
    if (candidate.startsWith("-"_el)) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Option value is missing"_el,
            text::StringFormat{"\"{}\" requires a value before the next option."}.build(optionName),
            optionIndex,
            option);
    }
    value = candidate;
    _argumentIndex = valueIndex;
    return true;
}

}
