// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ManifestEntry.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/StopSource.hpp>
#include <erbsland/UnmanagedEventThread.hpp>

namespace demo {

/// Checks one manifest in order. The owner loop decides whether to admit each next file.
class VerificationSession final {
public:
    /// Prepare a session for one run.
    /// @param manifest Absolute path used to resolve file names.
    /// @param failFast Stop at the first checksum mismatch.
    VerificationSession(const el::Path &manifest, bool failFast);
    /// Run the owner and worker, then drain the report and join the worker.
    /// @return True if every entry matched.
    /// @throws el::RuntimeError If a file cannot be read or a callback fails.
    auto run() -> bool;

private:
    void schedule(el::ItemIndex index);
    void check(el::ItemIndex index, const el::StopToken &stop);
    void record(el::ItemIndex index, bool matches);
    void finish(std::exception_ptr error);
    void printSummary() const;

private:
    el::Path _directory;                 ///< Base directory for manifest file names.
    el::List<ManifestEntry> _entries;    ///< Immutable after construction.
    bool _failFast;                      ///< Owner policy for mismatches.
    el::StopSource _cancellation;        ///< Stop request for this verification.
    el::EventLoopPtr _owner;             ///< Receives decisions and the final report.
    el::UnmanagedEventThreadPtr _worker; ///< Hashes at most one file per command.
    el::ItemCount _checked;              ///< Entries with a delivered checksum result.
    el::ItemCount _mismatches;           ///< Failed comparisons.
    std::exception_ptr _error;           ///< Worker failure to surface after the report drains.
};

}
