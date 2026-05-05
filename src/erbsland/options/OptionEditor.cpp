// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionEditor.hpp"

#include "Option.hpp"
#include "OptionChoices.hpp"

#include <utility>

namespace erbsland::options {

OptionEditor::OptionEditor(OptionPtr option) noexcept : _option{std::move(option)} {
}

auto OptionEditor::isValid() const noexcept -> bool {
    return _option != nullptr;
}

auto OptionEditor::setHelp(text::StringView description) -> OptionEditor & {
    if (_option != nullptr) {
        _option->setHelp(OptionHelp{std::move(description)});
    }
    return *this;
}

auto OptionEditor::setHelp(OptionHelp help) -> OptionEditor & {
    if (_option != nullptr) {
        _option->setHelp(std::move(help));
    }
    return *this;
}

auto OptionEditor::setType(const OptionType type) -> OptionEditor & {
    if (_option != nullptr) {
        _option->setType(type);
    }
    return *this;
}

auto OptionEditor::setType(const OptionType::Type type) -> OptionEditor & {
    return setType(OptionType{type});
}

auto OptionEditor::setFlags(const OptionFlags flags) -> OptionEditor & {
    if (_option != nullptr) {
        _option->setFlags(flags);
    }
    return *this;
}

auto OptionEditor::setFlag(const OptionFlag flag) -> OptionEditor & {
    if (_option != nullptr) {
        auto flags = _option->flags();
        flags.set(flag);
        _option->setFlags(flags);
    }
    return *this;
}

auto OptionEditor::clearFlag(const OptionFlag flag) -> OptionEditor & {
    if (_option != nullptr) {
        auto flags = _option->flags();
        flags.clear(flag);
        _option->setFlags(flags);
    }
    return *this;
}

auto OptionEditor::setChoices(OptionChoicesPtr choices) -> OptionEditor & {
    if (_option != nullptr) {
        if (choices != nullptr) {
            _option->setType(OptionType::Choice);
        }
        _option->setChoices(std::move(choices));
    }
    return *this;
}

auto OptionEditor::addChoice(text::StringView text) -> OptionEditor & {
    if (_option == nullptr) {
        return *this;
    }
    auto choices = _option->choices();
    if (choices == nullptr) {
        choices = OptionChoices::create();
        _option->setChoices(choices);
    }
    _option->setType(OptionType::Choice);
    choices->addChoice(std::move(text));
    return *this;
}

auto OptionEditor::setMaximum(const unit::ArgumentCount maximum) -> OptionEditor & {
    if (_option != nullptr) {
        _option->setMaximum(maximum);
    }
    return *this;
}

auto OptionEditor::setDefaultValue(OptionValueStorage defaultValue) -> OptionEditor & {
    if (_option != nullptr) {
        _option->setDefaultValue(std::move(defaultValue));
    }
    return *this;
}

auto OptionEditor::clearDefaultValue() -> OptionEditor & {
    if (_option != nullptr) {
        _option->setDefaultValue(std::nullopt);
    }
    return *this;
}

auto OptionEditor::setValidateFn(OptionValidateFn fn) -> OptionEditor & {
    if (_option != nullptr) {
        _option->setValidateFn(std::move(fn));
    }
    return *this;
}

}
