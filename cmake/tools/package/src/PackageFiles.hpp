// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PackageConfig.hpp"

#include <erbsland/path/Path.hpp>
#include <erbsland/text/String.hpp>

namespace erbsland::package {

/// Safely stages configured files below one package root.
/// @notest{Covered by package integration tests.}
class PackageFiles final {
public:
    /// Create a stage helper for paths relative to the ELCL file.
    PackageFiles(path::Path baseDirectory, path::Path packageRoot);
    /// Stage one configured source file or directory.
    void add(const FileEntry &entry) const;
    /// Copy a generated or built file into the package, rejecting collisions.
    void copyFile(const path::Path &source, const path::Path &relativeDestination) const;
    /// Copy a generated directory, preserving internal symlinks.
    void copyDirectory(const path::Path &source, const path::Path &relativeDestination) const;
    /// Validate a relative package path.
    static void validateRelative(const path::Path &path, bool allowEmpty = false);

private:
    /// Match the relative path against package glob patterns.
    [[nodiscard]] static auto matches(const text::StringList &patterns, const text::String &relative) -> bool;
    /// Match the relative path against package regular expressions.
    [[nodiscard]] static auto matchesRegex(const text::StringList &patterns, const text::String &relative) -> bool;

private:
    path::Path _baseDirectory; ///< Configuration file directory.
    path::Path _packageRoot; ///< Staging root for this package.
};

}
