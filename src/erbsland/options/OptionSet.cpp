// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionSet.hpp"

#include "Option.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

auto OptionSet::create() -> OptionSetPtr {
    return std::make_shared<OptionSet>();
}

void OptionSet::addOption(OptionPtr option) {
    _options.emplace_back(std::move(option));
}

auto OptionSet::addOption(std::initializer_list<text::StringView> names) -> OptionEditor {
    auto option = Option::create(names);
    addOption(option);
    return OptionEditor{option};
}

auto OptionSet::editOption(const text::StringView &name) -> OptionEditor {
    return OptionEditor{findOption(name)};
}

auto OptionSet::findOption(const text::StringView &name) const -> OptionPtr {
    for (const auto &option : _options) {
        for (const auto &optionName : option->names()) {
            if (optionName == name) {
                return option;
            }
        }
    }
    return {};
}

}
