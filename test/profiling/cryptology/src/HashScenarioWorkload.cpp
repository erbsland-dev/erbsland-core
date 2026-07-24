// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "HashScenarioWorkload.hpp"

#include "HashWorkerWorkload.hpp"

#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/profiling/Seed.hpp>

#include <memory>
#include <variant>

namespace app::cryptology {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

void HashScenarioWorkload::prepare(const pf::RunConfiguration &run, const pf::Scenario &scenario) {
    auto algorithmId = el::String{};
    auto inputSize = el::ByteLength{};
    _failWorker = false;
    for (const auto &axis : scenario.axes) {
        if (axis.axis == "algorithm"_el) {
            algorithmId = axis.value;
        }
    }
    for (const auto &parameter : scenario.parameters) {
        if (parameter.parameter == "input-size"_el) {
            inputSize = std::get<el::ByteLength>(parameter.value);
        } else if (parameter.parameter == "fail-worker"_el) {
            _failWorker = std::get<bool>(parameter.value);
        }
    }
    if (algorithmId.isEmpty()) {
        throw el::ApplicationError{"The hash scenario has no algorithm selection."_el};
    }
    if (inputSize.isZero()) {
        throw el::ApplicationError{"The hash input size must be positive."_el};
    }
    if (inputSize > run.memoryLimit) {
        throw el::ApplicationError{"The hash input exceeds the configured memory limit."_el};
    }
    _algorithm = el::cryptology::HashAlgorithm::fromStringOrThrow(algorithmId);
    auto random = el::FastRandom{pf::deriveSeed(run.seed, scenario.id, 0U, 0U)};
    _input = std::make_shared<const el::ByteBuffer>(random.buildByteBuffer(inputSize));
    auto hasher = el::cryptology::Hasher{_algorithm};
    hasher.update(_input->span());
    _expectedDigest = hasher.finalize();
}

auto HashScenarioWorkload::maximumOperations() const noexcept -> std::uint64_t {
    return 1'000'000U;
}

auto HashScenarioWorkload::createWorker([[maybe_unused]] const std::uint32_t worker) -> pf::WorkerWorkloadPtr {
    return std::make_shared<HashWorkerWorkload>(_algorithm, _input, _failWorker);
}

void HashScenarioWorkload::validate(const pf::SampleMeasurement &measurement) {
    if (measurement.operations == 0U) {
        throw el::ApplicationError{"The hash workload completed no operations."_el};
    }
    for (const auto &worker : measurement.workers) {
        if (worker.digest != _expectedDigest) {
            throw el::ApplicationError{
                el::StringFormat{"Hash result mismatch for '{}'."_el}.build(_algorithm.toString())};
        }
    }
}

}
