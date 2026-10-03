// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionHelpTarget.hpp"

#include "OptionLiterals.hpp"

#include "../Option.hpp"
#include "../OptionChoice.hpp"
#include "../OptionModule.hpp"

namespace erbsland::options::impl {

auto OptionHelpTarget::help() const noexcept -> const OptionHelp & {
    if (isModule()) {
        return module->help();
    }
    if (isChoice()) {
        return choice->help();
    }
    return option->help();
}

auto OptionHelpTarget::commandName() const -> text::String {
    if (isModule()) {
        return module->name();
    }
    if (isChoice()) {
        for (const auto &name : names) {
            if (name.contains(option_literal::cHelpPathSeparator)) {
                return name;
            }
        }
        return {};
    }
    for (const auto &name : option->names()) {
        if (Option::isLongName(name)) {
            return name;
        }
    }
    for (const auto &name : option->names()) {
        if (Option::isShortName(name)) {
            return name;
        }
    }
    return names.first();
}

auto OptionHelpTarget::sameTarget(const OptionHelpTarget &other) const noexcept -> bool {
    return module == other.module && option == other.option && choice == other.choice;
}

}
