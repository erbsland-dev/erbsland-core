// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "../Option.hpp"
#include "../OptionModule.hpp"
#include "../OptionType.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringSide.hpp"

namespace erbsland::options::impl {

using namespace text::literals;

auto OptionParser::prepareModuleParsing() -> bool {
    _argumentIndex = unit::ArgumentIndex::one();
    if (!isIndexInArgs(_argumentIndex)) {
        return makeError(OptionErrorReason::SyntaxError, "Missing module name"_el, _argumentIndex);
    }

    const auto &argument = getArgAt(_argumentIndex);
    if (argument == "-h"_el || argument == "--help"_el || argument == "--version"_el) {
        return true;
    }
    if (argument.startsWith("-"_el)) {
        return makeError(OptionErrorReason::SyntaxError, "Expected module name before options"_el, _argumentIndex);
    }
    if (!OptionModule::isValidName(argument)) {
        return makeError(OptionErrorReason::UnknownName, "Unknown module name"_el, _argumentIndex);
    }

    _selectedModule = findModule(argument);
    if (_selectedModule == nullptr) {
        return makeError(OptionErrorReason::UnknownName, "Unknown module name"_el, _argumentIndex);
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
        return makeError(OptionErrorReason::SyntaxError, "Malformed long option"_el, index);
    }
    const auto match = findLongOption(name);
    if (match.option == nullptr || match.disabled) {
        return makeError(OptionErrorReason::UnknownName, "Unknown option name"_el, index);
    }

    const auto optionType = match.option->type();
    if (optionType == OptionType::Flag) {
        if (!equalsIndex.isNoIndex()) {
            return makeError(
                OptionErrorReason::UnexpectedValueType, "Flags do not accept values"_el, index, match.option);
        }
        return acceptStorageResult(_storage.storeFlag(match.option, index));
    }

    auto value = text::StringView{};
    auto valueIndex = index;
    if (!equalsIndex.isNoIndex()) {
        value = argument.slice({equalsIndex.incremented(), unit::ByteLength::infinite()});
    } else if (!consumeFollowingValue(value, index)) {
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
            return makeError(OptionErrorReason::SyntaxError, "Malformed short option"_el, index);
        }
        const auto shortName = name.charAt(unit::CpIndex::one());
        const auto match = findShortOption(shortName);
        if (match.option == nullptr || match.disabled) {
            return makeError(OptionErrorReason::UnknownName, "Unknown option name"_el, index);
        }
        if (match.option->type() == OptionType::Flag) {
            return makeError(
                OptionErrorReason::UnexpectedValueType, "Flags do not accept values"_el, index, match.option);
        }
        return acceptStorageResult(_storage.storeValue(
            match.option, argument.slice({equalsIndex.incremented(), unit::ByteLength::infinite()}), index));
    }

    if (!argument.startsWith("-"_el)) {
        return makeError(OptionErrorReason::SyntaxError, "Malformed short option"_el, index);
    }

    const auto shortNames = argument.slice({unit::ByteIndex::one(), unit::ByteLength::infinite()});
    const auto firstCharacter = shortNames.charAt(text::StringSide::Front);
    const auto remainingNames =
        shortNames.slice({shortNames.indexAt(unit::CpIndex::one()), unit::ByteLength::infinite()});
    if (firstCharacter.isNull()) {
        return makeError(OptionErrorReason::SyntaxError, "Malformed short option"_el, index);
    }
    const auto firstMatch = findShortOption(firstCharacter);
    if (firstMatch.option == nullptr || firstMatch.disabled) {
        return makeError(OptionErrorReason::UnknownName, "Unknown option name"_el, index);
    }
    if (remainingNames.isEmpty()) {
        if (firstMatch.option->type() == OptionType::Flag) {
            return acceptStorageResult(_storage.storeFlag(firstMatch.option, index));
        }
        auto value = text::StringView{};
        if (!consumeFollowingValue(value, index)) {
            return false;
        }
        return acceptStorageResult(_storage.storeValue(firstMatch.option, value, _argumentIndex));
    }
    if (firstMatch.option->type() != OptionType::Flag) {
        return makeError(
            OptionErrorReason::UnexpectedValueType,
            "Grouped short options only support flags"_el,
            index,
            firstMatch.option);
    }
    if (!acceptStorageResult(_storage.storeFlag(firstMatch.option, index))) {
        return false;
    }
    for (const auto character : remainingNames) {
        const auto match = findShortOption(character);
        if (match.option == nullptr || match.disabled) {
            return makeError(OptionErrorReason::UnknownName, "Unknown option name"_el, index);
        }
        if (match.option->type() != OptionType::Flag) {
            return makeError(
                OptionErrorReason::UnexpectedValueType,
                "Grouped short options only support flags"_el,
                index,
                match.option);
        }
        if (!acceptStorageResult(_storage.storeFlag(match.option, index))) {
            return false;
        }
    }
    return true;
}

auto OptionParser::consumeFollowingValue(text::StringView &value, const unit::ArgumentIndex optionIndex) -> bool {
    const auto valueIndex = _argumentIndex.incremented();
    if (!isIndexInArgs(valueIndex)) {
        return makeError(OptionErrorReason::UnexpectedValueType, "Missing option value"_el, optionIndex);
    }
    const auto &candidate = getArgAt(valueIndex);
    if (candidate.startsWith("-"_el)) {
        return makeError(OptionErrorReason::UnexpectedValueType, "Missing option value"_el, optionIndex);
    }
    value = candidate;
    _argumentIndex = valueIndex;
    return true;
}

}
