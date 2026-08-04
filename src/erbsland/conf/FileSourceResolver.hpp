// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FileSourceResolver_fwd.hpp"
#include "SourceResolver.hpp"

#include "../path/Path.hpp"
#include "../text/StringList.hpp"

#include <bitset>
#include <exception>
#include <memory>

namespace erbsland::conf {

/// A file source resolver.
/// The file source resolver supports the recommended format to include files. It works with relative and
/// absolute paths and also has support for wildcards.
/// Here are a few examples:
/// @code
/// &at;include "file:example.elcl"              # File in the same directory.
/// &at;include "file:sub/example.elcl"          # File in a subdirectory of the current configuration file.
/// &at;include "file:../example.elcl"           # File in the parent directory (if access rules allow it)
/// &at;include "file:/usr/local/example.elcl"   # Absolute path.
/// @endcode
/// @tested{FileSourceResolverTest}
class FileSourceResolver : public SourceResolver {
public:
    /// Features of the file source resolver
    enum Feature : uint8_t {
        RecursiveWildcard, ///< Support for recursive wildcards.
        FilenameWildcard,  ///< Support for filename wildcards.
        AbsolutePaths,     ///< Support for absolute paths.
        WindowsUNCPath,    ///< Support for Windows UNC paths.
        FileProtocol,      ///< Support for the `file:` protocol prefix.

        _featureCount
    };

public:
    /// Create a new instance of the file source resolver.
    static auto create() -> FileSourceResolverPtr { return std::make_shared<FileSourceResolver>(); }

    // defaults
    FileSourceResolver() = default;
    ~FileSourceResolver() override = default;

public: // Settings.
    /// Enable a feature
    void enable(Feature feature);
    /// Disable a feature
    void disable(Feature feature);
    /// Test if a feature enabled.
    [[nodiscard]] auto isEnabled(Feature feature) const -> bool;

public: // implement `SourceResolver`
    auto resolve(const SourceResolverContext &context) -> SourceListPtr override;

private:
    /// Store the fixed portions of a filename wildcard pattern.
    struct FilenamePattern {
        text::String prefix;
        text::String suffix;
        bool hasWildcard;

        /// Test whether a path name matches this pattern.
        [[nodiscard]] auto matches(const path::Path &path) const noexcept -> bool;
    };

private:
    /// Remove the optional `file:` protocol prefix.
    /// @param path The path to modify.
    void removeFileProtocol(text::String &path) const;
    /// Normalize all path separators to slash characters.
    /// @param path The path to modify.
    void normalizePathSeparators(text::String &path);
    /// Validate a Windows UNC path.
    static void verifyUncPath(const text::String &path);
    /// Split a path into its directory and filename portions.
    [[nodiscard]] static auto splitDirectoryAndFilename(const text::String &path) noexcept
        -> std::tuple<text::String, text::String>;
    /// Parse a filename wildcard pattern.
    [[nodiscard]] static auto getFilenamePattern(const text::String &filename) -> FilenamePattern;
    /// Validate and normalize a directory wildcard pattern.
    [[nodiscard]] static auto validateDirectoryWildcard(const text::String &directory)
        -> std::tuple<text::String, bool>;
    /// Get the base directory for a source identifier.
    [[nodiscard]] static auto getBaseDirectory(const SourceIdentifierPtr &sourceIdentifier) -> path::Path;
    /// Build an included-file directory path.
    [[nodiscard]] auto buildDirectory(const SourceIdentifierPtr &sourceIdentifier, const text::String &directory) const
        -> path::Path;
    /// Scan a directory for paths matching a filename pattern.
    [[nodiscard]] static auto scanForPaths(
        const path::Path &directory, bool isRecursive, const FilenamePattern &filenamePattern) -> path::PathList;
    /// Create configuration sources for the given paths.
    [[nodiscard]] static auto createSourcesFromPaths(const path::PathList &paths) -> SourceListPtr;
    /// Throw a file-resolution error.
    [[noreturn]] static void throwError(
        text::String message, std::optional<path::Path> path = std::nullopt, std::exception_ptr cause = {});
    /// Order sources by their resolved paths.
    [[nodiscard]] static auto sortLess(const SourcePtr &a, const SourcePtr &b) noexcept -> bool;
    /// Split a protocol-stripped path into components.
    [[nodiscard]] static auto splitPath(const text::String &path) noexcept -> text::StringList;

private:
    std::bitset<_featureCount> _features{0b11111}; ///< Flags for the features.
};

}
