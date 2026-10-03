// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionHelpVisibility.hpp"

#include "../text/String.hpp"

#include <utility>

namespace erbsland::options {

/// Help visibility and text for options, sets, modules, choices, and the root options object.
/// This object stores display metadata only; parser behavior is controlled by option names, types, flags, and
/// callbacks.
/// @tested{OptionsFrameworkTest OptionHelpDocumentTest}
class OptionHelp {
public:
    /// Create empty option help metadata.
    OptionHelp() = default;
    /// Create help metadata with a brief title.
    /// @param title Brief text for overview rows, or a section heading for an option set.
    explicit OptionHelp(text::String title);

    // defaults
    ~OptionHelp() = default;
    OptionHelp(const OptionHelp &) = default;
    auto operator=(const OptionHelp &) -> OptionHelp & = default;
    OptionHelp(OptionHelp &&) = default;
    auto operator=(OptionHelp &&) -> OptionHelp & = default;

public: // accessors
    /// Get the visibility of this help text.
    [[nodiscard]] auto visibility() const noexcept -> OptionHelpVisibility { return _visibility; }
    /// Set the visibility of this help text.
    /// @param visibility Controls where this item appears in generated help output.
    void setVisibility(const OptionHelpVisibility visibility) noexcept { _visibility = visibility; }
    /// Get the title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _title; }
    /// Set the title.
    /// @param title Brief overview text for options, modules and choices, or the section heading for an option set.
    void setTitle(text::String title) { _title = std::move(title); }
    /// Get the main description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    /// Set the main description.
    /// @param description Detailed help text, or introductory text below an option-set heading or in root help.
    void setDescription(text::String description) { _description = std::move(description); }
    /// Get the epilog text.
    [[nodiscard]] auto epilog() const noexcept -> const text::String & { return _epilog; }
    /// Set the epilog text.
    /// @param epilog Optional trailing text rendered after root, module or detailed help output.
    void setEpilog(text::String epilog) { _epilog = std::move(epilog); }
    /// Get the optional example text.
    [[nodiscard]] auto example() const noexcept -> const text::String & { return _example; }
    /// Set the optional example text.
    /// @param example Short usage example for detailed help.
    void setExample(text::String example) { _example = std::move(example); }

private:
    OptionHelpVisibility _visibility{OptionHelpVisibility::Inherit}; ///< How visible this item is in help.
    text::String _title;                                             ///< An optional title.
    text::String _description;                                       ///< The main help text.
    text::String _epilog;                                            ///< Optional trailing text.
    text::String _example;                                           ///< Optional detailed-help example.
};

}
