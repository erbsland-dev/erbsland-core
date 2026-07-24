// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParserWorkerWorkload_fwd.hpp"

#include <erbsland/profiling/WorkerWorkload.hpp>

namespace app::conf {

/// Independent ELCL parser worker.
/// @tested{WorkloadRunnerTest}
class ParserWorkerWorkload final : public erbsland::profiling::WorkerWorkload {
public:
    ParserWorkerWorkload(erbsland::StringList documents, std::uint64_t corpusBytes, bool failWorker);

public: // implement WorkerWorkload
    [[nodiscard]] auto execute(const erbsland::profiling::WorkerExecutionContext &context)
        -> erbsland::profiling::WorkerMeasurement override;

private:
    erbsland::StringList _documents;
    std::uint64_t _corpusBytes{};
    bool _failWorker{};
};

}
