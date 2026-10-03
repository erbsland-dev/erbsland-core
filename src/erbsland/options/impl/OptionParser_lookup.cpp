// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionParser.hpp"

#include "AsciiOptionText.hpp"
#include "HelpName.hpp"
#include "OptionDisplayModel.hpp"
#include "OptionLiterals.hpp"
#include "OptionSuggestion.hpp"

#include "../Option.hpp"
#include "../OptionChoices.hpp"
#include "../OptionFlag.hpp"
#include "../OptionModule.hpp"
#include "../OptionParserFlag.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"
#include "../OptionType.hpp"

#include "../../i18n/DisplayText.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringFormat.hpp"
#include "../../unit/ByteUnit.hpp"

namespace erbsland::options::impl {

using namespace text;
using namespace unit;
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

auto OptionParser::findModule(const String &name) const -> OptionModulePtr {
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

auto OptionParser::findLongOption(const String &name) const -> NameMatch {
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

auto OptionParser::findShortOption(const Char shortName) const -> NameMatch {
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

auto OptionParser::suggestLongOptions(const String &name) const -> StringList {
    auto candidates = StringList{};
    for (const auto &optionSet : _activeOptionSets) {
        if (optionSet == nullptr || optionSet->flags().isSet(OptionFlag::Disabled) ||
            optionSet->help().visibility() == OptionHelpVisibility::Hidden) {
            continue;
        }
        for (const auto &option : optionSet->options()) {
            if (option == nullptr || option->isDisabled() ||
                option->help().visibility() == OptionHelpVisibility::Hidden ||
                !isEnabledBuiltInOption(optionSet, option)) {
                continue;
            }
            for (const auto &candidate : option->names()) {
                if (Option::isLongName(candidate)) {
                    candidates.append(candidate);
                }
            }
        }
    }
    return findOptionSuggestions(name, candidates);
}

auto OptionParser::suggestModules(const String &name) const -> StringList {
    auto candidates = StringList{};
    if (_options != nullptr) {
        for (const auto &module : _options->optionModules()) {
            if (module != nullptr && module->help().visibility() != OptionHelpVisibility::Hidden) {
                candidates.append(module->name());
            }
        }
    }
    return findOptionSuggestions(name, candidates);
}

auto OptionParser::isEnabledBuiltInOption(const OptionSetPtr &optionSet, const OptionPtr &option) const -> bool {
    if (_options == nullptr || optionSet != _options->builtInOptionSet()) {
        return true;
    }
    if (option == nullptr || option->isDisabled()) {
        return false;
    }
    if (option->hasLongName(option_literal::cHelpLongName)) {
        return !_options->parserFlags().isSet(OptionParserFlag::DisableHelp);
    }
    if (option->hasLongName(option_literal::cVersionLongName)) {
        return !_options->parserFlags().isSet(OptionParserFlag::DisableVersion);
    }
    return true;
}

auto OptionParser::isEnabledBuiltInOptionName(const String &name) const -> bool {
    if (_options == nullptr) {
        return false;
    }
    const auto &optionSet = _options->builtInOptionSet();
    if (optionSet == nullptr || optionSet->flags().isSet(OptionFlag::Disabled)) {
        return false;
    }
    for (const auto &option : optionSet->options()) {
        if (!isEnabledBuiltInOption(optionSet, option)) {
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

auto OptionParser::parseBooleanLiteral(const String &text, bool &value) noexcept -> bool {
    const auto falseDefault = text.toBoolean(false);
    const auto trueDefault = text.toBoolean(true);
    if (falseDefault != trueDefault) {
        return false;
    }
    value = falseDefault;
    return true;
}

auto OptionParser::validateHelpNames() -> bool {
    for (auto index = ArgumentIndex::one(); isIndexInArgs(index); ++index) {
        const auto argument = getArgAt(index);
        if (argument == option_literal::cEndOfOptions) {
            break;
        }
        const auto match = builtInFlagAt(index);
        if (!match.has_value() || match->status != OptionResultStatus::DisplayHelp) {
            continue;
        }
        const auto parts = argument.splitAtFirst(option_literal::cValueSeparator);
        if (!parts || normalizeOptionName(parts->first) != option_literal::cHelpLongName) {
            continue;
        }
        const auto &helpName = parts->second;
        if (isValidHelpName(helpName)) {
            continue;
        }
        _omitErrorArguments = true;
        return makeError(
            OptionErrorReason::SyntaxError,
            i18n::displayText("options.malformed_help_name_title"_el),
            i18n::displayText("options.malformed_help_name_description"_el),
            index);
    }
    return true;
}

auto OptionParser::builtInFlagAt(const ArgumentIndex index) const -> std::optional<BuiltInFlagMatch> {
    const auto argument = getArgAt(index);
    const auto parts = argument.splitAtFirst(option_literal::cValueSeparator);
    auto rawName = argument;
    if (parts) {
        rawName = parts->first;
    }
    const auto name = normalizeOptionName(rawName);
    if (name != option_literal::cHelpShortName && name != option_literal::cHelpLongName &&
        name != option_literal::cVersionLongName) {
        return {};
    }
    if (!isEnabledBuiltInOptionName(name)) {
        return {};
    }

    auto result = BuiltInFlagMatch{
        name == option_literal::cVersionLongName ? OptionResultStatus::DisplayVersion : OptionResultStatus::DisplayHelp,
        {},
        true};
    if (!parts) {
        return result;
    }
    if (name == option_literal::cVersionLongName || name == option_literal::cHelpShortName ||
        (_options != nullptr && _options->parserFlags().isSet(OptionParserFlag::NoHelpDetails))) {
        result.validValue = false;
        return result;
    }
    result.helpName = normalizeHelpName(parts->second);
    return result;
}

auto OptionParser::isHelpOrVersionRequest(OptionResultStatus &status) -> bool {
    return isHelpOrVersionRequest(status, ArgumentIndex::one());
}

auto OptionParser::isHelpOrVersionRequest(OptionResultStatus &status, const ArgumentIndex startIndex) -> bool {
    auto helpCount = std::size_t{0};
    auto versionCount = std::size_t{0};
    auto requestedStatus = std::optional<OptionResultStatus>{};
    auto requestedHelpName = String{};
    auto invalidSyntax = false;
    auto index = startIndex;
    while (isIndexInArgs(index)) {
        const auto &argument = getArgAt(index);
        if (argument == option_literal::cEndOfOptions) {
            break;
        }
        if (const auto match = builtInFlagAt(index)) {
            auto &count = match->status == OptionResultStatus::DisplayHelp ? helpCount : versionCount;
            ++count;
            invalidSyntax = invalidSyntax || !match->validValue || count > 1U;
            if (match->validValue && !requestedStatus.has_value()) {
                requestedStatus = match->status;
                requestedHelpName = match->helpName;
            }
        }
        ++index;
    }
    if (invalidSyntax || !requestedStatus.has_value()) {
        return false;
    }
    status = requestedStatus.value();
    _helpName = std::move(requestedHelpName);
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
                    i18n::displayText("options.invalid_option_definition_title"_el),
                    i18n::displayFormat("options.malformed_option_names_description"_el)
                        .build(OptionDisplayModel::optionTitle(option)),
                    ArgumentIndex::noIndex(),
                    option);
            }
            if (option->isPositionalArgument() && option->type() == OptionType::Flag) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    i18n::displayText("options.invalid_option_definition_title"_el),
                    i18n::displayFormat("options.positional_flag_definition_description"_el)
                        .build(OptionDisplayModel::optionTitle(option)),
                    ArgumentIndex::noIndex(),
                    option);
            }
            if (option->type() == OptionType::Flag && option->hasDefaultValue()) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    i18n::displayText("options.invalid_option_definition_title"_el),
                    i18n::displayFormat("options.flag_default_value_description"_el)
                        .build(OptionDisplayModel::optionTitle(option)),
                    ArgumentIndex::noIndex(),
                    option);
            }
            if (option->type() != OptionType::Flag && option->flags().isSet(OptionFlag::AcceptAsFlag) &&
                (option->isPositionalArgument() || !option->maximum().isOne())) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    i18n::displayText("options.invalid_option_definition_title"_el),
                    i18n::displayFormat("options.invalid_flag_form_definition_description"_el)
                        .build(OptionDisplayModel::optionTitle(option)),
                    ArgumentIndex::noIndex(),
                    option);
            }
            if (option->choices() != nullptr && option->type() != OptionType::Choice) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    i18n::displayText("options.invalid_option_definition_title"_el),
                    i18n::displayFormat("options.choice_type_mismatch_description"_el)
                        .build(OptionDisplayModel::optionTitle(option)),
                    ArgumentIndex::noIndex(),
                    option);
            }
            if (option->type() == OptionType::Choice &&
                (option->choices() == nullptr || option->choices()->choiceCount().isZero())) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    i18n::displayText("options.invalid_option_definition_title"_el),
                    i18n::displayFormat("options.missing_choice_definitions_description"_el)
                        .build(OptionDisplayModel::optionTitle(option)),
                    ArgumentIndex::noIndex(),
                    option);
            }
            if (option->type() == OptionType::SensitiveText && !option->maximum().isOne()) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    i18n::displayText("options.invalid_option_definition_title"_el),
                    i18n::displayFormat("options.sensitive_option_definition_description"_el)
                        .build(OptionDisplayModel::optionTitle(option)),
                    ArgumentIndex::noIndex(),
                    option);
            }
            if (option->type() == OptionType::SensitiveText && option->hasDefaultValue()) {
                return makeError(
                    OptionErrorReason::SyntaxError,
                    i18n::displayText("options.invalid_option_definition_title"_el),
                    i18n::displayFormat("options.sensitive_default_value_description"_el)
                        .build(OptionDisplayModel::optionTitle(option)),
                    ArgumentIndex::noIndex(),
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
                            i18n::displayText("options.duplicate_option_name_title"_el),
                            i18n::displayFormat("options.duplicate_option_name_description"_el)
                                .build(OptionDisplayModel::optionTitle(option)),
                            ArgumentIndex::noIndex(),
                            option);
                    }
                }
            }
        }
    }
    return true;
}

auto OptionParser::isIndexInArgs(const ArgumentIndex index) const -> bool {
    return index.toSizeT() < _args.count().toSizeT();
}

auto OptionParser::getArgAt(const ArgumentIndex index) const -> String {
    if (!isIndexInArgs(index)) {
        return {};
    }
    return _args.get(ItemIndex::fromSizeT(index.toSizeT()));
}

}
