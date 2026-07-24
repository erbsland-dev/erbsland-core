// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "FakeScenarioWorkload.hpp"

#include "FakeWorkerWorkload.hpp"

namespace pf = erbsland::profiling;

FakeScenarioWorkload::FakeScenarioWorkload(
    std::shared_ptr<std::atomic<std::uint64_t>> executions,
    std::shared_ptr<std::atomic<std::uint64_t>> validations,
    const bool fail) :
    _executions{std::move(executions)}, _validations{std::move(validations)}, _fail{fail} {
}

void FakeScenarioWorkload::prepare(
    [[maybe_unused]] const pf::RunConfiguration &run, [[maybe_unused]] const pf::Scenario &scenario) {
}

auto FakeScenarioWorkload::maximumOperations() const noexcept -> std::uint64_t {
    return 2U;
}

auto FakeScenarioWorkload::createWorker([[maybe_unused]] const std::uint32_t worker) -> pf::WorkerWorkloadPtr {
    return std::make_shared<FakeWorkerWorkload>(_executions, _fail);
}

void FakeScenarioWorkload::validate([[maybe_unused]] const pf::SampleMeasurement &measurement) {
    _validations->fetch_add(1U);
}
