// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionChoice_fwd.hpp"
#include "OptionHelp.hpp"

#include "../text/StringList.hpp"

#include <initializer_list>
#include <utility>

namespace erbsland::options {

/// A single accepted choice for an option.
///
/// Choice matching is case-insensitive, but the stored parsed value uses the configured choice text.
/// @tested{OptionsFrameworkTest OptionHelpDocumentTest}
class OptionChoice {
public:
    /// Create an empty option choice.
    OptionChoice() = default;
    /// Create a choice with the given text.
    /// @param text Choice text accepted on the command line.
    explicit OptionChoice(text::String text);
    /// Create a choice with the given text and help.
    /// @param text Choice text accepted on the command line.
    /// @param help Help metadata for generated details.
    OptionChoice(text::String text, OptionHelp help);

    // defaults
    ~OptionChoice() = default;
    OptionChoice(const OptionChoice &) = default;
    auto operator=(const OptionChoice &) -> OptionChoice & = default;
    OptionChoice(OptionChoice &&) = default;
    auto operator=(OptionChoice &&) -> OptionChoice & = default;

public:
    /// Create a shared choice.
    /// @param text Choice text accepted on the command line.
    /// @return A shared choice object.
    [[nodiscard]] static auto create(text::String text) -> OptionChoicePtr;
    /// Create a shared choice with help.
    /// @param text Choice text accepted on the command line.
    /// @param help Help metadata for generated details.
    /// @return A shared choice object.
    [[nodiscard]] static auto create(text::String text, OptionHelp help) -> OptionChoicePtr;

public: // accessors
    /// Get the help text.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp & { return _help; }
    /// Set the help text.
    /// @param help Replacement help metadata.
    void setHelp(OptionHelp help) { _help = std::move(help); }
    /// Set the help title for this choice.
    /// @param title Brief text shown in overview output.
    void setHelpTitle(text::String title) { _help.setTitle(std::move(title)); }
    /// Set the help description for this choice.
    /// @param description Detailed description shown on the choice help page.
    void setHelpDescription(text::String description) { _help.setDescription(std::move(description)); }
    /// Set the help epilog for this choice.
    /// @param epilog Optional trailing text for renderers that support choice-level epilogs.
    void setHelpEpilog(text::String epilog) { _help.setEpilog(std::move(epilog)); }
    /// Set the detailed-help example for this choice.
    /// @param example Short usage example for detailed help.
    void setHelpExample(text::String example) { _help.setExample(std::move(example)); }
    /// Set the help visibility for this choice.
    /// @param visibility Controls where this choice appears in generated help output.
    void setHelpVisibility(const OptionHelpVisibility visibility) noexcept { _help.setVisibility(visibility); }
    /// Get the choice text.
    [[nodiscard]] auto text() const noexcept -> const text::String & { return _text; }
    /// Set the choice text.
    /// @param text Replacement choice text.
    void setText(text::String text) { _text = std::move(text); }

    /// Get explicitly configured detailed-help aliases; an empty list enables the value-name fallback.
    [[nodiscard]] auto helpNames() const noexcept -> const text::StringList & { return _helpNames; }
    /// Replace the detailed-help aliases with normalized bare ASCII names.
    /// @param names Bare ASCII aliases satisfying `Option::isValidPositionalName()`, each at most 100 characters; empty
    /// restores the value-name fallback.
    void setHelpNames(const text::StringList &names);
    /// Replace the detailed-help aliases from a list of bare names.
    /// @param names Bare ASCII aliases satisfying `Option::isValidPositionalName()`, each at most 100 characters,
    /// independent of the accepted choice value.
    void setHelpNames(std::initializer_list<text::String> names);

private:
    text::StringList _helpNames; ///< Explicit aliases independent of the accepted choice value.
    OptionHelp _help;            ///< The help text for the choice.
    text::String _text;          ///< The text of this choice.
};

}
