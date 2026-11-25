// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/StringSet.hpp>
#include <tool/frame/ProjectFiles.hpp>

namespace tool::security_hashes {

/// Validated source selection and manifest destination.
class Settings final {
public: // configuration
    /// Create the documented configuration schema.
    /// @return Rules for the security_hashes section.
    [[nodiscard]] static auto rules() -> el::conf::vr::RulesPtr;
    /// Read settings after schema validation.
    /// @param configuration Validated command section.
    /// @param files Project confinement service.
    /// @throws el::conf::ConfError If a configured path or selection cannot be used.
    void read(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files);

public: // selection
    /// Test whether a directory or one of its ancestors is excluded.
    /// @param path Absolute project path.
    [[nodiscard]] auto excludesDirectory(const el::Path &path) const -> bool;
    /// Test whether a file is excluded, including the manifest itself.
    /// @param path Absolute project path.
    [[nodiscard]] auto excludesFile(const el::Path &path) const -> bool;
    /// Test whether a recursively discovered file has a selected name or suffix.
    /// @param path Absolute project path.
    [[nodiscard]] auto selects(const el::Path &path) const -> bool;
    /// Test whether a path can be represented unambiguously in the manifest.
    /// @param text Project-relative POSIX path.
    [[nodiscard]] static auto isManifestPath(const el::String &text) -> bool;

private:
    /// Read and validate a list of local filenames or dotted suffixes.
    [[nodiscard]] static auto readNames(const el::conf::ValuePtr &value, bool suffixes) -> el::StringSet;
    /// Read project-relative confined paths, retaining each entry in diagnostics.
    [[nodiscard]] static auto readPath(
        const el::conf::ValuePtr &value, const frame::ProjectFiles &files, bool allowMissingParents = false)
        -> el::Path;

public:
    el::Path output;              ///< Manifest destination with an existing parent directory.
    el::PathList directories;     ///< Recursive source roots.
    el::PathList individualFiles; ///< Explicit files, independent of suffix selection.

private:
    el::Path _root;                  ///< Validated project root.
    el::PathList _excludedPaths;     ///< Excluded directory subtrees.
    el::StringSet _directoryNames;   ///< Excluded directory basenames.
    el::StringSet _fileNames;        ///< Excluded filenames.
    el::StringSet _fileSuffixes;     ///< Excluded dotted suffixes.
    el::StringSet _includedNames;    ///< Selected exact filenames.
    el::StringSet _includedSuffixes; ///< Selected dotted suffixes.
};

}
