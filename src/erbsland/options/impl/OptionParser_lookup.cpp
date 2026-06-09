// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "../Option.hpp"
#include "../OptionChoices.hpp"
#include "../OptionModule.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"
#include "../OptionType.hpp"

#include "../../text/Literals.hpp"

namespace erbsland::options::impl {

using namespace text::literals;

auto OptionParser::collectActiveOptionSets() -> std::vector<OptionSetPtr> {
    auto result = std::vector<OptionSetPtr>{};
    if (_options == nullptr) {
        return result;
    }
    auto moduleSetCount = std::size_t{0};
    if (_selectedModule != nullptr) {
        moduleSetCount = _selectedModule->optionSets().size();
    }
    result.reserve(_options->optionSets().size() + moduleSetCount + 1U);
    if (_options->builtInOptionSet() != nullptr) {
        result.emplace_back(_options->builtInOptionSet());
    }
    for (const auto &optionSet : _options->optionSets()) {
        result.emplace_back(optionSet);
    }
    if (_selectedModule != nullptr) {
        for (const auto &optionSet : _selectedModule->optionSets()) {
            result.emplace_back(optionSet);
        }
    }
    return result;
}

auto OptionParser::findModule(const text::StringView &name) const -> OptionModulePtr {
    if (_options == nullptr) {
        return {};
    }
    for (const auto &module : _options->optionModules()) {
        if (module->hasName(name)) {
            return module;
        }
    }
    return {};
}

auto OptionParser::findLongOption(const text::StringView &name) const -> NameMatch {
    auto result = NameMatch{};
    for (const auto &optionSet : _activeOptionSets) {
        for (const auto &option : optionSet->options()) {
            if (option->hasLongName(name)) {
                if (option->isDisabled()) {
                    result.disabled = true;
                } else {
                    result.option = option;
                    return result;
                }
            }
        }
    }
    return result;
}

auto OptionParser::findShortOption(const text::Char shortName) const -> NameMatch {
    auto result = NameMatch{};
    for (const auto &optionSet : _activeOptionSets) {
        for (const auto &option : optionSet->options()) {
            if (option->hasShortName(shortName)) {
                if (option->isDisabled()) {
                    result.disabled = true;
                } else {
                    result.option = option;
                    return result;
                }
            }
        }
    }
    return result;
}

auto OptionParser::isHelpOrVersionRequest(OptionResultStatus &status) const -> bool {
    return isHelpOrVersionRequest(status, unit::ArgumentIndex::one());
}

auto OptionParser::isHelpOrVersionRequest(OptionResultStatus &status, const unit::ArgumentIndex startIndex) const
    -> bool {
    auto index = startIndex;
    while (index.toSizeT() < _args.size()) {
        const auto &argument = _args.at(index.toSizeT());
        if (argument == "--"_el) {
            return false;
        }
        if (argument == "-h"_el || argument == "--help"_el) {
            status = OptionResultStatus::DisplayHelp;
            return true;
        }
        if (argument == "--version"_el) {
            status = OptionResultStatus::DisplayVersion;
            return true;
        }
        ++index;
    }
    return false;
}

auto OptionParser::validateOptionNames() -> bool {
    for (const auto &optionSet : _activeOptionSets) {
        for (const auto &option : optionSet->options()) {
            if (option->isDisabled()) {
                continue;
            }
            if (!option->hasValidOptionNames()) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    "Malformed option definition"_el,
                    unit::ArgumentIndex::noIndex(),
                    option);
            }
            if (option->isPositionalArgument() && option->type() == OptionType::Flag) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    "Positional flags are not supported"_el,
                    unit::ArgumentIndex::noIndex(),
                    option);
            }
            if (option->choices() != nullptr && option->type() != OptionType::Choice) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    "Invalid option definition: choices require OptionType::Choice"_el,
                    unit::ArgumentIndex::noIndex(),
                    option);
            }
            if (option->type() == OptionType::Choice &&
                (option->choices() == nullptr || option->choices()->choiceCount().isZero())) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    "Invalid option definition: choice options require at least one choice"_el,
                    unit::ArgumentIndex::noIndex(),
                    option);
            }
            for (const auto &otherSet : _activeOptionSets) {
                for (const auto &otherOption : otherSet->options()) {
                    if (otherOption == option || otherOption->isDisabled()) {
                        continue;
                    }
                    if (option->hasConflictingOptionName(*otherOption)) {
                        return makeError(
                            OptionErrorReason::SyntaxError,
                            "Duplicate option name"_el,
                            unit::ArgumentIndex::noIndex(),
                            option);
                    }
                }
            }
        }
    }
    return true;
}

}
