// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PackageConfig.hpp"

#include <erbsland/path/Path.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringList.hpp>

#include <vector>

namespace erbsland::package {

/// Platform-specific dependency, signing, and archive operations.
/// @notest{Covered by platform package integration tests.}
class PackagePlatform final {
public:
    /// Run a child without a shell and report a sanitized failure.
    static auto run(
        const path::Path &program, const text::StringList &arguments,
        const text::String &secret = {}) -> text::String;
    /// Resolve Windows DLL dependencies using CMake's scanner.
    [[nodiscard]] static auto windowsDependencies(
        const path::Path &cmake, const path::Path &executable, const path::Path &output,
        const std::vector<path::Path> &searchDirectories) -> std::vector<path::Path>;
    /// Sign and verify a staged Windows executable.
    static void signWindows(const path::Path &executable, const PackageSettings &settings);
    /// Repair and verify a staged macOS app bundle.
    static void fixMacBundle(
        const path::Path &cmake, const path::Path &bundle, const std::vector<path::Path> &searchDirectories);
    /// Sign the nested code and outer macOS app bundle.
    static void signMacBundle(const path::Path &bundle, const PackageSettings &settings);
    /// Submit, wait for, and staple macOS notarization.
    static void notarizeMacBundle(const path::Path &bundle, const PackageSettings &settings, const path::Path &temporaryZip);
    /// Create a ZIP archive from a package-root directory.
    static void createArchive(const path::Path &packageRoot, const path::Path &temporaryZip);
};

}
