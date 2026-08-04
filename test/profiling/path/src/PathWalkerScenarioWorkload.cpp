// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathWalkerScenarioWorkload.hpp"

#include "PathWalkerWorkerWorkload.hpp"

#include <erbsland/path/PathInfo.hpp>

#include <filesystem>
#include <memory>
#include <variant>

namespace app::path {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

PathWalkerScenarioWorkload::PathWalkerScenarioWorkload(const PathWalkMethod method) noexcept : _method{method} {
}

void PathWalkerScenarioWorkload::prepare(
    [[maybe_unused]] const pf::RunConfiguration &run, const pf::Scenario &scenario) {
    _root = el::Path{"."_el};
    for (const auto &parameter : scenario.parameters) {
        if (parameter.parameter == "root"_el) {
            _root = std::get<el::Path>(parameter.value);
        }
    }
    const auto rootInfo = _root.info();
    if (!rootInfo.isDirectory()) {
        throw el::ApplicationError{
            el::StringFormat{"Profiling root '{}' is not a directory."_el}.build(_root.toString())};
    }
    _root = rootInfo.resolvedPath();

    _expectedEntryCount = 1U;
    auto error = std::error_code{};
    const auto options = std::filesystem::directory_options::skip_permission_denied;
    for (
        auto iterator = std::filesystem::recursive_directory_iterator{_root.toStdPath(), options, error};
        iterator != std::filesystem::recursive_directory_iterator{};
        iterator.increment(error)) {
        if (error) {
            throw el::ApplicationError{"The profiling root could not be scanned completely."_el};
        }
        ++_expectedEntryCount;
    }
    if (error) {
        throw el::ApplicationError{"The profiling root could not be scanned completely."_el};
    }
}

auto PathWalkerScenarioWorkload::maximumOperations() const noexcept -> std::uint64_t {
    return 1'000'000U;
}

auto PathWalkerScenarioWorkload::createWorker([[maybe_unused]] const std::uint32_t worker) -> pf::WorkerWorkloadPtr {
    return std::make_shared<PathWalkerWorkerWorkload>(_method, _root);
}

void PathWalkerScenarioWorkload::validate(const pf::SampleMeasurement &measurement) {
    if (measurement.operations == 0U || measurement.metrics.count() != el::ItemCount::one()) {
        throw el::ApplicationError{"The path-walker workload produced an invalid measurement."_el};
    }
    if (measurement.metrics.first() != measurement.operations * _expectedEntryCount) {
        throw el::ApplicationError{"The path-walker workload returned an unexpected entry count."_el};
    }
}

}
