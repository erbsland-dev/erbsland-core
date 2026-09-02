// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionChoiceEditor.hpp"

#include "OptionChoice.hpp"

#include <utility>

namespace erbsland::options {

OptionChoiceEditor::OptionChoiceEditor(OptionChoicePtr choice) noexcept : _choice{std::move(choice)} {
}

auto OptionChoiceEditor::isValid() const noexcept -> bool {
    return _choice != nullptr;
}

auto OptionChoiceEditor::setHelp(text::String description) -> OptionChoiceEditor & {
    if (_choice != nullptr) {
        _choice->setHelp(OptionHelp{std::move(description)});
    }
    return *this;
}

auto OptionChoiceEditor::setHelp(OptionHelp help) -> OptionChoiceEditor & {
    if (_choice != nullptr) {
        _choice->setHelp(std::move(help));
    }
    return *this;
}

auto OptionChoiceEditor::setHelpTitle(text::String title) -> OptionChoiceEditor & {
    if (_choice != nullptr) {
        _choice->setHelpTitle(std::move(title));
    }
    return *this;
}

auto OptionChoiceEditor::setHelpDescription(text::String description) -> OptionChoiceEditor & {
    if (_choice != nullptr) {
        _choice->setHelpDescription(std::move(description));
    }
    return *this;
}

auto OptionChoiceEditor::setHelpEpilog(text::String epilog) -> OptionChoiceEditor & {
    if (_choice != nullptr) {
        _choice->setHelpEpilog(std::move(epilog));
    }
    return *this;
}

auto OptionChoiceEditor::setHelpExample(text::String example) -> OptionChoiceEditor & {
    if (_choice != nullptr) {
        _choice->setHelpExample(std::move(example));
    }
    return *this;
}

auto OptionChoiceEditor::setHelpVisibility(const OptionHelpVisibility visibility) -> OptionChoiceEditor & {
    if (_choice != nullptr) {
        _choice->setHelpVisibility(visibility);
    }
    return *this;
}

}
