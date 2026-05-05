// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionHelpVisibility.hpp"

#include "../text/String.hpp"

#include <utility>

namespace erbsland::options {

/// Help visibility and text for options, sets, modules, and choices.
/// A help renderer can use this data to render help text.
/// @tested{OptionsFrameworkTest}
class OptionHelp {
public:
    OptionHelp() = default;
    /// Create help text from a description.
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
    void setVisibility(const OptionHelpVisibility visibility) noexcept { _visibility = visibility; }
    /// Get the title.
    [[nodiscard]] auto title() const noexcept -> const text::StringView & { return _title; }
    /// Set the title.
    void setTitle(text::StringView title) { _title = std::move(title); }
    /// Get the main description.
    [[nodiscard]] auto description() const noexcept -> const text::StringView & { return _description; }
    /// Set the main description.
    void setDescription(text::StringView description) { _description = std::move(description); }
    /// Get the epilog text.
    [[nodiscard]] auto epilog() const noexcept -> const text::StringView & { return _epilog; }
    /// Set the epilog text.
    void setEpilog(text::StringView epilog) { _epilog = std::move(epilog); }

private:
    OptionHelpVisibility _visibility{OptionHelpVisibility::Inherit}; ///< How visible this item is in help.
    text::StringView _title;                                         ///< An optional title.
    text::StringView _description;                                   ///< The main help text.
    text::StringView _epilog;                                        ///< Optional trailing text.
};

}
