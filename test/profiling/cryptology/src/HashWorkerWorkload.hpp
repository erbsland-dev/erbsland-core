// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashWorkerWorkload_fwd.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/profiling/WorkerWorkload.hpp>

#include <memory>

namespace app::cryptology {

/// Independent cryptographic hash worker.
/// @notest{Covered by the cryptology profiling smoke tests.}
class HashWorkerWorkload final : public erbsland::profiling::WorkerWorkload {
public:
    /// Create a hashing workload for shared input data.
    HashWorkerWorkload(
        erbsland::cryptology::HashAlgorithm algorithm,
        std::shared_ptr<const erbsland::ByteBuffer> input,
        bool failWorker);

public: // implement WorkerWorkload
    [[nodiscard]] auto execute(const erbsland::profiling::WorkerExecutionContext &context)
        -> erbsland::profiling::WorkerMeasurement override;

private:
    erbsland::cryptology::HashAlgorithm _algorithm;
    std::shared_ptr<const erbsland::ByteBuffer> _input;
    bool _failWorker{};
};

}
