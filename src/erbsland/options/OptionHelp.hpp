// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionHelpVisibility.hpp"

#include "../text/StringView.hpp"

#include <utility>

namespace erbsland::options {

/// Help visibility and text for options, sets, modules, choices, and the root options object.
/// This object stores display metadata only; parser behavior is controlled by option names, types, flags, and
/// callbacks.
/// @tested{OptionsFrameworkTest}
class OptionHelp {
public:
    OptionHelp() = default;
    /// Create help text from a description.
    /// @param description User-facing description text for generated help output.
    explicit OptionHelp(text::StringView description);

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
    [[nodiscard]] auto title() const noexcept -> const text::StringView & { return _title; }
    /// Set the title.
    /// @param title Short title, used for option-set groups and as fallback text when no description is available.
    void setTitle(text::StringView title) { _title = std::move(title); }
    /// Get the main description.
    [[nodiscard]] auto description() const noexcept -> const text::StringView & { return _description; }
    /// Set the main description.
    /// @param description Main user-facing help text.
    void setDescription(text::StringView description) { _description = std::move(description); }
    /// Get the epilog text.
    [[nodiscard]] auto epilog() const noexcept -> const text::StringView & { return _epilog; }
    /// Set the epilog text.
    /// @param epilog Optional trailing help text rendered after root or module help output.
    void setEpilog(text::StringView epilog) { _epilog = std::move(epilog); }

private:
    OptionHelpVisibility _visibility{OptionHelpVisibility::Inherit}; ///< How visible this item is in help.
    text::StringView _title;                                         ///< An optional title.
    text::StringView _description;                                   ///< The main help text.
    text::StringView _epilog;                                        ///< Optional trailing text.
};

}
