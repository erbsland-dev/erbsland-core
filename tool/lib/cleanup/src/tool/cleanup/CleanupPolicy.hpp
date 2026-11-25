// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SourceTree.hpp"

#include <erbsland/conf/Value.hpp>
#include <erbsland/StringList.hpp>
#include <tool/frame/ProjectFiles.hpp>

#include <exception>
#include <vector>

namespace tool::cleanup {

/// Validated repository cleanup settings; all directories are project-confined.
class CleanupPolicy final {
public: // configuration
    /// Read validated ELCL and resolve project paths.
    /// @param configuration The code_cleanup section.
    /// @param files Project file service.
    void read(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files);
    /// Find the explicit CMake target for the most specific matching root.
    /// @param directory Directory containing a CMakeLists.txt.
    /// @return Target name, or empty when CMake cleanup is disabled there.
    [[nodiscard]] auto cmakeTarget(const el::Path &directory) const -> el::String;
    /// Test whether a filename matches the configured C++ suffix and exclusion filters.
    /// @param path Candidate path; tree membership is checked separately.
    /// @return Whether its suffix is selected and its name is not excluded.
    [[nodiscard]] auto selects(const el::Path &path) const -> bool;

public:
    el::Path library;                                   ///< Library implementation root.
    el::Path publicIncludePrefix;                       ///< Compiler-visible prefix of mapped library headers.
    bool sharedIncludePrefix = false;                   ///< Preserve external angle includes under the public prefix.
    el::Path mapping;                                   ///< Canonical-first public header map.
    el::Path style;                                     ///< Explicit clang-format style file.
    el::Path cache;                                     ///< Rebuildable fingerprint cache.
    el::conf::ValuePtr formatterValue;                  ///< Configuration source for formatter lookup errors.
    el::String cmakeVersion;                            ///< Minimum version for simple CMake files.
    el::StringList suffixes;                            ///< C++ filename suffixes.
    el::StringList excludedNames;                       ///< Exact filenames skipped by C++ cleanup.
    std::vector<SourceTree> trees;                      ///< Non-overlapping source policies.
    std::vector<std::pair<el::Path, el::String>> cmake; ///< Directory-to-target policies.

private:
    /// Read and validate cleanup options.
    void readOptions(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files);
    /// Read and validate cleanup sourcetrees.
    void readSourceTrees(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files);
    /// Read and validate cleanup cmaketrees.
    void readCmakeTrees(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files);
    /// Resolve safe directory globs into sorted, non-overlapping project directories.
    /// @param patterns Nonempty project-relative directory patterns.
    /// @param files Project file service.
    /// @return Concrete roots with duplicate and nested selections collapsed.
    /// @throws el::conf::ConfError If a pattern is unsafe, invalid, or matches no directories.
    [[nodiscard]] static auto directories(const el::conf::ValuePtr &patterns, const frame::ProjectFiles &files)
        -> el::PathList;
    /// Validate a nonempty project-relative path before resolving it.
    static auto path(const el::conf::ValuePtr &value, const frame::ProjectFiles &files, bool directory = false)
        -> el::Path;
    /// Report an invalid cleanup setting with its configuration location.
    [[noreturn]] static void throwConfigurationError(
        const el::conf::ValuePtr &value, const el::String &description, const std::exception_ptr &cause = {});
};

}
