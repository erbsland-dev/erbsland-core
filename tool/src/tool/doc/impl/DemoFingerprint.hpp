// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../frame/ProjectFiles.hpp"

#include <erbsland/Hasher.hpp>
#include <erbsland/StringLiteral.hpp>
#include <erbsland/StringMap.hpp>

namespace tool::doc::impl {

/// Hash typed, length-framed demo inputs without constructing an intermediate serialization.
class DemoFingerprint final {
public:
    /// Start a fingerprint using the current format version.
    /// @param files Confined project file access.
    explicit DemoFingerprint(const frame::ProjectFiles &files);

public: // input
    /// Add ordered option records, excluding the generated sha256 value.
    /// @param options Canonical author metadata.
    void addOptions(const el::StringMap<el::String> &options);
    /// Add one typed file record after checking its input size.
    /// @param kind Record kind: source, fixture, or execution-file.
    /// @param path Confined file to read.
    /// @param name Original argument identifying the input; empty for the source record.
    void addFile(el::StringLiteral kind, const el::Path &path, const el::String &name = {});
    /// Complete the fingerprint and return its hexadecimal digest.
    [[nodiscard]] auto finish() -> el::String;

private:
    /// Add a byte count in the common fixed-width framing format.
    void addLength(el::ByteCount count);
    /// Add raw UTF-8 bytes with their length, without escaping or normalization.
    void addText(const el::String &text);

private:
    const frame::ProjectFiles &_files; ///< Confined input service.
    el::Hasher _hasher;                ///< Digest state for the ordered input records.
};

}
