// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Options.hpp"

#include "OptionDisplayText.hpp"
#include "OptionModule.hpp"
#include "OptionSet.hpp"

#include "impl/ExecutableName.hpp"

#include "../text/Literals.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

using namespace text::literals;

Options::Options() : _builtInOptionSet{createBuiltInOptionSet()} {
}

auto Options::create() -> OptionsPtr {
    return std::make_shared<Options>();
}

void Options::addSet(OptionSetPtr optionSet) {
    _optionSets.emplace_back(std::move(optionSet));
}

void Options::addModule(OptionModulePtr optionModule) {
    _optionModules.emplace_back(std::move(optionModule));
}

void Options::setExecutablePath(text::StringView executablePath) {
    _executablePath = std::move(executablePath);
    _executableName = impl::extractExecutableName(_executablePath);
}

auto Options::addOption(std::initializer_list<text::StringView> names) -> OptionEditor {
    return defaultOptionSet()->addOption(names);
}

auto Options::editOption(const text::StringView &name) -> OptionEditor {
    for (const auto &optionSet : _optionSets) {
        auto editor = optionSet->editOption(name);
        if (editor.isValid()) {
            return editor;
        }
    }
    return {};
}

auto Options::defaultOptionSet() -> OptionSetPtr {
    if (_optionSets.empty()) {
        _optionSets.emplace_back(OptionSet::create());
    }
    return _optionSets.front();
}

auto Options::createBuiltInOptionSet() -> OptionSetPtr {
    const auto &displayText = OptionDisplayText::defaultText();
    auto optionSet = OptionSet::create();
    optionSet->addOption({"-h"_el, "--help"_el}).setHelp(displayText.helpOptionDescription());
    optionSet->addOption("--version"_el).setHelp(displayText.versionOptionDescription());
    return optionSet;
}

}
