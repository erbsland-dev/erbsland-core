// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IncludeIndex.hpp"

namespace tool::cleanup {

/// Repair paths and sort the leading include region, then enforce project spacing.
class CppCleanup final {
public:
    /// Bind immutable per-run include indexes and policy.
    /// @param index Include inventory; must outlive this cleaner.
    explicit CppCleanup(const IncludeIndex &index);

public: // cleanup
    /// Normalize one source without reading or writing any files.
    /// @param path Absolute source path.
    /// @param text Original or clang-formatted source.
    /// @param tree The owning source tree.
    /// @return Cleaned source, retaining comments and unsupported include directives.
    /// @throws el::ApplicationError If an include cannot be resolved unambiguously.
    [[nodiscard]] auto apply(const el::Path &path, const el::String &text, const SourceTree &tree) const -> el::String;
    /// Fingerprint code outside the leading comment/pragma/include region.
    /// @param text Text after cheap cleanup.
    /// @return SHA-256 fingerprint used to select expensive formatting.
    [[nodiscard]] static auto codeFingerprint(const el::String &text) -> el::String;

private:
    /// One resolved include and its stable sort priority.
    struct Include {
        int priority;    ///< Include group, with the matching source header first.
        el::String name; ///< Resolved path used for case-insensitive sorting.
        el::String line; ///< Complete rendered line, including its trailing comment.
    };
    /// Copy the initial comment header and enforce compact pragma spacing.
    static auto copyHeader(el::StringList &output, const el::StringList &lines, el::ItemIndex end) -> el::ItemIndex;
    /// Sort independent include runs, preserving comments and macros as barriers.
    void sortIncludes(
        el::StringList &output,
        const el::StringList &lines,
        el::ItemIndex start,
        el::ItemIndex end,
        const el::Path &path,
        const SourceTree &tree) const;
    /// Remove the single copyright/own-header gap in ordinary source files.
    static void compactMainHeader(el::StringList &output, const el::Path &path);
    /// Locate the end of the leading region; conditional and other directives are barriers.
    static auto leadingEnd(const el::StringList &lines) -> el::ItemIndex;
    /// Normalize and sort literal includes in one run, preserving trailing comments.
    void sortRun(el::StringList &output, const el::StringList &run, const el::Path &path, const SourceTree &tree) const;
    /// Get the established include grouping priority.
    static auto group(const el::String &name, bool global) -> int;
    /// Determine whether this is the corresponding header for a .cpp or _part.cpp file.
    static auto isMain(const el::Path &path, const el::String &name, bool global) -> bool;

private:
    const IncludeIndex &_index; ///< Read-only resolution tables.
};

}
