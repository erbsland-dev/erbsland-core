// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionChoice_fwd.hpp"
#include "OptionHelp.hpp"

namespace erbsland::options {

/// A fluent editor for an option choice definition.
/// Editors are returned by `OptionChoices::addChoice()` so choice help can be configured in one expression.
/// @tested{OptionsFrameworkTest}
class OptionChoiceEditor {
public:
    /// Create an empty option choice editor.
    OptionChoiceEditor() = default;
    /// Create an editor for an option choice.
    explicit OptionChoiceEditor(OptionChoicePtr choice) noexcept;

    // defaults
    ~OptionChoiceEditor() = default;
    OptionChoiceEditor(const OptionChoiceEditor &) noexcept = default;
    auto operator=(const OptionChoiceEditor &) noexcept -> OptionChoiceEditor & = default;
    OptionChoiceEditor(OptionChoiceEditor &&) noexcept = default;
    auto operator=(OptionChoiceEditor &&) noexcept -> OptionChoiceEditor & = default;

public:
    /// Test if this editor has an option choice.
    /// @return `true` if mutating calls will be applied to an option choice.
    [[nodiscard]] auto isValid() const noexcept -> bool;
    /// Access the edited option choice.
    /// @return The shared option choice edited by this object, or `nullptr` for an invalid editor.
    [[nodiscard]] auto choice() const noexcept -> const OptionChoicePtr & { return _choice; }
    /// Set the help description.
    /// @param description User-facing help description for this choice.
    /// @return This editor for chaining.
    auto setHelp(text::String description) -> OptionChoiceEditor &;
    /// Set the full help definition.
    /// @param help Complete help metadata.
    /// @return This editor for chaining.
    auto setHelp(OptionHelp help) -> OptionChoiceEditor &;
    /// Set the help title.
    /// @param title Short title used when no description is available.
    /// @return This editor for chaining.
    auto setHelpTitle(text::String title) -> OptionChoiceEditor &;
    /// Set the help description.
    /// @param description User-facing help description for this choice.
    /// @return This editor for chaining.
    auto setHelpDescription(text::String description) -> OptionChoiceEditor &;
    /// Set the help epilog.
    /// @param epilog Optional trailing text for renderers that support choice-level epilogs.
    /// @return This editor for chaining.
    auto setHelpEpilog(text::String epilog) -> OptionChoiceEditor &;
    /// Set the detailed-help example.
    /// @param example Short usage example for detailed help.
    /// @return This editor for chaining.
    auto setHelpExample(text::String example) -> OptionChoiceEditor &;
    /// Set the help visibility.
    /// @param visibility Controls where this choice appears in generated help output.
    /// @return This editor for chaining.
    auto setHelpVisibility(OptionHelpVisibility visibility) -> OptionChoiceEditor &;

private:
    OptionChoicePtr _choice; ///< The option choice being edited.
};

}
