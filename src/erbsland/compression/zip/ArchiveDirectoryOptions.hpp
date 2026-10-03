// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ArchiveEntryOptions.hpp"

#include "../../path/Path_fwd.hpp"
#include "../../path/PathGlobPattern_fwd.hpp"
#include "../../path/PathInfo_fwd.hpp"
#include "../../text/StringList.hpp"

#include <functional>
#include <utility>

namespace erbsland::compression::zip {

/// Options for recursively adding a directory to a ZIP archive.
/// Globs match case-sensitively against normalized `/`-separated archive paths; `*` and `?` stay within one path
/// component while whole-component `**` matches zero or more components. Core sets and escapes are supported.
/// @seedoc{/reference/path/path_glob_patterns}
/// @tested{ZipArchiveTest}
class ArchiveDirectoryOptions final {
public:
    using Filter = std::function<bool(const path::Path &, const path::PathInfo &)>;

public:
    /// Get whether traversal descends below immediate child directories.
    [[nodiscard]] auto recursive() const noexcept -> bool { return _recursive; }
    /// Set whether traversal descends below immediate child directories.
    auto setRecursive(bool value) noexcept -> ArchiveDirectoryOptions & {
        _recursive = value;
        return *this;
    }
    /// Get whether the source directory itself is stored as the leading component.
    [[nodiscard]] auto includeRootDirectory() const noexcept -> bool { return _includeRootDirectory; }
    /// Set whether the source directory itself is stored as the leading component.
    auto setIncludeRootDirectory(bool value) noexcept -> ArchiveDirectoryOptions & {
        _includeRootDirectory = value;
        return *this;
    }
    /// Get the compiled include patterns; null accepts every path.
    [[nodiscard]] auto includeGlobs() const noexcept -> const path::PathGlobPatternPtr & { return _includeGlobs; }
    /// Replace the include patterns; null disables inclusion filtering.
    auto setIncludeGlobs(path::PathGlobPatternPtr value) noexcept -> ArchiveDirectoryOptions & {
        _includeGlobs = std::move(value);
        return *this;
    }
    /// Compile include patterns, preserving the previous value on failure.
    /// @param value Semicolon- or newline-separated patterns with default trimming and recursive wildcards.
    /// @throws err::ParseError If the pattern text is invalid or contains no alternatives.
    auto setIncludeGlobs(const text::String &value) -> ArchiveDirectoryOptions &;
    /// Join include patterns with semicolons and compile them; an empty list clears the filter.
    /// @param value Pattern sources, interpreted together as semicolon-separated alternatives.
    /// @throws err::ParseError If the joined pattern text is invalid; the previous value remains intact.
    auto setIncludeGlobs(const text::StringList &value) -> ArchiveDirectoryOptions &;
    /// Get the compiled exclude patterns; null excludes no paths.
    [[nodiscard]] auto excludeGlobs() const noexcept -> const path::PathGlobPatternPtr & { return _excludeGlobs; }
    /// Replace the exclude patterns; null disables exclusion filtering.
    auto setExcludeGlobs(path::PathGlobPatternPtr value) noexcept -> ArchiveDirectoryOptions & {
        _excludeGlobs = std::move(value);
        return *this;
    }
    /// Compile exclude patterns, preserving the previous value on failure.
    /// @param value Semicolon- or newline-separated patterns with default trimming and recursive wildcards.
    /// @throws err::ParseError If the pattern text is invalid or contains no alternatives.
    auto setExcludeGlobs(const text::String &value) -> ArchiveDirectoryOptions &;
    /// Join exclude patterns with semicolons and compile them; an empty list clears the filter.
    /// @param value Pattern sources, interpreted together as semicolon-separated alternatives.
    /// @throws err::ParseError If the joined pattern text is invalid; the previous value remains intact.
    auto setExcludeGlobs(const text::StringList &value) -> ArchiveDirectoryOptions &;
    /// Get the optional source-path and metadata callback.
    [[nodiscard]] auto filter() const noexcept -> const Filter & { return _filter; }
    /// Set the optional source-path and metadata callback.
    auto setFilter(Filter value) noexcept -> ArchiveDirectoryOptions & {
        _filter = std::move(value);
        return *this;
    }
    /// Get the defaults applied to accepted regular files.
    [[nodiscard]] auto entryOptions() const noexcept -> const ArchiveEntryOptions & { return _entryOptions; }
    /// Set the defaults applied to accepted regular files.
    auto setEntryOptions(ArchiveEntryOptions value) noexcept -> ArchiveDirectoryOptions & {
        _entryOptions = std::move(value);
        return *this;
    }

private:
    /// Compile a pattern list using the archive filtering syntax.
    [[nodiscard]] static auto compileGlobs(const text::String &value) -> path::PathGlobPatternPtr;

private:
    bool _recursive{true};
    bool _includeRootDirectory{};
    path::PathGlobPatternPtr _includeGlobs; ///< Shared include alternatives, null for unrestricted inclusion.
    path::PathGlobPatternPtr _excludeGlobs; ///< Shared exclude alternatives, null for no exclusions.
    Filter _filter;
    ArchiveEntryOptions _entryOptions;
};

}
