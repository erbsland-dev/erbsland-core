// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionDisplayModel.hpp"

#include "../Option.hpp"
#include "../OptionChoice.hpp"
#include "../OptionChoices.hpp"
#include "../OptionFlag.hpp"
#include "../OptionHelpVisibility.hpp"
#include "../OptionModule.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"
#include "../OptionType.hpp"

#include "../../i18n/DisplayTextMap.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringBuilder.hpp"

#include <algorithm>
#include <compare>
#include <iterator>
#include <utility>

namespace erbsland::options::impl {

using namespace text;
using namespace literals;

void OptionDisplayModel::appendMetaPlaceholder(StringBuilder &builder, const StringView &placeholder) {
    builder.append(U'<');
    builder.append(placeholder);
    builder.append(U'>');
}

auto OptionDisplayModel::optionSetTitle(const OptionSetPtr &optionSet, const i18n::DisplayTextMapConstPtr &displayText)
    -> String {
    if (optionSet != nullptr && !optionSet->help().title().isEmpty()) {
        return String{optionSet->help().title()};
    }
    return String{displayText->text("options.OptionsHeading"_el)};
}

OptionDisplayModel::OptionDisplayModel(
    OptionsPtr options, StringView moduleName, const i18n::DisplayTextMapConstPtr &displayText) :
    _options{std::move(options)},
    _displayText{displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap()} {
    _module = findModule(std::move(moduleName));
}

auto OptionDisplayModel::displayName() const -> StringView {
    if (_options == nullptr || _options->applicationInfo().applicationName().isEmpty()) {
        return _displayText->text("options.ApplicationNameFallback"_el);
    }
    return _options->applicationInfo().applicationName();
}

auto OptionDisplayModel::executableName() const -> StringView {
    if (_options == nullptr || _options->executableName().isEmpty()) {
        return displayName();
    }
    return _options->executableName();
}

auto OptionDisplayModel::helpTitleText() const -> String {
    auto builder = StringBuilder{};
    builder.append("► "_el);
    builder.append(displayName());
    builder.append(U' ');
    if (_options == nullptr) {
        builder.append("0.0.0"_el);
    } else {
        builder.append(_options->applicationInfo().applicationVersion().toString());
    }
    builder.append(" Help"_el);
    return builder.toString();
}

auto OptionDisplayModel::titleText() const -> StringView {
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
            if (!visibleOption(option, setVisibility)) {
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
            description = String{module->help().description()};
        } else if (!module->help().title().isEmpty()) {
            description = String{module->help().title()};
        }
        rows.emplace_back(OptionDisplayRow{String{module->name()}, description, {}, {}});
    }
    return rows;
}

auto OptionDisplayModel::usageOptions() const -> std::vector<OptionPtr> {
    auto result = std::vector<OptionPtr>{};
    for (const auto &optionSet : visibleOptionSets()) {
        const auto setVisibility = resolvedVisibility(optionSet->help());
        for (const auto &option : optionSet->options()) {
            if (visibleOption(option, setVisibility, true)) {
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
            if (option != nullptr && option->isPositionalArgument() && visibleOption(option, setVisibility)) {
                result.emplace_back(option);
            }
        }
    }
    return result;
}

auto OptionDisplayModel::optionTitle(const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText)
    -> String {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
    auto builder = StringBuilder{};
    auto first = true;
    if (option->isRegularOption()) {
        for (const auto &name : option->names()) {
            if (!Option::isOptionName(name)) {
                continue;
            }
            if (!first) {
                builder.append(", "_el);
            }
            builder.append(name);
            first = false;
        }
        const auto valueName = optionValueName(option, resolvedDisplayText);
        if (!valueName.isEmpty()) {
            builder.append(U' ');
            appendMetaPlaceholder(builder, valueName);
        }
        return builder.toString();
    }
    const auto valueName = positionalValueName(option, resolvedDisplayText);
    if (!valueName.isEmpty()) {
        appendMetaPlaceholder(builder, valueName);
    }
    return builder.toString();
}

auto OptionDisplayModel::optionDescription(const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText)
    -> String {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
    auto builder = StringBuilder{};
    if (isHelpOption(option)) {
        builder.append(resolvedDisplayText->text("options.HelpOptionDescription"_el));
    } else if (isVersionOption(option)) {
        builder.append(resolvedDisplayText->text("options.VersionOptionDescription"_el));
    } else if (!option->help().description().isEmpty()) {
        builder.append(option->help().description());
    } else if (!option->help().title().isEmpty()) {
        builder.append(option->help().title());
    }
    return builder.toString();
}

auto OptionDisplayModel::optionDetails(const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText)
    -> String {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
    auto builder = StringBuilder{};
    if (option->choices() != nullptr) {
        auto first = true;
        for (const auto &choice : option->choices()->choices()) {
            if (choice == nullptr || !visibleHelp(choice->help())) {
                continue;
            }
            if (first) {
                if (!builder.isEmpty()) {
                    builder.append(U' ');
                }
                builder.append(resolvedDisplayText->text("options.ChoicesLabel"_el));
                builder.append(": "_el);
            } else {
                builder.append(", "_el);
            }
            builder.append(choice->text());
            first = false;
        }
        if (!first) {
            builder.append(U'.');
        }
    }
    return builder.toString();
}

auto OptionDisplayModel::optionValueName(const OptionPtr &option, const i18n::DisplayTextMapConstPtr &displayText)
    -> StringView {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
    if (option == nullptr || option->type() == OptionType::Flag) {
        return {};
    }
    if (!option->valueName().isEmpty()) {
        return option->valueName();
    }
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
    -> StringView {
    const auto resolvedDisplayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
    if (option == nullptr) {
        return {};
    }
    if (!option->valueName().isEmpty()) {
        return option->valueName();
    }
    if (!option->names().empty()) {
        return option->names().front();
    }
    return resolvedDisplayText->text("options.ValuePlaceholder"_el);
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
    return option != nullptr && option->hasLongName("--help"_el);
}

auto OptionDisplayModel::isVersionOption(const OptionPtr &option) -> bool {
    return option != nullptr && option->hasLongName("--version"_el);
}

auto OptionDisplayModel::findModule(StringView moduleName) const -> OptionModulePtr {
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
            description = String{choice->help().description()};
        } else if (!choice->help().title().isEmpty()) {
            description = String{choice->help().title()};
        }
        if (!description.isEmpty()) {
            rows.emplace_back(OptionDisplayRow{String{choice->text()}, description, {}, {}});
        }
    }
    return rows;
}

auto OptionDisplayModel::optionSortKey(const OptionPtr &option) const -> String {
    if (option == nullptr) {
        return {};
    }
    auto result = StringView{};
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

}
