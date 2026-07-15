// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionEditor.hpp"

#include "../text/String.hpp"

#include <initializer_list>

namespace erbsland::options {

/// A common interface for objects that can own option definitions.
///
/// `Options`, `OptionSet`, and `OptionModule` all implement this interface so option registration can use the same
/// fluent `addOption()` and `editOption()` calls.
/// @tested{OptionsFrameworkTest}
class OptionSetManager {
public:
    OptionSetManager() = default;
    virtual ~OptionSetManager() = default;
    OptionSetManager(const OptionSetManager &) = default;
    auto operator=(const OptionSetManager &) -> OptionSetManager & = default;
    OptionSetManager(OptionSetManager &&) = default;
    auto operator=(OptionSetManager &&) -> OptionSetManager & = default;

public:
    /// Add an option with one name.
    /// @param name The command-line name, lookup alias, or positional argument name.
    /// @return An editor for the newly created option.
    virtual auto addOption(const text::StringView &name) -> OptionEditor;
    /// Add an option with one or more names.
    /// @param names Command-line names and lookup aliases.
    /// @return An editor for the newly created option.
    virtual auto addOption(std::initializer_list<text::StringView> names) -> OptionEditor = 0;
    /// Edit an existing option by name.
    /// @param name Any configured name or lookup alias of the option to edit.
    /// @return An editor for the option, or an invalid editor if no option was found.
    virtual auto editOption(const text::StringView &name) -> OptionEditor = 0;
};

}
