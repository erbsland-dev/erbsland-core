// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Input.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/List.hpp>
#include <tool/frame/ProjectFiles.hpp>
#include <tool/frame/TaskProgress.hpp>

namespace tool::security_hashes {

/// Generate and publish a versioned, line-ending-independent manifest.
class Manifest final {
public:
    /// Bind the manifest to its project file service.
    /// @param files Project service, which must outlive this object and its worker jobs.
    /// @param toolName Maintenance command that owns the manifest.
    Manifest(const frame::ProjectFiles &files, el::String toolName);

public: // generation
    /// Hash unique inputs concurrently and assemble sorted records.
    /// @param inputs Validated inventory with unique paths in binary sort order.
    /// @param progress Optional task progress.
    /// @param workers Maximum concurrent file reads and hashes, at least one.
    /// @return Complete LF-terminated manifest text.
    /// @throws el::ApplicationError If any source cannot be read safely.
    [[nodiscard]] auto generate(
        const el::List<Input> &inputs, unsigned int workers, const frame::TaskProgress &progress = {}) const
        -> el::String;
    /// Compare or atomically publish the manifest after all workers have succeeded.
    /// @param output Confined destination with an existing parent.
    /// @param text Complete generated manifest.
    /// @param verify Only compare, never write.
    /// @return True if the manifest needed updating.
    /// @throws el::ApplicationError On verification mismatch or file access failure.
    auto publish(const el::Path &output, const el::String &text, bool verify) const -> bool;

private:
    /// Hash one selected file, preserving its path in any runtime diagnostic.
    [[nodiscard]] auto hash(const Input &input) const -> el::ByteBlock;
    /// Encode a digest as lowercase hexadecimal.
    [[nodiscard]] static auto hex(const el::ByteBlock &digest) -> el::String;
    /// Append one field and its NUL delimiter.
    static void field(el::Hasher &hasher, const el::String &text);

private:
    el::String _toolName;              ///< Tool named in the generated banner.
    const frame::ProjectFiles &_files; ///< Immutable project service shared by worker jobs.
};

}
