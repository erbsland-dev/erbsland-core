// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionChoices.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

auto OptionChoices::create() -> OptionChoicesPtr {
    return std::make_shared<OptionChoices>();
}

auto OptionChoices::create(std::initializer_list<text::String> choices) -> OptionChoicesPtr {
    auto result = std::make_shared<OptionChoices>();
    for (auto choice : choices) {
        result->addChoice(OptionChoice::create(std::move(choice)));
    }
    return result;
}

auto OptionChoices::addChoice(OptionChoicePtr choice) -> OptionChoices & {
    _choices.emplace_back(std::move(choice));
    return *this;
}

auto OptionChoices::addChoice(text::String text) -> OptionChoices & {
    return addChoice(OptionChoice::create(std::move(text)));
}

}
