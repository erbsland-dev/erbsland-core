// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionDisplayModel.hpp"

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

#include "../../i18n/DisplayTextMap.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>
#include <compare>
#include <iterator>
#include <utility>

namespace erbsland::options::impl {

using namespace text;
using namespace literals;

void OptionDisplayModel::appendMetaPlaceholder(StringEditor &result, const String &placeholder) {
    result.append(U'<');
    result.append(placeholder);
    result.append(U'>');
}

auto OptionDisplayModel::optionSetTitle(const OptionSetPtr &optionSet, const i18n::DisplayTextMapConstPtr &displayText)
    -> String {
    if (optionSet != nullptr && !optionSet->help().title().isEmpty()) {
        return optionSet->help().title();
    }
    return displayText->text("options.OptionsHeading"_el);
}

OptionDisplayModel::OptionDisplayModel(
    OptionsPtr options, const String &moduleName, const i18n::DisplayTextMapConstPtr &displayText) :
    _options{std::move(options)}, _displayText{resolveDisplayText(displayText)} {
    _module = findModule(moduleName);
}

auto OptionDisplayModel::displayName() const -> String {
    if (_options == nullptr || _options->applicationInfo().applicationName().isEmpty()) {
        return _displayText->text("options.ApplicationNameFallback"_el);
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
    return String::fromJoined({"► "_el, displayName(), " "_el, version, " Help"_el});
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

auto OptionDisplayModel::visibleOptionSets() const -> std::vector<OptionSetPtr> {
    auto result = std::vector<OptionSetPtr>{};
    if (_options == nullptr) {
        return result;
    }
    const auto addVisibleSet = [this, &result](const OptionSetPtr &optionSet) -> void {
        if (visibleOptionSet(optionSet)) {
            result.emplace_back(optionSet);
        }
    };
    addVisibleSet(_options->builtInOptionSet());
    for (const auto &optionSet : _options->optionSets()) {
        addVisibleSet(optionSet);
    }
    if (_module != nullptr) {
        for (const auto &optionSet : _module->optionSets()) {
            addVisibleSet(optionSet);
        }
    }
    return result;
}

auto OptionDisplayModel::optionRows() const -> std::vector<OptionDisplayRow> {
    auto rows = std::vector<OptionDisplayRow>{};
    for (const auto &group : optionGroups()) {
        for (const auto &row : group.rows) {
            rows.emplace_back(row);
        }
    }
    return rows;
}

auto OptionDisplayModel::optionGroups() const -> std::vector<OptionDisplayGroup> {
    auto groups = std::vector<OptionDisplayGroup>{};
    for (const auto &optionSet : visibleOptionSets()) {
        const auto setVisibility = resolvedVisibility(optionSet->help());
        auto title = optionSetTitle(optionSet, _displayText);
        auto groupIterator = std::ranges::find_if(
            groups, [&title](const OptionDisplayGroup &group) -> bool { return group.title == title; });
        if (groupIterator == groups.end()) {
            groups.emplace_back(OptionDisplayGroup{title, {}});
            groupIterator = std::prev(groups.end());
        }
        for (const auto &option : optionSet->options()) {
            if (!enabledBuiltInOption(optionSet, option) || !visibleOption(option, setVisibility)) {
                continue;
            }
            groupIterator->rows.emplace_back(
                OptionDisplayRow{
                    optionTitle(option, _displayText),
                    optionDescription(option, _displayText),
                    choiceRows(option),
                    option});
        }
    }
    for (auto &group : groups) {
        std::ranges::stable_sort(
            group.rows, [this](const OptionDisplayRow &left, const OptionDisplayRow &right) -> bool {
                return optionSortKey(left.option.lock()).compare(optionSortKey(right.option.lock())) ==
                    std::strong_ordering::less;
            });
    }
    std::erase_if(groups, [](const OptionDisplayGroup &group) -> bool { return group.rows.empty(); });
    return groups;
}

auto OptionDisplayModel::moduleRows() const -> std::vector<OptionDisplayRow> {
    auto rows = std::vector<OptionDisplayRow>{};
    if (_options == nullptr) {
        return rows;
    }
    for (const auto &module : _options->optionModules()) {
        if (module == nullptr || !visibleModule(module->help())) {
            continue;
        }
        auto description = String{};
        if (!module->help().description().isEmpty()) {
            description = module->help().description();
        } else if (!module->help().title().isEmpty()) {
            description = module->help().title();
        }
        rows.emplace_back(OptionDisplayRow{module->name(), description, {}, {}});
    }
    return rows;
}

auto OptionDisplayModel::usageOptions() const -> std::vector<OptionPtr> {
    auto result = std::vector<OptionPtr>{};
    for (const auto &optionSet : visibleOptionSets()) {
        const auto setVisibility = resolvedVisibility(optionSet->help());
        for (const auto &option : optionSet->options()) {
            if (enabledBuiltInOption(optionSet, option) && visibleOption(option, setVisibility, true)) {
                result.emplace_back(option);
            }
        }
    }
    std::ranges::stable_sort(result, [this](const OptionPtr &left, const OptionPtr &right) -> bool {
        return optionSortKey(left).compare(optionSortKey(right)) == std::strong_ordering::less;
    });
    return result;
}

auto OptionDisplayModel::usagePositionalOptions() const -> std::vector<OptionPtr> {
    auto result = std::vector<OptionPtr>{};
    if (_module == nullptr && _options != nullptr && !_options->optionModules().empty()) {
        return result;
    }
    for (const auto &optionSet : visibleOptionSets()) {
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

auto OptionDisplayModel::optionTitle(const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText)
    -> String {
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
        const auto valueName = optionValueName(option, displayText);
        if (!valueName.isEmpty()) {
            result.append(U' ');
            appendMetaPlaceholder(result, valueName);
        }
        return result;
    }
    const auto valueName = positionalValueName(option, displayText);
    if (!valueName.isEmpty()) {
        appendMetaPlaceholder(result, valueName);
    }
    return result;
}

auto OptionDisplayModel::optionDescription(const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText)
    -> String {
    const auto resolvedDisplayText = resolveDisplayText(displayText);
    if (isHelpOption(option)) {
        return resolvedDisplayText->text("options.HelpOptionDescription"_el);
    }
    if (isVersionOption(option)) {
        return resolvedDisplayText->text("options.VersionOptionDescription"_el);
    }
    if (!option->help().description().isEmpty()) {
        return option->help().description();
    }
    return option->help().title();
}

auto OptionDisplayModel::optionDetails(const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText)
    -> String {
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
                result.append(resolveDisplayText(displayText)->text("options.ChoicesLabel"_el));
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

auto OptionDisplayModel::optionValueName(const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText)
    -> String {
    if (option == nullptr || option->type() == OptionType::Flag) {
        return {};
    }
    if (!option->valueName().isEmpty()) {
        return option->valueName();
    }
    const auto resolvedDisplayText = resolveDisplayText(displayText);
    if (option->type() == OptionType::Integer) {
        return resolvedDisplayText->text("options.IntegerPlaceholder"_el);
    }
    if (option->type() == OptionType::Text) {
        return resolvedDisplayText->text("options.ValuePlaceholder"_el);
    }
    if (option->type() == OptionType::Choice || option->choices() != nullptr) {
        return resolvedDisplayText->text("options.ChoicePlaceholder"_el);
    }
    return {};
}

auto OptionDisplayModel::positionalValueName(const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText)
    -> String {
    if (option == nullptr) {
        return {};
    }
    if (!option->valueName().isEmpty()) {
        return option->valueName();
    }
    if (!option->names().empty()) {
        return option->names().front();
    }
    return resolveDisplayText(displayText)->text("options.ValuePlaceholder"_el);
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
    return option != nullptr && option->type() == OptionType::Flag && option->hasLongName("--help"_el);
}

auto OptionDisplayModel::isVersionOption(const OptionPtr &option) -> bool {
    return option != nullptr && option->type() == OptionType::Flag && option->hasLongName("--version"_el);
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
    if (option->hasLongName("--help"_el)) {
        return !_options->parserFlags().isSet(OptionParserFlag::DisableHelp);
    }
    if (option->hasLongName("--version"_el)) {
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
        auto description = String{};
        if (!choice->help().description().isEmpty()) {
            description = choice->help().description();
        } else if (!choice->help().title().isEmpty()) {
            description = choice->help().title();
        }
        if (!description.isEmpty()) {
            rows.emplace_back(OptionDisplayRow{choice->text(), description, {}, {}});
        }
    }
    return rows;
}

auto OptionDisplayModel::optionSortKey(const OptionPtr &option) const -> String {
    if (option == nullptr) {
        return {};
    }
    auto result = String{};
    for (const auto &name : option->names()) {
        if (Option::isLongName(name)) {
            result = name.slice({unit::CpIndex{2}, unit::CpLength::infinite()});
            break;
        }
        if (result.isEmpty() && Option::isShortName(name)) {
            result = name.slice(StringSide::Back, unit::CpIndex{1U});
        } else if (result.isEmpty() && Option::isPositionalName(name)) {
            result = name;
        }
    }
    return result.transformed(Char::toAsciiLowercase);
}

auto OptionDisplayModel::resolveDisplayText(const i18n::DisplayTextMapConstPtr &displayText) noexcept
    -> i18n::DisplayTextMapConstPtr {
    if (displayText == nullptr) {
        return i18n::DisplayTextMap::defaultMap();
    }
    return displayText;
}

}
