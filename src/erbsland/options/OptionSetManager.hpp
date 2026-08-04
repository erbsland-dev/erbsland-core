// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionEditor.hpp"

#include "../text/StringEditor.hpp"

#include <initializer_list>

namespace erbsland::options {

/// A common interface for objects that can own option definitions.
///
/// `Options`, `OptionSet`, and `OptionModule` all implement this interface so option registration can use the same
/// fluent `addOption()` and `editOption()` calls.
/// @tested{OptionsFrameworkTest}
class OptionSetManager {
public:
    // defaults
    /// Create an empty option-set manager.
    OptionSetManager() = default;
    /// Destroy the option-set manager polymorphically.
    virtual ~OptionSetManager() = default;
    /// Copy the option-set manager base state.
    OptionSetManager(const OptionSetManager &) = default;
    /// Copy-assign the option-set manager base state.
    auto operator=(const OptionSetManager &) -> OptionSetManager & = default;
    /// Move the option-set manager base state.
    OptionSetManager(OptionSetManager &&) = default;
    /// Move-assign the option-set manager base state.
    auto operator=(OptionSetManager &&) -> OptionSetManager & = default;

public:
    /// Add an option with one name.
    /// @param name The command-line name, lookup alias, or positional argument name.
    /// @return An editor for the newly created option.
    virtual auto addOption(const text::String &name) -> OptionEditor;
    /// Add an option with one or more names.
    /// @param names Command-line names and lookup aliases.
    /// @return An editor for the newly created option.
    virtual auto addOption(std::initializer_list<text::String> names) -> OptionEditor = 0;
    /// Edit an existing option by name.
    /// @param name Any configured name or lookup alias of the option to edit.
    /// @return An editor for the option, or an invalid editor if no option was found.
    virtual auto editOption(const text::String &name) -> OptionEditor = 0;
};

}
