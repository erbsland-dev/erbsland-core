// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ArchiveEntryOptions.hpp"

#include "../../path/Path_fwd.hpp"
#include "../../path/PathInfo_fwd.hpp"
#include "../../text/StringList.hpp"

#include <functional>
#include <utility>

namespace erbsland::compression::zip {

/// Options for recursively adding a directory to a ZIP archive.
/// Globs match case-sensitively against normalized `/`-separated archive paths; `*` and `?` stay within one path
/// component while `**` crosses component boundaries.
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
    /// Get the case-sensitive include glob patterns.
    [[nodiscard]] auto includeGlobs() const noexcept -> const text::StringList & { return _includeGlobs; }
    /// Replace the case-sensitive include glob patterns.
    auto setIncludeGlobs(text::StringList value) noexcept -> ArchiveDirectoryOptions & {
        _includeGlobs = std::move(value);
        return *this;
    }
    /// Get the case-sensitive exclude glob patterns.
    [[nodiscard]] auto excludeGlobs() const noexcept -> const text::StringList & { return _excludeGlobs; }
    /// Replace the case-sensitive exclude glob patterns.
    auto setExcludeGlobs(text::StringList value) noexcept -> ArchiveDirectoryOptions & {
        _excludeGlobs = std::move(value);
        return *this;
    }
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
    bool _recursive{true};
    bool _includeRootDirectory{};
    text::StringList _includeGlobs;
    text::StringList _excludeGlobs;
    Filter _filter;
    ArchiveEntryOptions _entryOptions;
};

}
