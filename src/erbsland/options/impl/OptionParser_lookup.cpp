// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "OptionDisplayModel.hpp"

#include "../Option.hpp"
#include "../OptionChoices.hpp"
#include "../OptionFlag.hpp"
#include "../OptionModule.hpp"
#include "../OptionParserFlag.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"
#include "../OptionType.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringFormat.hpp"
#include "../../unit/ByteUnit.hpp"

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

auto OptionParser::findModule(const text::String &name) const -> OptionModulePtr {
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

auto OptionParser::findLongOption(const text::String &name) const -> NameMatch {
    auto result = NameMatch{};
    for (const auto &optionSet : _activeOptionSets) {
        for (const auto &option : optionSet->options()) {
            if (!isEnabledBuiltInOption(optionSet, option)) {
                continue;
            }
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
            if (!isEnabledBuiltInOption(optionSet, option)) {
                continue;
            }
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

auto OptionParser::isEnabledBuiltInOption(const OptionSetPtr &optionSet, const OptionPtr &option) const -> bool {
    if (_options == nullptr || optionSet != _options->builtInOptionSet()) {
        return true;
    }
    if (option == nullptr || option->isDisabled()) {
        return false;
    }
    if (option->hasLongName("--help"_el)) {
        return !_options->parserFlags().isSet(OptionParserFlag::DisableHelp);
    }
    if (option->hasLongName("--version"_el)) {
        return !_options->parserFlags().isSet(OptionParserFlag::DisableVersion);
    }
    return true;
}

auto OptionParser::isEnabledBuiltInFlag(const text::String &name) const -> bool {
    if (_options == nullptr) {
        return false;
    }
    const auto &optionSet = _options->builtInOptionSet();
    if (optionSet == nullptr || optionSet->flags().isSet(OptionFlag::Disabled)) {
        return false;
    }
    for (const auto &option : optionSet->options()) {
        if (!isEnabledBuiltInOption(optionSet, option) || option->type() != OptionType::Flag) {
            continue;
        }
        for (const auto &optionName : option->names()) {
            if (optionName == name) {
                return true;
            }
        }
    }
    return false;
}

auto OptionParser::booleanValuesEnabled() const noexcept -> bool {
    return _options != nullptr && !_options->parserFlags().isSet(OptionParserFlag::DisableBooleanValues);
}

auto OptionParser::parseBooleanLiteral(const text::String &text, bool &value) noexcept -> bool {
    const auto falseDefault = text.toBoolean(false);
    const auto trueDefault = text.toBoolean(true);
    if (falseDefault != trueDefault) {
        return false;
    }
    value = falseDefault;
    return true;
}

auto OptionParser::builtInFlagAt(const unit::ArgumentIndex index) const -> std::optional<BuiltInFlagMatch> {
    const auto argument = getArgAt(index);
    const auto equalsIndex = argument.find("="_el);
    const auto name = argument.slice({unit::ByteIndex::zero(), equalsIndex});
    if (name != "-h"_el && name != "--help"_el && name != "--version"_el) {
        return {};
    }
    if (!isEnabledBuiltInFlag(name)) {
        return {};
    }

    auto result =
        BuiltInFlagMatch{name == "--version"_el ? OptionResultStatus::DisplayVersion : OptionResultStatus::DisplayHelp};
    if (!equalsIndex.isNoIndex()) {
        result.explicitValue = true;
        if (!booleanValuesEnabled() ||
            !parseBooleanLiteral(
                argument.slice({equalsIndex.incremented(), unit::ByteLength::infinite()}), result.value)) {
            result.validValue = false;
        }
    } else if (booleanValuesEnabled()) {
        const auto valueIndex = index.incremented();
        if (isIndexInArgs(valueIndex) && parseBooleanLiteral(getArgAt(valueIndex), result.value)) {
            result.consumedFollowing = true;
            result.explicitValue = true;
        }
    }
    return result;
}

auto OptionParser::isHelpOrVersionRequest(OptionResultStatus &status) const -> bool {
    return isHelpOrVersionRequest(status, unit::ArgumentIndex::one());
}

auto OptionParser::isHelpOrVersionRequest(OptionResultStatus &status, const unit::ArgumentIndex startIndex) const
    -> bool {
    struct OccurrenceState final {
        std::size_t count{0};
        bool explicitValue{false};
    };
    auto helpState = OccurrenceState{};
    auto versionState = OccurrenceState{};
    auto requestedStatus = std::optional<OptionResultStatus>{};
    auto invalidSyntax = false;
    auto index = startIndex;
    while (isIndexInArgs(index)) {
        const auto &argument = getArgAt(index);
        if (argument == "--"_el) {
            break;
        }
        if (const auto match = builtInFlagAt(index)) {
            auto &state = match->status == OptionResultStatus::DisplayHelp ? helpState : versionState;
            invalidSyntax = invalidSyntax || !match->validValue ||
                (state.count > 0U && (state.explicitValue || match->explicitValue));
            ++state.count;
            state.explicitValue = state.explicitValue || match->explicitValue;
            if (match->validValue && match->value && !requestedStatus.has_value()) {
                requestedStatus = match->status;
            }
            if (match->consumedFollowing) {
                ++index;
            }
        }
        ++index;
    }
    if (invalidSyntax || !requestedStatus.has_value()) {
        return false;
    }
    status = requestedStatus.value();
    return true;
}

auto OptionParser::validateOptionNames() -> bool {
    for (const auto &optionSet : _activeOptionSets) {
        for (const auto &option : optionSet->options()) {
            if (option->isDisabled() || !isEnabledBuiltInOption(optionSet, option)) {
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
            if (option->type() == OptionType::SensitiveText && !option->maximum().isOne()) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    "Invalid option definition"_el,
                    text::StringFormat{"{} is sensitive text and must accept exactly one value."}.build(
                        OptionDisplayModel::optionTitle(option)),
                    unit::ArgumentIndex::noIndex(),
                    option);
            }
            if (option->type() == OptionType::SensitiveText && option->hasDefaultValue()) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    "Invalid option definition"_el,
                    text::StringFormat{"{} is sensitive text and cannot define a default value."}.build(
                        OptionDisplayModel::optionTitle(option)),
                    unit::ArgumentIndex::noIndex(),
                    option);
            }
            for (const auto &otherSet : _activeOptionSets) {
                for (const auto &otherOption : otherSet->options()) {
                    if (otherOption == option || otherOption->isDisabled() ||
                        !isEnabledBuiltInOption(otherSet, otherOption)) {
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

auto OptionParser::getArgAt(const unit::ArgumentIndex index) const -> text::String {
    if (!isIndexInArgs(index)) {
        return {};
    }
    return _args.get(unit::ItemIndex::fromSizeT(index.toSizeT()));
}

}
