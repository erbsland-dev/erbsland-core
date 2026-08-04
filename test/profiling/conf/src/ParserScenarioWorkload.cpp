// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserScenarioWorkload.hpp"

#include "EmbeddedDocuments.hpp"
#include "ParserWorkerWorkload.hpp"

#include <variant>

namespace app::conf {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

void ParserScenarioWorkload::prepare([[maybe_unused]] const pf::RunConfiguration &run, const pf::Scenario &scenario) {
    _documents.clear();
    _corpusBytes = 0U;
    _failWorker = false;
    for (const auto &parameter : scenario.parameters) {
        if (parameter.parameter == "fail-worker"_el) {
            _failWorker = std::get<bool>(parameter.value);
        }
    }
    for (const auto &document : embeddedDocuments()) {
        _documents.append(el::String{document.text});
        _corpusBytes += document.text.length().toRawValue();
    }
}

auto ParserScenarioWorkload::maximumOperations() const noexcept -> std::uint64_t {
    return 1'000'000U;
}

auto ParserScenarioWorkload::createWorker([[maybe_unused]] const std::uint32_t worker) -> pf::WorkerWorkloadPtr {
    return std::make_shared<ParserWorkerWorkload>(_documents, _corpusBytes, _failWorker);
}

void ParserScenarioWorkload::validate(const pf::SampleMeasurement &measurement) {
    if (measurement.operations == 0U) {
        throw el::ApplicationError{"The parser workload completed no operations."_el};
    }
}

}
