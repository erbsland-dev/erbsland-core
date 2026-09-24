// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Application.hpp"
#include "CompressionScenarioWorkload.hpp"
#include "Corpus.hpp"
#include "DefaultConfiguration.hpp"

#include <erbsland/compression/CompressionAlgorithm.hpp>
#include <erbsland/profiling/AxisSelection.hpp>
#include <erbsland/profiling/ParameterSelection.hpp>
#include <erbsland/profiling/Scenario.hpp>
#include <erbsland/text/StringFormat.hpp>

#include <array>
#include <memory>
#include <utility>

namespace app::compression {

namespace el = erbsland;
namespace pf = erbsland::profiling;

using namespace el::text::literals;

void CompressionProfileApplication::configureProfiling(pf::ProfilingDefinition &definition) {
    const auto corpus = Corpus{};
    auto algorithmAxis = pf::AxisDefinition{"algorithm"_el, "algorithm"_el, "algorithm"_el};
    for (const auto algorithm : el::compression::CompressionAlgorithm::all()) {
        algorithmAxis.addValue({.id = algorithm.toString(), .description = algorithm.toString()});
    }
    auto formatAxis = pf::AxisDefinition{"format"_el, "format"_el, "format"_el};
    formatAxis.addValue({.id = "raw"_el, .description = "Standalone algorithm representation."_el})
        .addValue({.id = "zip"_el, .description = "ZIP entry payload representation."_el})
        .addValue({.id = "core"_el, .description = "Self-describing Erbsland Core envelope."_el});
    auto levelAxis = pf::AxisDefinition{"level"_el, "level"_el, "level"_el};
    levelAxis.addValue({.id = "fastest"_el, .description = "Minimum compression effort."_el})
        .addValue({.id = "fast"_el, .description = "Speed-oriented compression effort."_el})
        .addValue({.id = "default"_el, .description = "Balanced codec-specific effort."_el})
        .addValue({.id = "high"_el, .description = "Size-oriented compression effort."_el})
        .addValue({.id = "highest"_el, .description = "Maximum compression effort."_el});
    auto corpusAxis = pf::AxisDefinition{"corpus"_el, "corpus"_el, "corpus"_el};
    for (const auto &entry : corpus.entries()) {
        corpusAxis.addValue({.id = entry.id, .description = entry.description});
    }

    constexpr auto cSnapshotSize = el::ByteLength{1024U * 1024U};
    constexpr auto cSmokeSize = el::ByteLength{16U * 1024U};
    auto snapshot = el::List<pf::Scenario>{};
    auto levels = el::List<pf::Scenario>{};
    auto formats = el::List<pf::Scenario>{};
    auto full = el::List<pf::Scenario>{};
    auto smoke = el::List<pf::Scenario>{};
    auto windows = el::List<pf::Scenario>{};
    const auto appendScenario = [](el::List<pf::Scenario> &suite,
                                    const el::String &suiteId,
                                    const el::String &functionality,
                                    const el::String &algorithm,
                                    const el::String &format,
                                    const el::String &level,
                                    const el::String &corpus,
                                    const el::ByteLength size) -> void {
        suite.append(
            pf::Scenario{
                .id = el::StringFormat{"{}:{}:{}:{}:{}:{}"_el}.build(
                    suiteId, functionality, algorithm, format, level, corpus),
                .group = functionality,
                .functionality = functionality,
                .axes =
                    el::List<pf::AxisSelection>{
                        {.axis = "algorithm"_el, .value = algorithm},
                        {.axis = "format"_el, .value = format},
                        {.axis = "level"_el, .value = level},
                        {.axis = "corpus"_el, .value = corpus}},
                .parameters = el::List<pf::ParameterSelection>{{.parameter = "input-size"_el, .value = size}}});
    };
    const auto functionalities = el::StringList{"compress"_el, "decompress"_el};
    const auto formatIds = el::StringList{"raw"_el, "zip"_el, "core"_el};
    const auto levelIds = el::StringList{"fastest"_el, "fast"_el, "default"_el, "high"_el, "highest"_el};
    for (const auto algorithm : el::compression::CompressionAlgorithm::all()) {
        const auto algorithmId = algorithm.toString();
        for (const auto &functionality : functionalities) {
            for (const auto &entry : corpus.entries()) {
                appendScenario(
                    snapshot,
                    "snapshot"_el,
                    functionality,
                    algorithmId,
                    "raw"_el,
                    "default"_el,
                    entry.id,
                    cSnapshotSize);
            }
            for (const auto &level : levelIds) {
                appendScenario(
                    levels, "levels"_el, functionality, algorithmId, "raw"_el, level, "mixed"_el, cSnapshotSize);
            }
            if (algorithm != el::compression::CompressionAlgorithm::Lz4Block) {
                auto levelIndex = std::size_t{};
                for (const auto &level : levelIds) {
                    // Four dictionaries/windows, with an 8 MiB floor for block-oriented codecs.
                    constexpr auto cLzmaSizes = std::array{8U, 8U, 32U, 64U, 128U};
                    constexpr auto cZstandardSizes = std::array{8U, 8U, 8U, 32U, 128U};
                    auto mebibytes = 8U;
                    if (algorithm == el::compression::CompressionAlgorithm::Lzma) {
                        mebibytes = cLzmaSizes[levelIndex];
                    } else if (algorithm == el::compression::CompressionAlgorithm::Zstandard) {
                        mebibytes = cZstandardSizes[levelIndex];
                    }
                    for (const auto &corpusId : el::StringList{"matches"_el, "entropy"_el, "mixed"_el}) {
                        appendScenario(
                            windows,
                            "windows"_el,
                            functionality,
                            algorithmId,
                            "raw"_el,
                            level,
                            corpusId,
                            el::ByteLength{mebibytes * 1024U * 1024U});
                    }
                    ++levelIndex;
                }
            }
            for (const auto &format : formatIds) {
                if (algorithm == el::compression::CompressionAlgorithm::Lz4Block && format == "zip"_el) {
                    continue;
                }
                appendScenario(
                    formats,
                    "formats"_el,
                    functionality,
                    algorithmId,
                    format,
                    "default"_el,
                    "structured"_el,
                    cSnapshotSize);
            }
            for (const auto &format : formatIds) {
                if (algorithm == el::compression::CompressionAlgorithm::Lz4Block && format == "zip"_el) {
                    continue;
                }
                for (const auto &level : levelIds) {
                    for (const auto &entry : corpus.entries()) {
                        appendScenario(
                            full, "full"_el, functionality, algorithmId, format, level, entry.id, cSnapshotSize);
                    }
                }
            }
            appendScenario(
                smoke, "smoke"_el, functionality, algorithmId, "raw"_el, "default"_el, "mixed"_el, cSmokeSize);
        }
    }
    for (const auto &functionality : functionalities) {
        appendScenario(
            smoke, "smoke"_el, functionality, "lz4-block"_el, "core"_el, "default"_el, "structured"_el, cSmokeSize);
        appendScenario(
            smoke, "smoke"_el, functionality, "lzma"_el, "zip"_el, "default"_el, "structured"_el, cSmokeSize);
        for (const auto &entry : corpus.entries()) {
            if (entry.id == "mixed"_el || entry.id == "structured"_el) {
                continue;
            }
            appendScenario(
                smoke, "smoke"_el, functionality, "deflate"_el, "raw"_el, "default"_el, entry.id, cSmokeSize);
        }
    }

    definition
        .setApplication(
            "Compression Profiling"_el,
            el::Version{1, 0, 0},
            "Compression Algorithm Profiler and Benchmark"_el,
            "Profiles deterministic one-shot compression and decompression workloads."_el)
        .setDefaultConfiguration(el::String{cDefaultConfigurationText})
        .addAxis(std::move(algorithmAxis))
        .addAxis(std::move(formatAxis))
        .addAxis(std::move(levelAxis))
        .addAxis(std::move(corpusAxis))
        .addParameter(
            {.id = "input-size"_el,
                .configurationName = "input_size"_el,
                .type = pf::ParameterType::ByteLength,
                .defaultValue = cSnapshotSize})
        .addMetric({.id = "uncompressed-bytes"_el, .unit = "bytes"_el, .reportRate = true})
        .addMetric({.id = "compressed-bytes"_el, .unit = "bytes"_el, .reportRate = true})
        .addFunctionality(
            {.id = "compress"_el,
                .description = "Compress a complete deterministic byte block."_el,
                .api = el::StringList{"compression::ByteCompressor::compress"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<CompressionScenarioWorkload>(ProfileOperation::Compress);
                }})
        .addFunctionality(
            {.id = "decompress"_el,
                .description = "Decompress a complete prepared byte block."_el,
                .api = el::StringList{"compression::ByteDecompressor::decompress"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<CompressionScenarioWorkload>(ProfileOperation::Decompress);
                }})
        .setCompatibility([](const pf::Scenario &scenario) -> bool {
            auto algorithm = el::String{};
            auto format = el::String{};
            for (const auto &axis : scenario.axes) {
                if (axis.axis == "algorithm"_el) {
                    algorithm = axis.value;
                } else if (axis.axis == "format"_el) {
                    format = axis.value;
                }
            }
            return algorithm != "lz4-block"_el || format != "zip"_el;
        })
        .addSuite({.id = "snapshot"_el, .scenarios = std::move(snapshot)})
        .addSuite({.id = "levels"_el, .scenarios = std::move(levels)})
        .addSuite({.id = "formats"_el, .scenarios = std::move(formats)})
        .addSuite({.id = "full"_el, .scenarios = std::move(full)})
        .addSuite({.id = "smoke"_el, .scenarios = std::move(smoke)})
        .addSuite({.id = "windows"_el, .scenarios = std::move(windows)});
}

auto CompressionProfileApplication::executeProfiling() -> el::ExitCode {
    return runRegisteredProfiling();
}

}
