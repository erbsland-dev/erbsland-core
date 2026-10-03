// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FileSourceResolver_fwd.hpp"
#include "SourceResolver.hpp"

#include "../path/Path.hpp"
#include "../path/PathGlobPattern_fwd.hpp"
#include "../text/StringList.hpp"

#include <bitset>
#include <exception>
#include <memory>

namespace erbsland::conf {

/// Resolve file includes using literal paths or compiled Core path globs.
/// @seedoc{/topics/conf/source-resolving}
/// @tested{FileSourceResolverTest}
class FileSourceResolver : public SourceResolver {
public:
    /// Features of the file source resolver
    enum Feature : uint8_t {
        RecursiveWildcard, ///< Support for recursive wildcards.
        Wildcards,         ///< Support for *, ?, sets, and recursive wildcards in path components.
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
    /// Remove the optional `file:` protocol prefix.
    /// @param path The path to modify.
    void removeFileProtocol(text::String &path) const;
    /// Validate a Windows UNC path.
    static void verifyUncPath(const text::String &path);
    /// Get the base directory for a source identifier.
    [[nodiscard]] static auto getBaseDirectory(const SourceIdentifierPtr &sourceIdentifier) -> path::Path;
    /// Build an included-file directory path.
    [[nodiscard]] auto buildDirectory(const SourceIdentifierPtr &sourceIdentifier, const path::Path &directory) const
        -> path::Path;
    /// Scan a directory for paths matching a compiled relative path pattern.
    [[nodiscard]] static auto scanForPaths(const path::Path &directory, const path::PathGlobPatternPtr &pattern)
        -> path::PathList;
    /// Create configuration sources for the given paths.
    [[nodiscard]] static auto createSourcesFromPaths(const path::PathList &paths) -> SourceListPtr;
    /// Throw a file-resolution error.
    [[noreturn]] static void throwError(
        text::String message, std::optional<path::Path> path = std::nullopt, std::exception_ptr cause = {});
    /// Order sources by their resolved paths.
    [[nodiscard]] static auto sortLess(const path::Path &a, const path::Path &b) noexcept -> bool;

private:
    std::bitset<_featureCount> _features{0b11111}; ///< Flags for the features.
};

}
