// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "OptionDisplayModel.hpp"

#include "../Option.hpp"
#include "../OptionChoices.hpp"
#include "../OptionFlag.hpp"
#include "../OptionModule.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"
#include "../OptionType.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringFormat.hpp"

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
    const auto addEnabledSet = [&result](const OptionSetPtr &optionSet) -> void {
        if (optionSet != nullptr && !optionSet->flags().isSet(OptionFlag::Disabled)) {
            result.emplace_back(optionSet);
        }
    };
    addEnabledSet(_options->builtInOptionSet());
    for (const auto &optionSet : _options->optionSets()) {
        addEnabledSet(optionSet);
    }
    if (_selectedModule != nullptr) {
        for (const auto &optionSet : _selectedModule->optionSets()) {
            addEnabledSet(optionSet);
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
    while (isIndexInArgs(index)) {
        const auto &argument = getArgAt(index);
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
                    "Invalid option definition"_el,
                    text::StringFormat{"{} contains one or more malformed option names."}.build(
                        OptionDisplayModel::optionTitle(option)),
                    unit::ArgumentIndex::noIndex(),
                    option);
            }
            if (option->isPositionalArgument() && option->type() == OptionType::Flag) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    "Invalid option definition"_el,
                    text::StringFormat{"{} is positional, but positional arguments cannot be flags."}.build(
                        OptionDisplayModel::optionTitle(option)),
                    unit::ArgumentIndex::noIndex(),
                    option);
            }
            if (option->choices() != nullptr && option->type() != OptionType::Choice) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    "Invalid option definition"_el,
                    text::StringFormat{"{} defines choices, but its type is not OptionType::Choice."}.build(
                        OptionDisplayModel::optionTitle(option)),
                    unit::ArgumentIndex::noIndex(),
                    option);
            }
            if (option->type() == OptionType::Choice &&
                (option->choices() == nullptr || option->choices()->choiceCount().isZero())) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    "Invalid option definition"_el,
                    text::StringFormat{"{} is a choice option, but it has no accepted choices."}.build(
                        OptionDisplayModel::optionTitle(option)),
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
                            text::StringFormat{"{} conflicts with another active option definition."}.build(
                                OptionDisplayModel::optionTitle(option)),
                            unit::ArgumentIndex::noIndex(),
                            option);
                    }
                }
            }
        }
    }
    return true;
}

auto OptionParser::isIndexInArgs(const unit::ArgumentIndex index) const -> bool {
    return index.toSizeT() < _args.count().toSizeT();
}

auto OptionParser::getArgAt(const unit::ArgumentIndex index) const -> text::StringView {
    if (!isIndexInArgs(index)) {
        return {};
    }
    return _args.get(unit::ElementIndex::fromSizeT(index.toSizeT()));
}

}
