// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionHelpSearch.hpp"

#include "AsciiOptionText.hpp"
#include "HelpName.hpp"
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

#include "../../text/fuzzy/Matcher.hpp"
#include "../../text/StringEditor.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::options::impl {

using namespace text;
using namespace unit;

OptionHelpSearch::OptionHelpSearch(OptionsPtr options, const String &moduleName) : _options{std::move(options)} {
    if (_options == nullptr) {
        return;
    }
    auto selected = OptionModulePtr{};
    for (const auto &module : _options->optionModules()) {
        if (module != nullptr && module->hasName(moduleName)) {
            selected = module;
            break;
        }
    }
    appendModule(selected);
    appendSet(_options->builtInOptionSet(), {});
    for (const auto &set : _options->optionSets()) {
        appendSet(set, {});
    }
    for (const auto &module : _options->optionModules()) {
        if (module != selected) {
            appendModule(module);
        }
    }
}

void OptionHelpSearch::appendModule(const OptionModulePtr &module) {
    if (module == nullptr || module->help().visibility() == OptionHelpVisibility::Hidden) {
        return;
    }
    appendTarget({module, {}, {}, StringList{module->name()}});
    for (const auto &set : module->optionSets()) {
        appendSet(set, module);
    }
}

void OptionHelpSearch::appendSet(const OptionSetPtr &set, const OptionModulePtr &module) {
    if (set == nullptr || set->flags().isSet(OptionFlag::Disabled) ||
        set->help().visibility() == OptionHelpVisibility::Hidden) {
        return;
    }
    for (const auto &option : set->options()) {
        if (option == nullptr || option->isDisabled() || !option->isRegularOption() ||
            option->help().visibility() == OptionHelpVisibility::Hidden) {
            continue;
        }
        if (set == _options->builtInOptionSet()) {
            if ((option->hasLongName(option_literal::cHelpLongName) &&
                    _options->parserFlags().isSet(OptionParserFlag::DisableHelp)) ||
                (option->hasLongName(option_literal::cVersionLongName) &&
                    _options->parserFlags().isSet(OptionParserFlag::DisableVersion))) {
                continue;
            }
        }
        auto names = StringList{};
        for (const auto &name : option->names()) {
            names.append(name);
        }
        appendTarget({module, option, {}, std::move(names)});
        if (option->choices() == nullptr) {
            continue;
        }
        for (const auto &choice : option->choices()->choices()) {
            if (choice == nullptr || choice->help().visibility() == OptionHelpVisibility::Hidden) {
                continue;
            }
            auto aliases = choiceNames(option, choice);
            if (!aliases.isEmpty()) {
                appendTarget({module, option, choice, std::move(aliases)});
            }
        }
    }
}

void OptionHelpSearch::appendTarget(OptionHelpTarget target) {
    if (std::ranges::none_of(
            _targets, [&target](const OptionHelpTarget &existing) -> bool { return existing.sameTarget(target); })) {
        _targets.emplace_back(std::move(target));
    }
}

auto OptionHelpSearch::choiceNames(const OptionPtr &option, const OptionChoicePtr &choice) -> StringList {
    auto bare = choice->helpNames();
    if (bare.isEmpty() && Option::isValidPositionalName(choice->text())) {
        bare.append(normalizeOptionName(choice->text()));
    }
    auto names = bare;
    // Prefer a long option name for commands; all aliases still work for lookup.
    auto owner = OptionHelpTarget{{}, option, {}, {}};
    for (const auto &alias : bare) {
        auto qualified = StringEditor{owner.commandName()};
        qualified.append(option_literal::cHelpPathSeparator).append(alias);
        names.append(qualified);
        for (const auto &optionName : option->names()) {
            auto alternative = StringEditor{optionName};
            alternative.append(option_literal::cHelpPathSeparator).append(alias);
            if (!names.contains(alternative)) {
                names.append(alternative);
            }
        }
    }
    return names;
}

auto OptionHelpSearch::exactMatches(const String &name) const -> std::vector<OptionHelpTarget> {
    const auto canonical = normalizeHelpName(name);
    auto matches = std::vector<OptionHelpTarget>{};
    for (const auto &target : _targets) {
        if (target.names.contains(canonical)) {
            matches.emplace_back(target);
        }
    }
    return matches;
}

auto OptionHelpSearch::partialMatches(const String &name) const -> std::vector<OptionHelpTarget> {
    const auto canonical = normalizeHelpName(name);
    const auto length = canonical.characterLength().toSizeT();
    auto maximumDistance = CpCount{3U};
    if (length <= 4U) {
        maximumDistance = CpCount{1U};
    } else if (length <= 8U) {
        maximumDistance = CpCount{2U};
    }
    auto matcher = fuzzy::Matcher{canonical}.setMaximumDistance(maximumDistance).setMaximumResults(ItemCount::one());
    auto ranked = std::vector<std::pair<CpCount, OptionHelpTarget>>{};
    for (const auto &target : _targets) {
        auto candidates = StringList{};
        for (const auto &alias : target.names) {
            if (!Option::isValidShortName(alias)) {
                candidates.append(alias);
            }
        }
        const auto matches = matcher.findMatches(candidates);
        if (!matches.isEmpty()) {
            ranked.emplace_back(matches.first().distance(), target);
        }
    }
    std::ranges::stable_sort(
        ranked,
        [](const std::pair<CpCount, OptionHelpTarget> &left,
            const std::pair<CpCount, OptionHelpTarget> &right) -> bool { return left.first < right.first; });
    auto result = std::vector<OptionHelpTarget>{};
    for (const auto &[distance, target] : ranked) {
        result.emplace_back(target);
    }
    if (!result.empty()) {
        return result;
    }
    for (const auto &target : _targets) {
        if (std::ranges::any_of(target.names, [&canonical](const String &alias) -> bool {
                return !Option::isValidShortName(alias) && alias.startsWith(canonical);
            })) {
            result.emplace_back(target);
        }
    }
    return result;
}

auto OptionHelpSearch::requiresResults(const std::vector<OptionHelpTarget> &matches) noexcept -> bool {
    if (matches.empty()) {
        return false;
    }
    const auto &first = matches.front();
    for (const auto &target : matches) {
        if (target.isModule() != first.isModule() || target.isChoice() != first.isChoice()) {
            return true;
        }
        if (target.isChoice() && std::ranges::any_of(matches, [&target](const OptionHelpTarget &other) -> bool {
                return other.module == target.module && !other.sameTarget(target);
            })) {
            return true;
        }
    }
    return false;
}

}
