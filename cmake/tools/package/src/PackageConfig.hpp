// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PackageSettings.hpp"

#include <erbsland/conf/Document.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringList.hpp>

#include <vector>

namespace erbsland::package {

/// Reads a package ELCL file and resolves layered settings.
/// @notest{Covered by package integration tests.}
class PackageConfig final {
public:
    /// Read the optional configuration file and enable environment placeholders.
    explicit PackageConfig(path::Path file);
    /// Resolve the settings for the named package, platform, architecture, and target.
    [[nodiscard]] auto settings(
        const text::String &name, const text::String &platform, const text::String &architecture,
        const text::String &target, const text::String &projectName,
        const text::String &projectVersion) const -> PackageSettings;
    /// Get the directory used to resolve relative configured paths.
    [[nodiscard]] auto baseDirectory() const noexcept -> const path::Path & { return _baseDirectory; }
    /// Expand package placeholders, failing on an unknown attribute.
    [[nodiscard]] static auto expand(
        const text::String &format, const PackageSettings &settings, const text::String &platform,
        const text::String &architecture, const text::String &target) -> text::String;

private:
    /// Quote a package or target name when ELCL requires a text name.
    [[nodiscard]] static auto pathSegment(const text::String &name) -> text::String;
    /// Merge one existing override section.
    void applySection(PackageSettings &result, const text::String &sectionPath) const;
    /// Append file selection entries from a section list.
    void addFiles(PackageSettings &result, const text::String &sectionPath) const;

private:
    path::Path _baseDirectory; ///< Config directory or project root.
    conf::DocumentPtr _document; ///< Parsed optional configuration.
};

}
