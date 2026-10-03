// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SourceTreeScenarioWorkload.hpp"

#include "SourceTreeWorkerWorkload.hpp"

#include "impl/SourceTreeSelection.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathType.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringFormat.hpp>

#include <filesystem>
#include <memory>
#include <variant>

namespace app::path {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

SourceTreeScenarioWorkload::SourceTreeScenarioWorkload(const SourceReadMethod method) noexcept : _method{method} {
}

void SourceTreeScenarioWorkload::prepare(
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
            el::StringFormat{"Source-tree root '{}' is not a directory."_el}.build(_root.toString())};
    }
    _root = rootInfo.resolvedPath();
    inventory();
    if (_expectedFileCount == 0U) {
        throw el::ApplicationError{
            el::StringFormat{"Source-tree root '{}' contains no selected C++ files."_el}.build(_root.toString())};
    }
    auto byteKind = "file-data"_el;
    if (_method == SourceReadMethod::Text) {
        byteKind = "decoded-utf8"_el;
    }
    el::io::printLine(
        "record=corpus scenario="_el,
        scenario.id,
        " root=\""_el,
        _root.toString(),
        "\" files="_el,
        _expectedFileCount,
        " input-bytes="_el,
        _expectedByteCount,
        " returned-byte-kind="_el,
        byteKind);
}

void SourceTreeScenarioWorkload::inventory() {
    _expectedFileCount = 0U;
    _expectedByteCount = 0U;
    if (!impl::shouldDescendSourceTree(_root)) {
        return;
    }
    for (
        auto iterator = std::filesystem::recursive_directory_iterator{_root.toStdPath()};
        iterator != std::filesystem::recursive_directory_iterator{};
        ++iterator) {
        const auto path = el::Path{iterator->path()};
        const auto status = iterator->symlink_status();
        if (std::filesystem::is_directory(status)) {
            if (!impl::shouldDescendSourceTree(path)) {
                iterator.disable_recursion_pending();
            }
        } else if (std::filesystem::is_regular_file(status) && impl::isSourceFile(path, el::PathType::RegularFile)) {
            ++_expectedFileCount;
            _expectedByteCount += iterator->file_size();
        }
    }
}

auto SourceTreeScenarioWorkload::maximumOperations() const noexcept -> std::uint64_t {
    // One complete tree per sample bounds deadline overshoot and keeps corpus counts directly readable.
    return 1U;
}

auto SourceTreeScenarioWorkload::createWorker([[maybe_unused]] const std::uint32_t worker) -> pf::WorkerWorkloadPtr {
    return std::make_shared<SourceTreeWorkerWorkload>(_method, _root);
}

void SourceTreeScenarioWorkload::validate(const pf::SampleMeasurement &measurement) {
    if (measurement.operations == 0U || measurement.metrics.count() != el::ItemCount{3U}) {
        throw el::ApplicationError{"The source-tree workload produced an invalid measurement."_el};
    }
    if (measurement.metrics.get(el::ItemIndex{1U}) != measurement.operations * _expectedFileCount) {
        throw el::ApplicationError{"The source-tree file count changed during profiling."_el};
    }
    if (_method == SourceReadMethod::Data &&
        measurement.metrics.get(el::ItemIndex{2U}) != measurement.operations * _expectedByteCount) {
        throw el::ApplicationError{"The source-tree byte count changed during profiling."_el};
    }
}

}
