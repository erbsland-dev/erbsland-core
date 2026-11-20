// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/List.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

namespace demo {

/// One checksum and its manifest-relative file name.
class ManifestEntry final {
public:
    /// Create a new manifest entry.
    /// @param digest Expected digest of the file in hexadecimal.
    /// @param name Relative path to the file from the manifest directory.
    ManifestEntry(el::String digest, el::Path name);

public: // accessors
    /// The manifest-relative file name printed in the report.
    [[nodiscard]] auto name() const noexcept -> const el::Path & { return _name; }

public: // verification
    /// Hash the file and compare its digest.
    /// @param directory Directory containing the manifest.
    /// @return True if the file matches the manifest.
    /// @throws el::RuntimeError If the file cannot be read.
    [[nodiscard]] auto matchesFile(const el::Path &directory) const -> bool;

public: // factories
    /// Read a UTF-8 manifest containing one hexadecimal digest, two spaces and a file name per line.
    /// @param path Manifest to load.
    /// @return Entries in verification order.
    /// @throws el::RuntimeError If the file cannot be read or a record is malformed.
    static auto read(const el::Path &path) -> el::List<ManifestEntry>;

private:
    el::String _digest; ///< Expected SHA-256 in hexadecimal.
    el::Path _name;     ///< File name relative to the manifest.
};

}
