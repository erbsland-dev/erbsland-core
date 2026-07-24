// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "FakeWorkerWorkload.hpp"

#include <erbsland/profiling/WorkerExecutionContext.hpp>

namespace pf = erbsland::profiling;

using namespace el::text::literals;

FakeWorkerWorkload::FakeWorkerWorkload(std::shared_ptr<std::atomic<std::uint64_t>> executions, const bool fail) :
    _executions{std::move(executions)}, _fail{fail} {
}

auto FakeWorkerWorkload::execute(const pf::WorkerExecutionContext &context) -> pf::WorkerMeasurement {
    if (_fail) {
        throw el::ApplicationError{el::String{"Requested fake worker failure."_el}};
    }
    _executions->fetch_add(1U);
    return pf::WorkerMeasurement{.operations = context.operations};
}
