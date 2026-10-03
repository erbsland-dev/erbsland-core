// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EnvironmentChanges_fwd.hpp"
#include "EnvironmentInherit.hpp"

#include "impl/environment/EnvironmentChangesProcessor_fwd.hpp"

#include "../path/Path.hpp"
#include "../text/String.hpp"
#include "../text/StringList.hpp"
#include "../text/StringMap.hpp"
#include "../util/List.hpp"

#include <optional>
#include <utility>

namespace erbsland::system {

/// Describes inheritance, variable changes, and portable PATH edits without reading the environment.
/// @seedoc{/reference/system/system_services}
/// @tested{EnvironmentChangesTest EnvironmentSubprocessInteropTest}
class EnvironmentChanges final {
    friend class impl::EnvironmentChangesProcessor;

public: // tests
    /// Test whether application inherits every variable without changing anything.
    [[nodiscard]] auto isUnchangedInheritance() const noexcept -> bool;

public: // inheritance
    /// Replace the inheritance selection with all or no base variables.
    auto setInherit(EnvironmentInherit inherit) -> EnvironmentChanges &;
    /// Inherit only the named base variables; missing variables are ignored.
    /// @param names The replacement selection; an empty list inherits nothing.
    auto setInherit(text::StringList names) -> EnvironmentChanges &;

public: // variable changes
    /// Assign a variable after inheritance, replacing any pending assignment or removal.
    /// @param name A non-empty valid UTF-8 name without '=' or null characters.
    /// @param value A valid UTF-8 value without null characters; may be empty.
    auto setVariable(const text::String &name, text::String value) -> EnvironmentChanges &;
    /// Remove a variable after inheritance, replacing any pending assignment.
    /// @param name The variable name.
    auto removeVariable(const text::String &name) -> EnvironmentChanges &;
    /// Remove the named variables after inheritance.
    /// @param names The variable names.
    auto removeVariable(const text::StringList &names) -> EnvironmentChanges &;
    /// Cancel a variable's pending assignment or removal, restoring inheritance behavior.
    /// @param name The variable name.
    auto clearVariableChange(const text::String &name) -> EnvironmentChanges &;
    /// Clear variable actions while preserving inheritance and PATH rules.
    auto clearVariableChanges() noexcept -> EnvironmentChanges &;

public: // path changes
    /// Insert a path at the front, ahead of earlier prepends.
    /// @param path A valid path without the native PATH separator or null characters.
    auto prependPath(path::Path path) -> EnvironmentChanges &;
    /// Insert a path at the back, after earlier appends.
    /// @param path A valid path without the native PATH separator or null characters.
    auto appendPath(path::Path path) -> EnvironmentChanges &;
    /// Remove every lexically equivalent path before inserting additions.
    /// @param path The path to remove.
    auto removePath(path::Path path) -> EnvironmentChanges &;
    /// Remove a base directory and its descendants before inserting additions.
    /// @param base The base directory; matching is lexical and component-based.
    auto removePathsIn(path::Path base) -> EnvironmentChanges &;
    /// Clear PATH rules while preserving inheritance and variable actions.
    auto clearPathChanges() noexcept -> EnvironmentChanges &;

public: // application
    /// Return a new environment by applying this descriptor to a supplied base.
    /// @param base The source environment; this method never reads or changes the process environment.
    /// @return The resulting environment using the current platform's name and PATH rules.
    [[nodiscard]] auto apply(const text::StringMap<text::String> &base) const -> text::StringMap<text::String>;

private:
    /// Validate an explicitly configured name.
    static void validateName(const text::String &name);
    /// Validate an explicitly configured value.
    static void validateValue(const text::String &value);
    /// Validate an explicitly configured PATH argument.
    static void validatePath(const path::Path &path);
    /// Erase the pending action for a previously validated variable name.
    void eraseVariableChange(const text::String &name);
    /// Test if any PATH rules were configured.
    [[nodiscard]] auto hasPathChanges() const noexcept -> bool;

private:
    EnvironmentInherit _inherit{EnvironmentInherit::All}; ///< Base mode when no named selection is present.
    std::optional<text::StringList> _inheritedNames;      ///< Optional named inheritance selection.
    util::List<std::pair<text::String, std::optional<text::String>>> _variables; ///< Last action for each name.
    path::PathList _frontPaths;                                                  ///< Prepends in reverse call order.
    path::PathList _backPaths;                                                   ///< Appends in call order.
    path::PathList _removedPaths;                                                ///< Exact lexical removals.
    path::PathList _removedBases;                                                ///< Base and descendant removals.
};

}
