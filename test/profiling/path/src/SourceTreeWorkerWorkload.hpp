// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SourceReadMethod.hpp"
#include "SourceTreeWorkerWorkload_fwd.hpp"

#include <erbsland/Path.hpp>
#include <erbsland/profiling/WorkerWorkload.hpp>

#include <cstdint>
#include <stop_token>

namespace app::path {

/// One independent caller repeatedly discovering and reading a source tree.
class SourceTreeWorkerWorkload final : public erbsland::profiling::WorkerWorkload {
private:
    /// Counts retained for one pass or the complete worker batch.
    struct Counters {
        std::uint64_t entries{};       ///< Reported directories and selected files.
        std::uint64_t files{};         ///< Completely read files.
        std::uint64_t returnedBytes{}; ///< Bytes returned by the selected API.
    };

public:
    /// Create a worker for one source tree.
    /// @param method Whole-file read API to exercise.
    /// @param root Directory to rediscover on every pass.
    SourceTreeWorkerWorkload(SourceReadMethod method, el::Path root);

public: // implement WorkerWorkload
    [[nodiscard]] auto execute(const erbsland::profiling::WorkerExecutionContext &context)
        -> erbsland::profiling::WorkerMeasurement override;

private:
    /// Walk the tree, returning false if cancellation interrupted this pass.
    [[nodiscard]] auto walkAndRead(Counters &counters, std::stop_token stopToken) const -> bool;
    /// Read a whole source file as tolerant UTF-8; return the payload byte count.
    [[nodiscard]] auto readTextFile(const el::Path &path) const -> std::uint64_t;
    /// Read a whole source file; return its byte count.
    [[nodiscard]] auto readDataFile(const el::Path &path) const -> std::uint64_t;

private:
    SourceReadMethod _method; ///< API used for each file.
    el::Path _root;           ///< Absolute directory to traverse.
};

}
