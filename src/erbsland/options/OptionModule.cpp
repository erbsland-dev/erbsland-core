// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionModule.hpp"

#include "OptionSet.hpp"

#include "impl/AsciiOptionText.hpp"

#include "../err/ParameterFailure.hpp"
#include "../i18n/DisplayText.hpp"
#include "../text/Literals.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

using namespace text;
using namespace text::literals;

OptionModule::OptionModule(const String &name) {
    setName(name);
}

auto OptionModule::create() -> OptionModulePtr {
    return std::make_shared<OptionModule>();
}

auto OptionModule::create(const String &name) -> OptionModulePtr {
    return std::make_shared<OptionModule>(name);
}

auto OptionModule::isValidName(const String &name) noexcept -> bool {
    return impl::isAsciiNameToken(name);
}

void OptionModule::addSet(OptionSetPtr optionSet) {
    _optionSets.emplace_back(std::move(optionSet));
}

auto OptionModule::addOption(std::initializer_list<String> names) -> OptionEditor {
    return defaultOptionSet()->addOption(names);
}

auto OptionModule::editOption(const String &name) -> OptionEditor {
    for (const auto &optionSet : _optionSets) {
        auto editor = optionSet->editOption(name);
        if (editor.isValid()) {
            return editor;
        }
    }
    return {};
}

void OptionModule::setName(const String &name) {
    if (!isValidName(name)) {
        throw err::ParameterFailure{i18n::displayText("options.invalid_module_definition_name"_el), "name"_el};
    }
    _name = impl::normalizeOptionName(name);
}

auto OptionModule::hasName(const String &name) const -> bool {
    return _name == impl::normalizeOptionName(name);
}

auto OptionModule::defaultOptionSet() -> OptionSetPtr {
    if (_optionSets.empty()) {
        _optionSets.emplace_back(OptionSet::create());
    }
    return _optionSets.front();
}

}
