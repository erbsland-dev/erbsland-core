// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FileEntry.hpp"

#include <erbsland/text/String.hpp>

#include <vector>

namespace erbsland::package {

/// Effective settings for one package.
/// @notest{Covered by package integration tests.}
struct PackageSettings final {
    text::String name;                    ///< Effective package name.
    text::String projectName;             ///< Root CMake project name.
    text::String versionSource{"cmake"};  ///< Version provider.
    text::String version;                 ///< Complete version text.
    text::String versionFile;             ///< Relative version source file.
    text::String versionPattern;          ///< Version extraction expression.
    text::String targetDir;               ///< Archive root directory format.
    text::String filenameFormat;          ///< ZIP filename format without extension.
    text::String bundleId;                ///< macOS bundle identifier.
    text::String signingIdentity;         ///< macOS Developer ID identity.
    text::String notaryProfile;           ///< macOS notarytool keychain profile.
    text::String certificateSha1;         ///< Windows certificate thumbprint.
    text::String certificateFile;         ///< Windows signing certificate path.
    text::String csp;                     ///< Windows cryptographic provider.
    text::String key;                     ///< Windows key identifier.
    text::String timestampServer;         ///< Windows timestamp service.
    text::String signingToolArchitecture; ///< Windows signing host architecture.
    text::String signingToolPath;         ///< Optional explicit Windows SignTool executable.
    bool macApp{true};                    ///< Create macOS application bundles.
    bool signing{};                       ///< Enable code signing.
    bool notarization{};                  ///< Enable notarization.
    std::vector<FileEntry> files;         ///< Extra files and directories.
    std::vector<path::Path> dependencyDirectories; ///< Additional runtime-library locations.
};

}
