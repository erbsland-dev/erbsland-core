// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionDisplayModel.hpp"

#include "../Option.hpp"
#include "../OptionHelpSorting.hpp"
#include "../OptionModule.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"

#include "../../i18n/DisplayText.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringTransformations.hpp"

#include <algorithm>
#include <iterator>
#include <tuple>
#include <utility>

namespace erbsland::options::impl {

using namespace text;
using namespace literals;

auto OptionDisplayModel::optionGroups() const -> std::vector<OptionDisplayGroup> {
    struct Group {
        OptionDisplayGroup display;
        std::vector<OptionPtr> options;
        OptionHelpSorting sorting;
        int weight;
        bool isDefault = false;
        OptionHelpSorting firstExplicitSorting = OptionHelpSorting::Inherit;
    };
    auto groups = std::vector<Group>{};
    const auto inheritedSorting = OptionHelpSorting::Inherit;
    const auto defaultTitle = i18n::displayText("options.options_heading"_el);
    for (const auto &[optionSet, sorting] : visibleOptionSets()) {
        const auto &help = optionSet->help();
        const auto setVisibility = resolvedVisibility(help);
        auto visible = std::vector<OptionPtr>{};
        for (const auto &option : optionSet->options()) {
            if (enabledBuiltInOption(optionSet, option) && visibleOption(option, setVisibility) &&
                option->isRegularOption()) {
                visible.push_back(option);
            }
        }
        if (visible.empty()) {
            continue;
        }
        const auto isDefault = help.title().isEmpty();
        auto title = help.title();
        if (isDefault) {
            title = defaultTitle;
        }
        auto groupIterator =
            std::ranges::find_if(groups, [&title](const Group &group) -> bool { return group.display.title == title; });
        if (groupIterator == groups.end()) {
            // The first visible contributor defines sorting and weight for a named section.
            groups.push_back({{std::move(title), {}, {}}, {}, sorting, help.weight()});
            groupIterator = std::prev(groups.end());
        }
        auto &group = *groupIterator;
        group.isDefault = group.isDefault || isDefault;
        if (group.firstExplicitSorting == inheritedSorting) {
            group.firstExplicitSorting = help.sorting();
        }
        const auto &description = help.description();
        if (!description.isEmpty() && !group.display.descriptions.contains(description)) {
            group.display.descriptions.append(description);
        }
        group.options.insert(group.options.end(), visible.begin(), visible.end());
    }
    const auto pagePolicy = pageSorting();
    if (pagePolicy != OptionHelpSorting::None) {
        std::ranges::stable_sort(groups, {}, [pagePolicy](const Group &group) {
            auto weight = 0;
            if (pagePolicy == OptionHelpSorting::ByWeight) {
                weight = group.weight;
            }
            return std::tuple{!group.isDefault, weight, toCaseFolded(group.display.title)};
        });
    }
    auto result = std::vector<OptionDisplayGroup>{};
    result.reserve(groups.size());
    for (auto &group : groups) {
        auto sorting = group.sorting;
        if (group.isDefault) {
            sorting = pagePolicy;
            if (group.firstExplicitSorting != inheritedSorting) {
                sorting = group.firstExplicitSorting;
            }
        }
        sortOptions(group.options, sorting);
        group.display.rows.reserve(group.options.size());
        for (const auto &option : group.options) {
            group.display.rows.push_back(
                {optionTitle(option), optionBrief(option, _options), choiceRows(option), option});
        }
        result.push_back(std::move(group.display));
    }
    return result;
}

auto OptionDisplayModel::moduleRows() const -> std::vector<OptionDisplayRow> {
    auto rows = std::vector<OptionDisplayRow>{};
    if (_options == nullptr) {
        return rows;
    }
    auto modules = std::vector<OptionModulePtr>{};
    for (const auto &module : _options->optionModules()) {
        if (module != nullptr && visibleModule(module->help())) {
            modules.push_back(module);
        }
    }
    const auto sorting = rootSorting();
    if (sorting != OptionHelpSorting::None) {
        std::ranges::stable_sort(modules, {}, [sorting](const OptionModulePtr &module) {
            auto weight = 0;
            if (sorting == OptionHelpSorting::ByWeight) {
                weight = module->help().weight();
            }
            return std::pair{weight, module->name()};
        });
    }
    rows.reserve(modules.size());
    for (const auto &module : modules) {
        rows.emplace_back(OptionDisplayRow{module->name(), module->help().title(), {}, {}});
    }
    return rows;
}

auto OptionDisplayModel::rootSorting() const noexcept -> OptionHelpSorting {
    if (_options != nullptr && _options->help().sorting() != OptionHelpSorting::Inherit) {
        return _options->help().sorting();
    }
    return OptionHelpSorting::ByLongName;
}

auto OptionDisplayModel::pageSorting() const noexcept -> OptionHelpSorting {
    if (_module != nullptr && _module->help().sorting() != OptionHelpSorting::Inherit) {
        return _module->help().sorting();
    }
    return rootSorting();
}

void OptionDisplayModel::sortOptions(std::vector<OptionPtr> &options, const OptionHelpSorting sorting) {
    if (sorting == OptionHelpSorting::None) {
        return;
    }
    std::ranges::stable_sort(options, {}, [sorting](const OptionPtr &option) {
        auto longName = String{};
        auto shortName = String{};
        for (const auto &name : option->names()) {
            if (longName.isEmpty() && Option::isLongName(name)) {
                longName = name.slice({unit::CpIndex{2}, unit::CpCount::infinite()});
            } else if (shortName.isEmpty() && Option::isShortName(name)) {
                shortName = toAsciiLowercase(name.slice(StringSide::Back, unit::CpIndex{1U}));
            }
            if (!longName.isEmpty() && !shortName.isEmpty()) {
                break;
            }
        }
        if (sorting == OptionHelpSorting::ByShortName) {
            return std::tuple{static_cast<int>(shortName.isEmpty()), shortName, longName};
        }
        auto weight = 0;
        if (sorting == OptionHelpSorting::ByWeight) {
            weight = option->help().weight();
        }
        if (longName.isEmpty()) {
            return std::tuple{weight, shortName, String{}};
        }
        return std::tuple{weight, longName, String{}};
    });
}

}
