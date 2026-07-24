// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathWalkerWorkerWorkload_fwd.hpp"
#include "PathWalkMethod.hpp"

#include <erbsland/profiling/WorkerWorkload.hpp>

namespace app::path {

/// Independent worker that repeatedly traverses one directory tree.
/// @notest{Covered by manual profiling runs and the PathWalker unit tests.}
class PathWalkerWorkerWorkload final : public erbsland::profiling::WorkerWorkload {
public:
    PathWalkerWorkerWorkload(PathWalkMethod method, erbsland::Path root);

public: // implement WorkerWorkload
    [[nodiscard]] auto execute(const erbsland::profiling::WorkerExecutionContext &context)
        -> erbsland::profiling::WorkerMeasurement override;

private:
    [[nodiscard]] auto walkPathCallback() const -> std::uint64_t;
    [[nodiscard]] auto walkPathInfoCallback() const -> std::uint64_t;
    [[nodiscard]] auto walkStdRecursive() const -> std::uint64_t;

private:
    PathWalkMethod _method;
    erbsland::Path _root;
};

}
