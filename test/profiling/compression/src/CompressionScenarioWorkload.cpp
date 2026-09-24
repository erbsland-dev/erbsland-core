// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CompressionScenarioWorkload.hpp"

#include "CompressionWorkerWorkload.hpp"
#include "Corpus.hpp"

#include <erbsland/compression/ByteCompressor.hpp>
#include <erbsland/compression/ByteDecompressor.hpp>
#include <erbsland/profiling/RunConfiguration.hpp>
#include <erbsland/stream/StandardStreams.hpp>

#include <algorithm>
#include <limits>
#include <variant>

namespace app::compression {

namespace el = erbsland;
namespace pf = erbsland::profiling;

using namespace el::text::literals;

void CompressionScenarioWorkload::prepare(const pf::RunConfiguration &run, const pf::Scenario &scenario) {
    const auto algorithmId = axisValue(scenario, "algorithm"_el);
    const auto corpusId = axisValue(scenario, "corpus"_el);
    _algorithm = el::compression::CompressionAlgorithm::fromStringOrThrow(algorithmId);
    _format = parseFormat(axisValue(scenario, "format"_el));
    _level = parseLevel(axisValue(scenario, "level"_el));
    auto inputSize = el::ByteLength{};
    for (const auto &parameter : scenario.parameters) {
        if (parameter.parameter == "input-size"_el) {
            inputSize = std::get<el::ByteLength>(parameter.value);
        }
    }
    if (inputSize.isZero()) {
        throw el::ApplicationError{"The compression input size must be positive."_el};
    }
    if (inputSize > run.memoryLimit) {
        throw el::ApplicationError{"The compression input exceeds the configured memory limit."_el};
    }
    const auto corpus = Corpus{};
    const auto *entry = corpus.find(corpusId);
    if (entry == nullptr) {
        throw el::ApplicationError{"The compression scenario selects an unknown corpus."_el};
    }
    _source = std::make_shared<const el::ByteBlock>(corpus.build(*entry, inputSize, run.seed));
    const auto compressor = el::compression::ByteCompressor{_algorithm, _format, _level};
    const auto maximum = compressor.maximumCompressedLength(inputSize);
    if (maximum > run.memoryLimit || inputSize.toRawValue() > run.memoryLimit.toRawValue() - maximum.toRawValue()) {
        throw el::ApplicationError{"The compression fixture bound exceeds the configured memory limit."_el};
    }
    _compressed = std::make_shared<const el::ByteBlock>(compressor.compress(*_source));
    if (_compressed->length() > maximum) {
        throw el::ApplicationError{"A compressor exceeded its advertised maximum output length."_el};
    }
    _options.setMaximumOutputLength(inputSize).setMaximumWorkspaceLength(run.memoryLimit);
    if (_algorithm != el::compression::CompressionAlgorithm::Lzma ||
        _format != el::compression::CompressionFormat::Zip) {
        _options.setExpectedOutputLength(inputSize);
    }
    const auto decompressed = el::compression::ByteDecompressor{_algorithm, _format, _options}.decompress(*_compressed);
    if (decompressed != *_source) {
        throw el::ApplicationError{"Compression corpus round-trip validation failed."_el};
    }
    _expected = _operation == ProfileOperation::Compress ? _compressed : _source;
    validateMemory(run);
    const auto sizeRatio =
        static_cast<double>(_compressed->length().toRawValue()) / static_cast<double>(_source->length().toRawValue());
    el::stream::io::printLine(
        "record=compression-size scenario="_el,
        scenario.id,
        " input-bytes="_el,
        _source->length().toRawValue(),
        " compressed-bytes="_el,
        _compressed->length().toRawValue(),
        " size-ratio="_el,
        sizeRatio,
        " savings-percent="_el,
        100.0 * (1.0 - sizeRatio),
        " reduced="_el,
        sizeRatio < 1.0 ? "true"_el : "false"_el);
}

auto CompressionScenarioWorkload::maximumOperations() const noexcept -> std::uint64_t {
    return 1'000'000U;
}

auto CompressionScenarioWorkload::createWorker([[maybe_unused]] const std::uint32_t worker) -> pf::WorkerWorkloadPtr {
    return std::make_shared<CompressionWorkerWorkload>(
        _operation, _algorithm, _format, _level, _options, _source, _compressed);
}

void CompressionScenarioWorkload::validate(const pf::SampleMeasurement &measurement) {
    if (measurement.operations == 0U || measurement.metrics.count() != el::ItemCount{2U}) {
        throw el::ApplicationError{"The compression workload produced an invalid measurement."_el};
    }
    const auto sourceBytes = measurement.operations * _source->length().toRawValue();
    const auto compressedBytes = measurement.operations * _compressed->length().toRawValue();
    if (measurement.metrics.get(el::ItemIndex{0U}, 0U) != sourceBytes ||
        measurement.metrics.get(el::ItemIndex{1U}, 0U) != compressedBytes) {
        throw el::ApplicationError{"The compression workload byte counters failed validation."_el};
    }
    for (const auto &worker : measurement.workers) {
        if (worker.digest != *_expected) {
            throw el::ApplicationError{"A compression worker produced unexpected bytes."_el};
        }
    }
}

auto CompressionScenarioWorkload::axisValue(const pf::Scenario &scenario, const el::String &axis) -> el::String {
    for (const auto &selection : scenario.axes) {
        if (selection.axis == axis) {
            return selection.value;
        }
    }
    throw el::ApplicationError{el::StringFormat{"A compression scenario has no '{}' selection."_el}.build(axis)};
}

auto CompressionScenarioWorkload::parseFormat(const el::String &value) -> el::compression::CompressionFormat {
    if (value == "raw"_el) {
        return el::compression::CompressionFormat::Raw;
    }
    if (value == "zip"_el) {
        return el::compression::CompressionFormat::Zip;
    }
    if (value == "core"_el) {
        return el::compression::CompressionFormat::Core;
    }
    throw el::ApplicationError{"Unsupported compression format."_el};
}

auto CompressionScenarioWorkload::parseLevel(const el::String &value) -> el::compression::CompressionLevel {
    if (value == "fastest"_el) {
        return el::compression::CompressionLevel::Fastest;
    }
    if (value == "fast"_el) {
        return el::compression::CompressionLevel::Fast;
    }
    if (value == "default"_el) {
        return el::compression::CompressionLevel::Default;
    }
    if (value == "high"_el) {
        return el::compression::CompressionLevel::High;
    }
    if (value == "highest"_el) {
        return el::compression::CompressionLevel::Highest;
    }
    throw el::ApplicationError{"Unsupported compression level."_el};
}

void CompressionScenarioWorkload::validateMemory(const pf::RunConfiguration &run) const {
    const auto limit = run.memoryLimit.toRawValue();
    const auto source = _source->length().toRawValue();
    const auto compressed = _compressed->length().toRawValue();
    if (source > limit || compressed > limit - source) {
        throw el::ApplicationError{"The prepared compression fixtures exceed the configured memory limit."_el};
    }
    const auto shared = source + compressed;
    const auto retainedSamples = run.mode == pf::RunMode::Benchmark ? run.samples : 1U;
    const auto retainedResults =
        static_cast<std::uint64_t>(run.threadCount) * (static_cast<std::uint64_t>(retainedSamples) + 1U);
    const auto resultLength = _expected->length().toRawValue();
    if (retainedResults != 0U && resultLength > (limit - shared) / retainedResults) {
        throw el::ApplicationError{
            "The compression results retained for validation exceed the configured memory limit."_el};
    }
}

}
