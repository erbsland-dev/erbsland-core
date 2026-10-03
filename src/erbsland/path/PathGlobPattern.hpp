// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Path_fwd.hpp"
#include "PathGlobMatchMode.hpp"
#include "PathGlobPattern_fwd.hpp"
#include "PathGlobPatternFlags.hpp"

#include "impl/PathGlobCompiler_fwd.hpp"
#include "impl/PathGlobData_fwd.hpp"

#include "../text/AnyString.hpp"
#include "../text/String_fwd.hpp"

#include <memory>

namespace erbsland::path {

/// An immutable, case-sensitive path glob, safe for concurrent matching without filesystem access.
/// @seedoc{/reference/path/path_glob_patterns}
/// @tested{PathGlobPatternTest PathWalkerFilterTest FileSourceResolverTest ZipArchiveTest}
class PathGlobPattern final {
    friend class impl::PathGlobCompiler;

public: // construction
    /// Release the compiled program when its last shared owner releases the pattern.
    ~PathGlobPattern();
    // defaults
    PathGlobPattern(const PathGlobPattern &) = delete;
    auto operator=(const PathGlobPattern &) -> PathGlobPattern & = delete;

public: // factories
    /// Compile path components and optional OR alternatives using Core character-set syntax.
    /// @param pattern Pattern text in any supported encoding.
    /// @param flags Enabled syntax; explicit flags replace the default set.
    /// @return A fully compiled immutable pattern.
    /// @throws err::ParseError If syntax is invalid or no alternatives remain.
    [[nodiscard]] static auto compile(
        text::AnyString pattern, PathGlobPatternFlags flags = PathGlobPatternFlag::Default) -> PathGlobPatternPtr;

public: // matching
    /// Match every non-root component, ignoring the pattern's leading anchor.
    /// @param path Candidate path; empty paths never match.
    /// @return Whether the complete path matches any alternative.
    [[nodiscard]] auto fullMatch(const Path &path) const -> bool;
    /// Match a suffix beginning at a component boundary and reaching the path end.
    /// @param path Candidate path; anchored alternatives start at its first non-root component.
    /// @return Whether any alternative matches.
    [[nodiscard]] auto suffixMatch(const Path &path) const -> bool;
    /// Match using a runtime-selected mode.
    /// @param path Candidate path; roots do not participate in matching.
    /// @param mode Full or suffix matching.
    /// @return Whether any alternative matches.
    [[nodiscard]] auto match(const Path &path, PathGlobMatchMode mode) const -> bool;
    /// Test whether a strict descendant could match, without accessing the filesystem.
    /// False guarantees that pruning is safe; true may conservatively retain a branch.
    /// @param directory Candidate directory, independent of its filesystem type or existence.
    /// @param mode Matching mode to use for descendants.
    /// @return Whether a matching descendant remains possible; false for an empty path.
    [[nodiscard]] auto mayMatchDescendant(const Path &directory, PathGlobMatchMode mode = PathGlobMatchMode::Full) const
        -> bool;
    /// Match text after the same lexical parsing as Path{text}.
    /// @param text Candidate path spelling.
    /// @return Whether the complete parsed path matches.
    [[nodiscard]] auto fullMatch(const text::String &text) const -> bool;
    /// Match a suffix of a lexically parsed path.
    /// @param text Candidate path spelling.
    /// @return Whether a suffix matches.
    [[nodiscard]] auto suffixMatch(const text::String &text) const -> bool;
    /// Match a lexically parsed path with a selected mode.
    /// @param text Candidate path spelling.
    /// @param mode Full or suffix matching.
    /// @return Whether the parsed path matches.
    [[nodiscard]] auto match(const text::String &text, PathGlobMatchMode mode) const -> bool;
    /// Test descendant feasibility after lexical path parsing.
    /// @param text Candidate directory spelling.
    /// @param mode Matching mode to use for descendants.
    /// @return Whether a strict descendant could match.
    [[nodiscard]] auto mayMatchDescendant(
        const text::String &text, PathGlobMatchMode mode = PathGlobMatchMode::Full) const -> bool;

private:
    /// Retain fully compiled immutable data.
    explicit PathGlobPattern(std::unique_ptr<const impl::PathGlobData> data);
    std::unique_ptr<const impl::PathGlobData> _data; ///< Compiled alternatives; never mutated after publication.
};

}
