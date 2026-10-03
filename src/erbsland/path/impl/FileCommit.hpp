// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathInfoData_fwd.hpp"

#include "../Path.hpp"
#include "../PathCommitOptions.hpp"

#include "../../stream/OutputStream.hpp"
#include "../../text/String.hpp"

#include <exception>
#include <functional>
#include <vector>

namespace erbsland::path::impl {

/// Shared, synchronous file finalization, backup retention, and native replacement engine.
/// @tested{PathReplacementTest}
class FileCommit final {
public:
    /// Bind a temporary file and a destination to stable absolute paths.
    /// @param source The owned temporary path.
    /// @param destination The final destination.
    /// @param options The commit policies.
    /// @param settings The output settings used for finalization and backup transfers.
    /// @throws PathError If binding the paths fails.
    FileCommit(Path source, Path destination, PathCommitOptions options, stream::OutputStreamSettings settings);

public: // operations
    /// Validate policies and destination, optionally creating parents.
    /// @throws PathError If validation or parent creation fails.
    void prepare() const;
    /// Finalize output, prepare backups, and replace the destination.
    /// @param stream The backing output stream, or null for an already-finalized temporary file.
    /// @throws PathError If any commit stage fails; the underlying exception is retained.
    void commit(stream::OutputStream *stream);
    /// Add stage and path context without discarding the captured cause.
    /// @param description The failed stage description.
    /// @param source The stage's source path.
    /// @param destination The stage's destination path.
    /// @param cause The original exception, when available.
    /// @throws PathError Always.
    [[noreturn]] static void throwPathError(
        text::String description, const Path &source, const Path &destination, std::exception_ptr cause = {});

private:
    /// Run one stage and retain its original failure as a cause.
    void stage(
        text::String description, const Path &source, const Path &destination, const std::function<void()> &fn) const;
    /// Load fresh native metadata, suppressing only NotFound when absence is allowed.
    auto inspect(const Path &path, bool allowMissing = false) const -> PathInfoData;
    /// Reject unsafe destination types and incompatible collisions.
    auto validateDestination() const -> bool;
    /// Invoke the caller check and refresh destination metadata.
    auto check() const -> bool;
    /// Flush and close output, supporting graceful-close retry.
    void finalize(stream::OutputStream &stream) const;
    /// Stage old content, validate backups, rotate, and install generation one.
    void backups();
    /// Build the reserved path for one numbered generation.
    auto backupPath(std::size_t generation) const -> Path;
    /// Enumerate and validate canonical numbered backup files without following links.
    auto backupGenerations() const -> std::vector<std::size_t>;
    /// Report the failed bounded operation and its configured timeout.
    void timeout(text::String operation, const Path &source, const Path &destination) const;

private:
    Path _source;                           ///< Absolute owned temporary file.
    Path _destination;                      ///< Absolute final destination.
    PathCommitOptions _options;             ///< Replacement and retention policies.
    stream::OutputStreamSettings _settings; ///< Transfer timeouts and buffering.
};

}
