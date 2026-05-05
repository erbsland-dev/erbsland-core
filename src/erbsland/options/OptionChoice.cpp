// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionChoice.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

OptionChoice::OptionChoice(text::StringView text) : _text{std::move(text)} {
}

OptionChoice::OptionChoice(text::StringView text, OptionHelp help) : _help{std::move(help)}, _text{std::move(text)} {
}

auto OptionChoice::create(text::StringView text) -> OptionChoicePtr {
    return std::make_shared<OptionChoice>(std::move(text));
}

auto OptionChoice::create(text::StringView text, OptionHelp help) -> OptionChoicePtr {
    return std::make_shared<OptionChoice>(std::move(text), std::move(help));
}

}
