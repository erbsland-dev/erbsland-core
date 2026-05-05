// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionModule.hpp"

#include "OptionSet.hpp"

#include "impl/AsciiOptionText.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

OptionModule::OptionModule(text::StringView name) : _name{name.transformed(text::Char::toAsciiLowercase)} {
}

auto OptionModule::create() -> OptionModulePtr {
    return std::make_shared<OptionModule>();
}

auto OptionModule::create(text::StringView name) -> OptionModulePtr {
    return std::make_shared<OptionModule>(std::move(name));
}

auto OptionModule::isValidName(const text::StringView &name) noexcept -> bool {
    return impl::isAsciiNameToken(name);
}

void OptionModule::addSet(OptionSetPtr optionSet) {
    _optionSets.emplace_back(std::move(optionSet));
}

auto OptionModule::addOption(std::initializer_list<text::StringView> names) -> OptionEditor {
    return defaultOptionSet()->addOption(names);
}

auto OptionModule::editOption(const text::StringView &name) -> OptionEditor {
    for (const auto &optionSet : _optionSets) {
        auto editor = optionSet->editOption(name);
        if (editor.isValid()) {
            return editor;
        }
    }
    return {};
}

void OptionModule::setName(text::StringView name) {
    _name = name.transformed(text::Char::toAsciiLowercase);
}

auto OptionModule::hasName(const text::StringView &name) const -> bool {
    return _name.compare(name, text::Char::compareAsciiFolded) == std::strong_ordering::equal;
}

auto OptionModule::defaultOptionSet() -> OptionSetPtr {
    if (_optionSets.empty()) {
        _optionSets.emplace_back(OptionSet::create());
    }
    return _optionSets.front();
}

}
