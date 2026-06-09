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

#include "../../text/Literals.hpp"
#include "../../text/StringBuilder.hpp"

#include <utility>

namespace erbsland::options::impl {

using namespace text::literals;

OptionDisplayModel::OptionDisplayModel(OptionsPtr options, text::StringView moduleName, OptionDisplayText displayText) :
    _options{std::move(options)}, _displayText{std::move(displayText)} {
    _module = findModule(moduleName);
}

auto OptionDisplayModel::displayName() const -> text::StringView {
    if (_options == nullptr || _options->applicationInfo().applicationName().isEmpty()) {
        return _displayText.applicationNameFallback();
    }
    return _options->applicationInfo().applicationName();
}

auto OptionDisplayModel::executableName() const -> text::StringView {
    if (_options == nullptr || _options->executableName().isEmpty()) {
        return displayName();
    }
    return _options->executableName();
}

auto OptionDisplayModel::helpTitleText() const -> text::String {
    auto builder = text::StringBuilder{};
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

auto OptionDisplayModel::titleText() const -> text::StringView {
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
    for (const auto &optionSet : visibleOptionSets()) {
        for (const auto &option : optionSet->options()) {
            if (option == nullptr || option->isDisabled() || !visibleHelp(option->help())) {
                continue;
            }
            rows.emplace_back(
                OptionDisplayRow{
                    optionTitle(option, _displayText), optionDescription(option, _displayText), choiceRows(option)});
        }
    }
    return rows;
}

auto OptionDisplayModel::moduleRows() const -> std::vector<OptionDisplayRow> {
    auto rows = std::vector<OptionDisplayRow>{};
    if (_options == nullptr) {
        return rows;
    }
    for (const auto &module : _options->optionModules()) {
        if (module == nullptr || !visibleHelp(module->help())) {
            continue;
        }
        auto description = text::String{};
        if (!module->help().description().isEmpty()) {
            description = text::String{module->help().description()};
        } else if (!module->help().title().isEmpty()) {
            description = text::String{module->help().title()};
        }
        rows.emplace_back(OptionDisplayRow{text::String{module->name()}, description, {}});
    }
    return rows;
}

auto OptionDisplayModel::optionTitle(const OptionPtr &option, const OptionDisplayText &displayText) -> text::String {
    auto builder = text::StringBuilder{};
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
        if (option->type() == OptionType::Integer) {
            builder.append(U' ');
            builder.append(displayText.integerPlaceholder());
        } else if (option->type() == OptionType::Text) {
            builder.append(U' ');
            builder.append(displayText.valuePlaceholder());
        } else if (option->type() == OptionType::Choice) {
            builder.append(U' ');
            builder.append(displayText.choicePlaceholder());
        }
        return builder.toString();
    }
    if (!option->names().empty()) {
        builder.append(displayText.placeholderPrefix());
        builder.append(option->names().front());
        builder.append(displayText.placeholderSuffix());
    } else {
        builder.append(displayText.valuePlaceholder());
    }
    return builder.toString();
}

auto OptionDisplayModel::optionDescription(const OptionPtr &option, const OptionDisplayText &displayText)
    -> text::String {
    auto builder = text::StringBuilder{};
    if (isHelpOption(option)) {
        builder.append(displayText.helpOptionDescription());
    } else if (isVersionOption(option)) {
        builder.append(displayText.versionOptionDescription());
    } else if (!option->help().description().isEmpty()) {
        builder.append(option->help().description());
    } else if (!option->help().title().isEmpty()) {
        builder.append(option->help().title());
    }
    if (option->type() == OptionType::Choice && option->choices() != nullptr) {
        auto first = true;
        for (const auto &choice : option->choices()->choices()) {
            if (choice == nullptr || !visibleHelp(choice->help())) {
                continue;
            }
            if (first) {
                if (!builder.isEmpty()) {
                    builder.append(U' ');
                }
                builder.append(displayText.choicesLabel());
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

auto OptionDisplayModel::visibleHelp(const OptionHelp &help) noexcept -> bool {
    return help.visibility() != OptionHelpVisibility::Hidden && help.visibility() != OptionHelpVisibility::Detail;
}

auto OptionDisplayModel::isHelpOption(const OptionPtr &option) -> bool {
    return option != nullptr && option->hasLongName("--help"_el);
}

auto OptionDisplayModel::isVersionOption(const OptionPtr &option) -> bool {
    return option != nullptr && option->hasLongName("--version"_el);
}

auto OptionDisplayModel::findModule(text::StringView moduleName) const -> OptionModulePtr {
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

auto OptionDisplayModel::visibleOptionSet(const OptionSetPtr &optionSet) const noexcept -> bool {
    return optionSet != nullptr && !optionSet->flags().isSet(OptionFlag::Disabled) && visibleHelp(optionSet->help());
}

auto OptionDisplayModel::choiceRows(const OptionPtr &option) const -> std::vector<OptionDisplayRow> {
    auto rows = std::vector<OptionDisplayRow>{};
    if (option == nullptr || option->choices() == nullptr) {
        return rows;
    }
    for (const auto &choice : option->choices()->choices()) {
        if (choice == nullptr || !visibleHelp(choice->help())) {
            continue;
        }
        auto description = text::String{};
        if (!choice->help().description().isEmpty()) {
            description = text::String{choice->help().description()};
        } else if (!choice->help().title().isEmpty()) {
            description = text::String{choice->help().title()};
        }
        if (!description.isEmpty()) {
            rows.emplace_back(OptionDisplayRow{text::String{choice->text()}, description, {}});
        }
    }
    return rows;
}

}
