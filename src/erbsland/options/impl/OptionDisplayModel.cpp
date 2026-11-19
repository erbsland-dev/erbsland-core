// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionDisplayModel.hpp"

#include "OptionLiterals.hpp"

#include "../Option.hpp"
#include "../OptionChoice.hpp"
#include "../OptionChoices.hpp"
#include "../OptionFlag.hpp"
#include "../OptionHelpVisibility.hpp"
#include "../OptionModule.hpp"
#include "../OptionParserFlag.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"
#include "../OptionType.hpp"

#include "../../i18n/DisplayText.hpp"
#include "../../i18n/impl/DisplayTextCore.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::options::impl {

using namespace text;
using namespace literals;

void OptionDisplayModel::appendMetaPlaceholder(StringEditor &result, const String &placeholder) {
    result.append(U'<');
    result.append(placeholder);
    result.append(U'>');
}

OptionDisplayModel::OptionDisplayModel(OptionsPtr options, const String &moduleName) : _options{std::move(options)} {
    _module = findModule(moduleName);
}

auto OptionDisplayModel::displayName() const -> String {
    if (_options == nullptr || _options->applicationInfo().applicationName().isEmpty()) {
        return i18n::displayText("options.application_name_fallback"_el);
    }
    return _options->applicationInfo().applicationName();
}

auto OptionDisplayModel::executableName() const -> String {
    if (_options == nullptr || _options->executableName().isEmpty()) {
        return displayName();
    }
    return _options->executableName();
}

auto OptionDisplayModel::helpTitleText() const -> String {
    const auto version =
        _options == nullptr ? String{"0.0.0"_el} : _options->applicationInfo().applicationVersion().toString();
    return i18n::displayFormat("options.help_title"_el).build(displayName(), version);
}

auto OptionDisplayModel::titleText() const -> String {
    if (_module != nullptr && !_module->help().title().isEmpty()) {
        return _module->help().title();
    }
    if (_options != nullptr && !_options->help().title().isEmpty()) {
        return _options->help().title();
    }
    if (_options != nullptr && !_options->applicationInfo().applicationName().isEmpty()) {
        return _options->applicationInfo().applicationName();
    }
    return {};
}

auto OptionDisplayModel::selectedHelp() const -> const OptionHelp * {
    if (_module != nullptr) {
        return &_module->help();
    }
    if (_options != nullptr) {
        return &_options->help();
    }
    return nullptr;
}

auto OptionDisplayModel::visibleOptionSets() const -> std::vector<VisibleOptionSet> {
    auto result = std::vector<VisibleOptionSet>{};
    if (_options == nullptr) {
        return result;
    }
    const auto addVisibleSet = [this, &result](const OptionSetPtr &optionSet, OptionHelpSorting inherited) -> void {
        if (visibleOptionSet(optionSet)) {
            auto sorting = optionSet->help().sorting();
            if (sorting == OptionHelpSorting::Inherit) {
                sorting = inherited;
            }
            result.push_back({optionSet, sorting});
        }
    };
    const auto rootPolicy = rootSorting();
    addVisibleSet(_options->builtInOptionSet(), rootPolicy);
    for (const auto &optionSet : _options->optionSets()) {
        addVisibleSet(optionSet, rootPolicy);
    }
    if (_module != nullptr) {
        const auto modulePolicy = pageSorting();
        for (const auto &optionSet : _module->optionSets()) {
            addVisibleSet(optionSet, modulePolicy);
        }
    }
    return result;
}

auto OptionDisplayModel::positionalRows() const -> std::vector<OptionDisplayRow> {
    auto rows = std::vector<OptionDisplayRow>{};
    for (const auto &option : usagePositionalOptions()) {
        rows.push_back({optionTitle(option), optionBrief(option, _options), choiceRows(option), option});
    }
    return rows;
}

auto OptionDisplayModel::usageOptions() const -> std::vector<OptionPtr> {
    auto result = std::vector<OptionPtr>{};
    for (const auto &entry : visibleOptionSets()) {
        const auto &optionSet = entry.set;
        const auto setVisibility = resolvedVisibility(optionSet->help());
        for (const auto &option : optionSet->options()) {
            if (enabledBuiltInOption(optionSet, option) && visibleOption(option, setVisibility, true)) {
                result.emplace_back(option);
            }
        }
    }
    sortOptions(result, pageSorting());
    return result;
}

auto OptionDisplayModel::usagePositionalOptions() const -> std::vector<OptionPtr> {
    auto result = std::vector<OptionPtr>{};
    if (_module == nullptr && _options != nullptr && !_options->optionModules().empty()) {
        return result;
    }
    for (const auto &entry : visibleOptionSets()) {
        const auto &optionSet = entry.set;
        const auto setVisibility = resolvedVisibility(optionSet->help());
        for (const auto &option : optionSet->options()) {
            if (enabledBuiltInOption(optionSet, option) && option != nullptr && option->isPositionalArgument() &&
                visibleOption(option, setVisibility)) {
                result.emplace_back(option);
            }
        }
    }
    return result;
}

auto OptionDisplayModel::optionTitle(const OptionPtr &option) -> String {
    auto result = StringEditor{};
    auto first = true;
    if (option->isRegularOption()) {
        for (const auto &name : option->names()) {
            if (!Option::isOptionName(name)) {
                continue;
            }
            if (!first) {
                result.append(", "_el);
            }
            result.append(name);
            first = false;
        }
        const auto valueName = optionValueName(option);
        if (!valueName.isEmpty()) {
            result.append(U' ');
            appendMetaPlaceholder(result, valueName);
        }
        return result;
    }
    const auto valueName = positionalValueName(option);
    if (!valueName.isEmpty()) {
        appendMetaPlaceholder(result, valueName);
    }
    return result;
}

auto OptionDisplayModel::displayAliases(const Option &option) -> StringList {
    auto aliases = StringList{};
    for (const auto &name : option.names()) {
        if (Option::isShortName(name)) {
            aliases.append(name);
        }
    }
    for (const auto &name : option.names()) {
        if (Option::isLongName(name)) {
            aliases.append(name);
        }
    }
    return aliases;
}

auto OptionDisplayModel::optionBrief(const OptionPtr &option, const OptionsPtr &options) -> String {
    return optionHelpText(option, options, true);
}

auto OptionDisplayModel::optionDescription(const OptionPtr &option, const OptionsPtr &options) -> String {
    return optionHelpText(option, options, false);
}

auto OptionDisplayModel::optionHelpText(const OptionPtr &option, const OptionsPtr &options, const bool brief)
    -> String {
    if (option == nullptr) {
        return {};
    }
    const auto &authored = brief ? option->help().title() : option->help().description();
    if (options == nullptr || options->builtInOptionSet() == nullptr ||
        std::ranges::find(options->builtInOptionSet()->options(), option) ==
            options->builtInOptionSet()->options().end()) {
        return authored;
    }
    auto key = String{};
    if (isHelpOption(option)) {
        key = brief ? "options.help_option_description"_el : "options.help_option_details"_el;
    } else if (isVersionOption(option)) {
        key = brief ? "options.version_option_description"_el : "options.version_option_details"_el;
    }
    if (!key.isEmpty() && authored == i18n::impl::DisplayTextCore{}.findText(key).value()) {
        return i18n::displayText(key);
    }
    return authored;
}

auto OptionDisplayModel::optionDetails(const OptionPtr &option) -> String {
    auto result = StringEditor{};
    if (option->choices() != nullptr) {
        auto first = true;
        for (const auto &choice : option->choices()->choices()) {
            if (choice == nullptr || !visibleHelp(choice->help())) {
                continue;
            }
            if (first) {
                if (!result.isEmpty()) {
                    result.append(U' ');
                }
                result.append(i18n::displayText("options.choices_label"_el));
                result.append(": "_el);
            } else {
                result.append(", "_el);
            }
            result.append(choice->text());
            first = false;
        }
        if (!first) {
            result.append(U'.');
        }
    }
    return result;
}

auto OptionDisplayModel::optionValueName(const OptionPtr &option) -> String {
    if (option == nullptr || option->type() == OptionType::Flag) {
        return {};
    }
    if (!option->valueName().isEmpty()) {
        return option->valueName();
    }

    if (isHelpOption(option)) {
        return i18n::displayText("options.help_keyword_placeholder"_el);
    }
    if (option->type() == OptionType::Boolean) {
        return i18n::displayText("options.boolean_placeholder"_el);
    }
    if (option->type() == OptionType::Integer) {
        return i18n::displayText("options.integer_placeholder"_el);
    }
    if (option->type() == OptionType::Text || option->type() == OptionType::SensitiveText) {
        return i18n::displayText("options.value_placeholder"_el);
    }
    if (option->type() == OptionType::Choice || option->choices() != nullptr) {
        return i18n::displayText("options.choice_placeholder"_el);
    }
    return {};
}

auto OptionDisplayModel::positionalValueName(const OptionPtr &option) -> String {
    if (option == nullptr) {
        return {};
    }
    if (!option->valueName().isEmpty()) {
        return option->valueName();
    }
    if (!option->names().isEmpty()) {
        return option->names().first();
    }
    return i18n::displayText("options.value_placeholder"_el);
}

auto OptionDisplayModel::resolvedVisibility(const OptionHelp &help, const OptionHelpVisibility inherited) noexcept
    -> OptionHelpVisibility {
    if (help.visibility() == OptionHelpVisibility::Inherit) {
        return inherited == OptionHelpVisibility::Inherit ? OptionHelpVisibility::Normal : inherited;
    }
    return help.visibility();
}

auto OptionDisplayModel::visibleHelp(const OptionHelp &help) noexcept -> bool {
    return resolvedVisibility(help) != OptionHelpVisibility::Hidden;
}

auto OptionDisplayModel::isHelpOption(const OptionPtr &option) -> bool {
    return option != nullptr && option->hasLongName(option_literal::cHelpLongName);
}

auto OptionDisplayModel::isVersionOption(const OptionPtr &option) -> bool {
    return option != nullptr && option->type() == OptionType::Flag &&
        option->hasLongName(option_literal::cVersionLongName);
}

auto OptionDisplayModel::findModule(const String &moduleName) const -> OptionModulePtr {
    if (_options == nullptr || moduleName.isEmpty()) {
        return {};
    }
    for (const auto &module : _options->optionModules()) {
        if (module != nullptr && module->hasName(moduleName)) {
            return module;
        }
    }
    return {};
}

auto OptionDisplayModel::hasModules() const noexcept -> bool {
    return _options != nullptr && !_options->optionModules().empty();
}

auto OptionDisplayModel::visibleOptionSet(const OptionSetPtr &optionSet) const noexcept -> bool {
    return optionSet != nullptr && !optionSet->flags().isSet(OptionFlag::Disabled) &&
        resolvedVisibility(optionSet->help()) != OptionHelpVisibility::Hidden;
}

auto OptionDisplayModel::visibleOption(
    const OptionPtr &option, const OptionHelpVisibility setVisibility, const bool forUsage) const noexcept -> bool {
    if (option == nullptr || option->isDisabled()) {
        return false;
    }
    const auto visibility = resolvedVisibility(option->help(), setVisibility);
    if (forUsage) {
        return option->isRegularOption() && visibility == OptionHelpVisibility::Usage;
    }
    if (visibility == OptionHelpVisibility::Hidden) {
        return false;
    }
    if (_module == nullptr && hasModules()) {
        return visibility == OptionHelpVisibility::Overview || visibility == OptionHelpVisibility::Usage;
    }
    return true;
}

auto OptionDisplayModel::enabledBuiltInOption(const OptionSetPtr &optionSet, const OptionPtr &option) const noexcept
    -> bool {
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

auto OptionDisplayModel::visibleChoice(
    const OptionHelp &help, const OptionHelpVisibility optionVisibility) const noexcept -> bool {
    return resolvedVisibility(help, optionVisibility) != OptionHelpVisibility::Hidden;
}

auto OptionDisplayModel::visibleModule(const OptionHelp &help) const noexcept -> bool {
    return resolvedVisibility(help) != OptionHelpVisibility::Hidden;
}

auto OptionDisplayModel::choiceRows(const OptionPtr &option) const -> std::vector<OptionDisplayRow> {
    auto rows = std::vector<OptionDisplayRow>{};
    if (option == nullptr || option->choices() == nullptr) {
        return rows;
    }
    const auto optionVisibility = resolvedVisibility(option->help());
    for (const auto &choice : option->choices()->choices()) {
        if (choice == nullptr || !visibleChoice(choice->help(), optionVisibility)) {
            continue;
        }
        const auto &description = choice->help().title();
        if (!description.isEmpty()) {
            rows.emplace_back(OptionDisplayRow{choice->text(), description, {}, {}});
        }
    }
    return rows;
}

}
