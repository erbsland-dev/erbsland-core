// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EnvironmentVariableBackend.hpp"

#include "../../../text/StringList.hpp"
#include "../../../util/List.hpp"

#include <functional>
#include <optional>

namespace erbsland::system::impl {

/// Parsed PATH entries with their original spelling and platform-aware comparison keys.
/// @tested{EnvironmentPathsTest EnvironmentChangesTest}
class EnvironmentPaths final {
public:
    /// Resolve an environment variable in the environment being edited.
    using Lookup = EnvironmentVariableBackend::Lookup;

    /// Create an initially empty PATH list.
    /// @param backend Platform parsing and comparison rules.
    /// @param lookup Lookup used to expand Windows percent-delimited variables.
    EnvironmentPaths(const EnvironmentVariableBackend &backend, Lookup lookup);

public: // parsing and output
    /// Append a native entry, skipping malformed entries and lexical duplicates.
    /// @param spelling Original native spelling, including optional Windows quotes and placeholders.
    void appendEntry(const text::String &spelling);
    /// Read all entries from an existing PATH value.
    /// @param value Native PATH value; an empty value denotes the current directory.
    void read(const text::String &value);
    /// Get normalized paths in search order.
    [[nodiscard]] auto paths() const -> path::PathList;
    /// Join valid entries, preserving their original spelling.
    [[nodiscard]] auto text() const -> text::String;

public: // editing
    /// Insert a path at an index, removing any equivalent existing entry first.
    /// @param index Insertion position in the original list, including its end.
    /// @param path The path to insert.
    void insert(unit::ItemIndex index, const path::Path &path);
    /// Append a path unless an equivalent entry already exists.
    /// @param path The path to append.
    void append(const path::Path &path);
    /// Remove equivalent paths, optionally including descendants.
    /// @param path The path to remove.
    /// @param descendants Also remove paths below this directory.
    void remove(const path::Path &path, bool descendants = false);
    /// Validate an explicit path argument under a platform policy.
    /// @param path The caller-supplied path.
    /// @param backend Platform parsing rules.
    /// @throws err::ParameterFailure If the path cannot be represented unambiguously in PATH.
    static void validate(const path::Path &path, const EnvironmentVariableBackend &backend);

public: // executable lookup
    /// Build candidate paths for a name or explicit absolute path, without filesystem access.
    /// @param name Executable name or absolute native path; Windows adds .exe if no suffix is present.
    /// @return Candidates in PATH order, or the single explicit path.
    /// @throws err::ParameterFailure If the name is empty or an ambiguous relative path.
    [[nodiscard]] auto executableCandidates(const text::String &name) const -> path::PathList;

private:
    /// One valid native entry and its expanded lexical comparison key.
    /// @tested{EnvironmentPathsTest}
    struct Entry {
        text::String spelling; ///< Original valid PATH entry.
        path::Path key;        ///< Expanded, lexically normalized comparison path.
    };
    /// Find the first equivalent normalized key.
    [[nodiscard]] auto find(const path::Path &key) const -> unit::ItemIndex;

private:
    const EnvironmentVariableBackend &_backend; ///< Platform parsing policy.
    Lookup _lookup;                             ///< Environment for Windows expansion.
    util::List<Entry> _entries;                 ///< Valid entries in search order.
};

}
