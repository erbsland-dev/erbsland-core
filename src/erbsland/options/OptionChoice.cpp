// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionChoice.hpp"

#include "Option.hpp"

#include "impl/AsciiOptionText.hpp"

#include "../err/ParameterFailure.hpp"
#include "../i18n/DisplayText.hpp"
#include "../text/Literals.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

using namespace text;

OptionChoice::OptionChoice(String text) : _text{std::move(text)} {
}

OptionChoice::OptionChoice(String text, OptionHelp help) : _help{std::move(help)}, _text{std::move(text)} {
}

void OptionChoice::setHelpNames(std::initializer_list<String> names) {
    setHelpNames(StringList{names});
}

void OptionChoice::setHelpNames(const StringList &names) {
    auto replacement = StringList{};
    for (const auto &name : names) {
        if (!Option::isValidPositionalName(name)) {
            using namespace text::literals;
            throw err::ParameterFailure{i18n::displayText("options.invalid_choice_help_name"_el), "names"_el};
        }
        const auto canonical = impl::normalizeOptionName(name);
        if (!replacement.contains(canonical)) {
            replacement.append(canonical);
        }
    }
    _helpNames = std::move(replacement);
}

auto OptionChoice::create(String text) -> OptionChoicePtr {
    return std::make_shared<OptionChoice>(std::move(text));
}

auto OptionChoice::create(String text, OptionHelp help) -> OptionChoicePtr {
    return std::make_shared<OptionChoice>(std::move(text), std::move(help));
}

}
