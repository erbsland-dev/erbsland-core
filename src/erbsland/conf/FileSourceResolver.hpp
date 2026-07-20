// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SourceResolver.hpp"

#include "../path/Path.hpp"
#include "../text/StringList.hpp"

#include <bitset>
#include <exception>
#include <memory>

namespace erbsland::conf {

class FileSourceResolver;
using FileSourceResolverPtr = std::shared_ptr<FileSourceResolver>;

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

    /// Default constructor.
    FileSourceResolver() = default;
    /// Default destructor.
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
    struct FilenamePattern {
        text::String prefix;
        text::String suffix;
        bool hasWildcard;

        [[nodiscard]] auto matches(const path::Path &path) const noexcept -> bool;
    };

private:
    void removeFileProtocol(text::String &path) const;
    void normalizePathSeparators(text::String &path);
    static void verifyUncPath(const text::String &path);
    [[nodiscard]] static auto splitDirectoryAndFilename(const text::String &path) noexcept
        -> std::tuple<text::String, text::String>;
    [[nodiscard]] static auto getFilenamePattern(const text::String &filename) -> FilenamePattern;
    [[nodiscard]] static auto validateDirectoryWildcard(const text::String &directory)
        -> std::tuple<text::String, bool>;
    [[nodiscard]] static auto getBaseDirectory(const SourceIdentifierPtr &sourceIdentifier) -> path::Path;
    [[nodiscard]] auto buildDirectory(const SourceIdentifierPtr &sourceIdentifier, const text::String &directory) const
        -> path::Path;
    [[nodiscard]] static auto scanForPaths(
        const path::Path &directory, bool isRecursive, const FilenamePattern &filenamePattern) -> path::PathList;
    [[nodiscard]] static auto createSourcesFromPaths(const path::PathList &paths) -> SourceListPtr;
    [[noreturn]] static void throwError(
        text::String message, std::optional<path::Path> path = std::nullopt, std::exception_ptr cause = {});
    [[nodiscard]] static auto sortLess(const SourcePtr &a, const SourcePtr &b) noexcept -> bool;
    [[nodiscard]] static auto splitPath(const text::String &path) noexcept -> text::StringList;

private:
    std::bitset<_featureCount> _features{0b11111}; ///< Flags for the features.
};

}
