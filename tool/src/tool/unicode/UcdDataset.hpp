// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/ProjectFiles.hpp"

#include <erbsland/CodeLocation.hpp>
#include <erbsland/StringMap.hpp>

#include <exception>

namespace tool::unicode {

/// A versioned, integrity-checked snapshot of the required UCD sources.
/// @tested{UcdDatasetTest IdnaDataFullTest}
class UcdDataset final {
public:
    /// Read and verify all required inputs before generating anything.
    /// @param directory UCD directory containing manifest.elcl.
    /// @param version Expected Unicode version.
    /// @param requiredFiles The generation inputs to verify and retain.
    /// @throws el::ApplicationError On missing inputs, invalid checksums, or inconsistent versions.
    /// @throws el::EncodingError If an input BOM declares an encoding other than UTF-8.
    /// @throws el::conf::ConfError If the manifest cannot be parsed or required values are missing or malformed.
    UcdDataset(const el::Path &directory, const el::String &version, el::StringList requiredFiles = filenames());

public: // accessors
    /// Get the verified input as tolerant UTF-8 with automatic BOM handling.
    /// @param name Required source filename.
    /// @return The retained text.
    /// @throws el::ApplicationError If the name is unknown.
    [[nodiscard]] auto text(const el::String &name) const -> el::String;
    /// Get the absolute source path for one input.
    /// @param name Source filename.
    [[nodiscard]] auto sourcePath(const el::String &name) const -> el::Path;
    /// Get the required source filenames.
    [[nodiscard]] static auto filenames() -> el::StringList;

private:
    /// Require a complete version token in the source header, except for UnicodeData.txt.
    /// @param name The source filename.
    /// @param version The expected Unicode version.
    /// @param text The decoded source text.
    void validateHeader(const el::String &name, const el::String &version, const el::String &text) const;
    /// Report a dataset integrity or input failure.
    /// @param description The reason input verification failed.
    /// @param path The affected input path, or empty for the dataset directory.
    /// @param cause The underlying failure when present.
    /// @param location The source location when known.
    [[noreturn]] void throwDatasetError(
        el::String description,
        const el::Path &path = {},
        const std::exception_ptr &cause = {},
        el::CodeLocation location = {}) const;

private:
    el::Path _directory;              ///< Absolute dataset directory.
    el::StringMap<el::String> _texts; ///< Verified input texts.
};

}
