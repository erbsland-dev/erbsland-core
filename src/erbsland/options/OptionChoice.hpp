// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionChoice_fwd.hpp"
#include "OptionHelp.hpp"

#include <utility>

namespace erbsland::options {

/// A single accepted choice for an option.
/// @tested{OptionsFrameworkTest}
class OptionChoice {
public:
    OptionChoice() = default;
    /// Create a choice with the given text.
    explicit OptionChoice(text::StringView text);
    /// Create a choice with the given text and help.
    OptionChoice(text::StringView text, OptionHelp help);

    // defaults
    ~OptionChoice() = default;
    OptionChoice(const OptionChoice &) = default;
    auto operator=(const OptionChoice &) -> OptionChoice & = default;
    OptionChoice(OptionChoice &&) = default;
    auto operator=(OptionChoice &&) -> OptionChoice & = default;

public:
    /// Create a shared choice.
    [[nodiscard]] static auto create(text::StringView text) -> OptionChoicePtr;
    /// Create a shared choice with help.
    [[nodiscard]] static auto create(text::StringView text, OptionHelp help) -> OptionChoicePtr;

public: // accessors
    /// Get the help text.
    [[nodiscard]] auto help() const noexcept -> const OptionHelp & { return _help; }
    /// Set the help text.
    void setHelp(OptionHelp help) { _help = std::move(help); }
    /// Get the choice text.
    [[nodiscard]] auto text() const noexcept -> const text::StringView & { return _text; }
    /// Set the choice text.
    void setText(text::StringView text) { _text = std::move(text); }

private:
    OptionHelp _help;       ///< The help text for the choice.
    text::StringView _text; ///< The text of this choice.
};

}
