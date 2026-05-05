// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Option_fwd.hpp"
#include "OptionCallback.hpp"
#include "OptionChoices_fwd.hpp"
#include "OptionFlag.hpp"
#include "OptionHelp.hpp"
#include "OptionType.hpp"
#include "OptionValueStorage.hpp"

#include "../text/String.hpp"
#include "../unit/ArgumentUnit.hpp"

#include <optional>

namespace erbsland::options {

/// A fluent editor for an option definition.
/// @tested{OptionsFrameworkTest}
class OptionEditor {
public:
    OptionEditor() = default;
    /// Create an editor for an option.
    explicit OptionEditor(OptionPtr option) noexcept;

    // defaults
    ~OptionEditor() = default;
    OptionEditor(const OptionEditor &) noexcept = default;
    auto operator=(const OptionEditor &) noexcept -> OptionEditor & = default;
    OptionEditor(OptionEditor &&) noexcept = default;
    auto operator=(OptionEditor &&) noexcept -> OptionEditor & = default;

public:
    /// Test if this editor has an option.
    [[nodiscard]] auto isValid() const noexcept -> bool;
    /// Access the edited option.
    [[nodiscard]] auto option() const noexcept -> const OptionPtr & { return _option; }
    /// Set the help description.
    auto setHelp(text::StringView description) -> OptionEditor &;
    /// Set the full help definition.
    auto setHelp(OptionHelp help) -> OptionEditor &;
    /// Set the option type.
    auto setType(OptionType type) -> OptionEditor &;
    /// Set the option type.
    auto setType(OptionType::Type type) -> OptionEditor &;
    /// Set all option flags.
    auto setFlags(OptionFlags flags) -> OptionEditor &;
    /// Set one option flag.
    auto setFlag(OptionFlag flag) -> OptionEditor &;
    /// Clear one option flag.
    auto clearFlag(OptionFlag flag) -> OptionEditor &;
    /// Set the choices and promote the option type to `OptionType::Choice`.
    auto setChoices(OptionChoicesPtr choices) -> OptionEditor &;
    /// Add a choice by text and promote the option type to `OptionType::Choice`.
    auto addChoice(text::StringView text) -> OptionEditor &;
    /// Set the maximum number of values.
    auto setMaximum(unit::ArgumentCount maximum) -> OptionEditor &;
    /// Set the default value.
    auto setDefaultValue(OptionValueStorage defaultValue) -> OptionEditor &;
    /// Clear the default value.
    auto clearDefaultValue() -> OptionEditor &;
    /// Set the validation callback.
    auto setValidateFn(OptionValidateFn fn) -> OptionEditor &;

private:
    OptionPtr _option; ///< The option being edited.
};

}
