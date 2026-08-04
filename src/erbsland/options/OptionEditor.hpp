// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Option.hpp"
#include "Option_fwd.hpp"
#include "OptionCallback.hpp"
#include "OptionChoices_fwd.hpp"
#include "OptionFlag.hpp"
#include "OptionHelp.hpp"
#include "OptionType.hpp"
#include "OptionValueStorage.hpp"

#include "../text/StringEditor.hpp"
#include "../unit/ArgumentUnit.hpp"

#include <optional>

namespace erbsland::options {

/// A fluent editor for an option definition.
/// Editors are returned by `addOption()` and `editOption()` so option definitions can be configured in one expression.
/// @tested{OptionsFrameworkTest}
class OptionEditor {
public:
    /// Create an empty option editor.
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
    /// @return `true` if mutating calls will be applied to an option.
    [[nodiscard]] auto isValid() const noexcept -> bool;
    /// Access the edited option.
    /// @return The shared option edited by this object, or `nullptr` for an invalid editor.
    [[nodiscard]] auto option() const noexcept -> const OptionPtr & { return _option; }
    /// Set the help description.
    /// @param description User-facing help description for this option.
    /// @return This editor for chaining.
    auto setHelp(text::String description) -> OptionEditor &;
    /// Set the full help definition.
    /// @param help Complete help metadata.
    /// @return This editor for chaining.
    auto setHelp(OptionHelp help) -> OptionEditor &;
    /// Set the help title.
    /// @param title Short title used when no description is available.
    /// @return This editor for chaining.
    auto setHelpTitle(text::String title) -> OptionEditor &;
    /// Set the help description.
    /// @param description User-facing help description for this option.
    /// @return This editor for chaining.
    auto setHelpDescription(text::String description) -> OptionEditor &;
    /// Set the help epilog.
    /// @param epilog Optional trailing text for renderers that support option-level epilogs.
    /// @return This editor for chaining.
    auto setHelpEpilog(text::String epilog) -> OptionEditor &;
    /// Set the help visibility.
    /// @param visibility Controls where this option appears in generated help output.
    /// @return This editor for chaining.
    auto setHelpVisibility(OptionHelpVisibility visibility) -> OptionEditor &;
    /// Set the custom value name shown in help output.
    /// @param valueName Bare value name without angle brackets. Empty restores the type-derived default.
    /// @return This editor for chaining.
    auto setValueName(text::String valueName) -> OptionEditor &;
    /// Set the option type.
    /// @param type The expected command-line value type.
    /// @return This editor for chaining.
    auto setType(OptionType type) -> OptionEditor &;
    /// Set the option type.
    auto setType(OptionType::Type type) -> OptionEditor &;
    /// Set all option flags.
    /// @param flags Replacement flags for the option.
    /// @return This editor for chaining.
    auto setFlags(OptionFlags flags) -> OptionEditor &;
    /// Set one option flag.
    auto setFlag(OptionFlag flag) -> OptionEditor &;
    /// Clear one option flag.
    auto clearFlag(OptionFlag flag) -> OptionEditor &;
    /// Set the choices and promote the option type to `OptionType::Choice`.
    auto setChoices(OptionChoicesPtr choices) -> OptionEditor &;
    /// Add a choice by text and promote the option type to `OptionType::Choice`.
    auto addChoice(text::String text) -> OptionEditor &;
    /// Set the maximum number of values.
    auto setMaximum(unit::ArgumentCount maximum) -> OptionEditor &;
    /// Set the default value.
    auto setDefaultValue(OptionValueStorage defaultValue) -> OptionEditor &;
    /// Clear the default value.
    auto clearDefaultValue() -> OptionEditor &;
    /// Set the validation callback.
    /// @param fn The new value validation callback `(OptionValuePtr valueToValidate, OptionValuesPtr values) -> void`
    ///    The callback must throw an `el::OptionError` on failure.
    auto setValidateFn(OptionValidateFn fn) -> OptionEditor &;
    /// Setup a value validation for a type that parses text.
    /// The submitted type must have one of these methods:
    /// - a static `fromStringOrThrow(String)` method that throws an `err::ParseError` on failure.
    /// - a static `isValidString(String)` method that returns `false` on failure.
    /// - a static `fromString(String)` method that returns `std::nullopt` on failure.
    /// The methods are used in in the order shown above.
    /// @tparam tValueType The type to validate.
    /// @param errorTitle The error title to use in the error message.
    template <typename tValueType>
    auto setValidateTextValue(text::String errorTitle = {}) -> OptionEditor &;

public: // convenience methods
    /// Set the option as required.
    auto setRequired() -> OptionEditor &;

private:
    OptionPtr _option; ///< The option being edited.
};

}

#include "OptionEditor.tpp"
