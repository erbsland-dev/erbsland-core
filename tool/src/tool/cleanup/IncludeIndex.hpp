// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CleanupPolicy.hpp"

#include "../headers/PublicHeaderMap.hpp"

#include <erbsland/StringHashMap.hpp>
#include <erbsland/StringHashSet.hpp>

namespace tool::cleanup {

/// Immutable lookup tables shared by parallel C++ cleanup jobs.
class IncludeIndex final {
public:
    /// Bind the configured library location and compiler-visible public prefix.
    /// @param policy Validated cleanup settings.
    /// @param projectRoot Absolute project root used to derive source include spellings.
    IncludeIndex(const CleanupPolicy &policy, const el::Path &projectRoot);

public: // inventory
    /// Add a regular file discovered by a confined, link-skipping walk.
    /// @param path Absolute file path.
    void add(const el::Path &path);
    /// Read the generator's canonical-first mapping and validate its records.
    /// @param text Complete mapping file.
    void readMapping(const el::String &text);
    /// Resolve one include, preserving unrelated external and local consumer headers.
    /// @param file Source file containing the include.
    /// @param name Include path without delimiters.
    /// @param global Whether angle brackets are used; updated on successful resolution.
    /// @param tree Source-tree policy.
    /// @return Normalized path without delimiters.
    /// @throws el::ApplicationError If a project include is missing or ambiguous.
    [[nodiscard]] auto resolve(const el::Path &file, const el::String &name, bool &global, const SourceTree &tree) const
        -> el::String;
    /// Test whether the inventory contains a regular file.
    /// @param path Absolute candidate path, normalized lexically before lookup.
    /// @return Whether this file was discovered by the inventory.
    [[nodiscard]] auto contains(const el::Path &path) const -> bool;

private:
    /// Test a path that has already been normalized lexically.
    [[nodiscard]] auto containsNormalized(const el::Path &path) const -> bool;
    /// Find a library header using exact path, unique suffix, then closest filename context.
    [[nodiscard]] auto libraryTarget(const el::String &name, const el::Path &file) const -> el::String;
    /// Convert a source-relative path into its canonical public spelling.
    [[nodiscard]] auto publicName(const el::String &source) const -> el::String;
    /// Test a consumer-local include and its optional generated .in template.
    [[nodiscard]] auto local(const el::Path &file, const el::String &name, const SourceTree &tree) const -> bool;

private:
    el::Path _library;                         ///< Absolute library implementation root.
    el::String _sourcePrefix;                  ///< Project-relative library directory with a trailing slash.
    el::String _sourceMarker;                  ///< Source prefix with a leading slash for component matching.
    el::String _publicPrefix;                  ///< Compiler-visible include directory with a trailing slash.
    el::StringHashSet _files;                  ///< Absolute, lexically normalized regular paths.
    el::StringHashSet _libraryFiles;           ///< Library-relative file paths.
    el::StringHashMap<el::StringList> _byName; ///< Library filenames and possible source paths.
    headers::PublicHeaderMap _mapping;         ///< Shared canonical and compatibility publication map.
};

}
