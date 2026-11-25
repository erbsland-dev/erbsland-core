// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CleanupPolicy.hpp"

#include <erbsland/StringHashMap.hpp>

namespace tool::cleanup {

/// Run the external formatter only for code fingerprints absent from a successful baseline.
class ClangFormat final {
public:
    /// Resolve the formatter, capture its version and read a matching cache.
    /// @param policy Formatting configuration.
    /// @param files Confined project file service.
    /// @param full Ignore all cached file fingerprints.
    ClangFormat(const CleanupPolicy &policy, const frame::ProjectFiles &files, bool full);

public: // formatting
    /// Test whether a source's non-include code differs from its successful baseline.
    /// @param path Absolute source path.
    /// @param fingerprint Digest after cheap cleanup.
    /// @return Whether this file needs a formatter run.
    [[nodiscard]] auto needsFormat(const el::Path &path, const el::String &fingerprint) const -> bool;
    /// Format a source to captured stdout, without editing it.
    /// @param path Checked source path.
    /// @return Complete formatted source.
    /// @throws el::ApplicationError If the process fails or output is truncated.
    [[nodiscard]] auto format(const el::Path &path) const -> el::String;
    /// Build a cache from successfully applied file fingerprints.
    /// @param fingerprints Project-relative path and code digest pairs.
    /// @return Complete cache text.
    [[nodiscard]] auto cacheText(const std::vector<std::pair<el::String, el::String>> &fingerprints) const
        -> el::String;

private:
    /// Find a real executable outside the project, following system installation symlinks.
    static auto executable(const el::conf::ValuePtr &value, const el::Path &root) -> el::Path;
    /// Run directly with bounded capture and a restricted inherited environment.
    [[nodiscard]] auto run(const el::StringList &arguments) const -> el::String;

private:
    const CleanupPolicy &_policy;                ///< Immutable run configuration.
    const frame::ProjectFiles &_files;           ///< Confined file access.
    el::Path _executable;                        ///< Validated absolute program path.
    el::String _signature;                       ///< Versioned formatting configuration fingerprint.
    el::StringHashMap<el::String> _fingerprints; ///< Prior successful per-file code digests.
};

}
