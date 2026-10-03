// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Hasher.hpp>
#include <erbsland/Path.hpp>

#include <filesystem>

namespace tool::pre_commit::impl {

/// Preserve literal symlink targets and Git filenames outside Core's text/path repertoire.
class NativeGitFile final {
public:
    /// Bind a raw, NUL-delimited Git filename to its repository root.
    /// @param root Repository root represented by Core.
    /// @param relative Raw UTF-8/native bytes from Git's forward-slash filename listing.
    NativeGitFile(const el::Path &root, el::String relative);
    /// Hash the literal native target without normalization, including dangling links.
    [[nodiscard]] auto symlinkFingerprint() const -> el::ByteBlock;
    /// Append a type marker and fingerprint when the name cannot be represented by Core Path.
    /// @param hasher Aggregate repository fingerprint.
    void appendFingerprint(el::Hasher &hasher) const;

private:
    /// Hash regular-file bytes through the native interoperability boundary in bounded chunks.
    [[nodiscard]] auto fileFingerprint() const -> el::ByteBlock;
    /// Translate filesystem errors into recoverable task diagnostics.
    [[noreturn]] void fail(el::String description) const;

private:
    std::filesystem::path _path; ///< Native spelling; never normalized or resolved.
    el::String _relative;        ///< Original Git spelling for diagnostic context.
};

}
