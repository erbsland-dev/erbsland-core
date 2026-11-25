// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GitStateSnapshot.hpp"

#include <erbsland/Path.hpp>
#include <erbsland/StringList.hpp>
#include <tool/frame/TaskProgress.hpp>

#include <optional>

namespace tool::pre_commit {

/// Capture and compare a run's initial and final repository states without touching the index.
class GitState final {
public:
    /// Bind the repository root; Git is resolved lazily inside the capture task.
    /// @param projectRoot Absolute repository directory.
    explicit GitState(el::Path projectRoot);

public: // snapshots
    /// Capture and retain the baseline, replacing any earlier snapshot.
    /// @param progress Optional progress updates.
    /// @throws el::RuntimeError If Git or filesystem access fails.
    void capture(const frame::TaskProgress &progress = {});
    /// Compare the current state with the retained baseline.
    /// @param progress Optional progress updates.
    /// @throws el::ApplicationError If the baseline is absent or the repository changed.
    void verify(const frame::TaskProgress &progress = {}) const;
    /// Read a standalone snapshot without replacing the baseline.
    /// @param progress Optional progress updates.
    /// @return Byte-sensitive state for comparison.
    /// @throws el::RuntimeError If Git or filesystem access fails.
    [[nodiscard]] auto snapshot(const frame::TaskProgress &progress = {}) const -> GitStateSnapshot;

private:
    /// Hash an untracked regular file in bounded chunks without text decoding.
    [[nodiscard]] static auto fileFingerprint(const el::Path &path) -> el::ByteBlock;

    /// Run Git and retain its machine output without text transformations.
    [[nodiscard]] auto gitOutput(const el::StringList &arguments) const -> el::String;
    /// Fingerprint untracked file kinds and contents, without following final symbolic links.
    [[nodiscard]] auto untrackedFingerprints(const frame::TaskProgress &progress) const -> el::ByteBlock;

private:
    el::Path _root;                           ///< Repository working directory.
    mutable el::Path _git;                    ///< First executable resolved from PATH, reused throughout the run.
    std::optional<GitStateSnapshot> _initial; ///< Absent when initial capture failed.
};

}
