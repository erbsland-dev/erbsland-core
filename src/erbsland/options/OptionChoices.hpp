// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionChoice.hpp"
#include "OptionChoices_fwd.hpp"

#include "../unit/ArgumentUnit.hpp"

#include <vector>

namespace erbsland::options {

/// A collection of accepted option choices.
///
/// Assign a collection to an option with `setChoices()` or use `OptionEditor::addChoice()` for simple definitions.
/// @tested{OptionsFrameworkTest}
class OptionChoices {
public:
    OptionChoices() = default;

    // defaults
    ~OptionChoices() = default;
    OptionChoices(const OptionChoices &) = default;
    auto operator=(const OptionChoices &) -> OptionChoices & = default;
    OptionChoices(OptionChoices &&) = default;
    auto operator=(OptionChoices &&) -> OptionChoices & = default;

public:
    /// Create an empty shared choice collection.
    /// @return A shared choice collection without choices.
    [[nodiscard]] static auto create() -> OptionChoicesPtr;
    /// Create a shared choice collection from text values.
    /// @param choices Choice texts to add in declaration order.
    /// @return A shared choice collection containing one choice for each text.
    [[nodiscard]] static auto create(std::initializer_list<text::StringView> choices) -> OptionChoicesPtr;
    /// Add a choice.
    /// @param choice Choice object to append.
    /// @return This collection for chaining.
    auto addChoice(OptionChoicePtr choice) -> OptionChoices &;
    /// Add a choice by text.
    /// @param text Choice text to append.
    /// @return This collection for chaining.
    auto addChoice(text::StringView text) -> OptionChoices &;

public: // accessors
    /// Get all choices.
    [[nodiscard]] auto choices() const noexcept -> const std::vector<OptionChoicePtr> & { return _choices; }
    /// Get the number of choices.
    /// @return The number of configured choices.
    [[nodiscard]] auto choiceCount() const noexcept -> unit::ArgumentCount {
        return unit::ArgumentCount::fromSizeT(_choices.size());
    }

private:
    std::vector<OptionChoicePtr> _choices; ///< The accepted choices.
};

}
