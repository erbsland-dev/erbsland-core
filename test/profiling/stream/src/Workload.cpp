// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Workload.hpp"

#include "impl/WorkloadTools.hpp"

namespace app::stream {

using namespace el::text::literals;
using namespace impl;

WorkloadRunner::WorkloadRunner(Configuration configuration) : _configuration{std::move(configuration)} {
}

void WorkloadRunner::printDryRun() const {
    auto corpusBytes = std::uint64_t{};
    for (const auto &scenario : _configuration.scenarios) {
        const auto bufferSizes = el::stream::impl::streamBufferSizes(scenario.buffering);
        const auto effectiveBackLimit =
            scenario.backBufferLimit != 0U ? scenario.backBufferLimit : bufferSizes.outputBackLimit.toRawValue();
        const auto fileSize = selectedFileSize(_configuration, scenario);
        const auto slots = slotCount(_configuration, scenario, fileSize);
        const auto shared = scenario.direction == Direction::Read && scenario.locality == Locality::Hot;
        const auto copies = shared ? 1U : static_cast<std::uint64_t>(_configuration.run.threadCount) * slots;
        const auto physicalSize = estimatedPhysicalFileSize(scenario, fileSize);
        corpusBytes = std::max(corpusBytes, physicalSize * copies);
        el::io::printLine(
            "record=scenario id="_el,
            scenario.id,
            " direction="_el,
            toString(scenario.direction),
            " type="_el,
            toString(scenario.fileType),
            " method="_el,
            toString(scenario.method),
            " locality="_el,
            toString(scenario.locality),
            " chunk-mode="_el,
            toString(scenario.chunkMode),
            " file-min="_el,
            scenario.fileSizeMinimum,
            " file-max="_el,
            scenario.fileSizeMaximum,
            " selected-file-size="_el,
            fileSize,
            " files-per-worker="_el,
            slots,
            " threads="_el,
            _configuration.run.threadCount,
            " buffering="_el,
            toString(scenario.buffering),
            " ring="_el,
            bufferSizes.ioRing.toRawValue(),
            " aggregate="_el,
            bufferSizes.aggregateChunk.toRawValue(),
            " decoder="_el,
            bufferSizes.decoder.toRawValue(),
            " output-retained-initial="_el,
            bufferSizes.outputRetainedInitial.toRawValue(),
            " back-limit="_el,
            effectiveBackLimit);
        for (auto worker = std::uint32_t{}; worker < _configuration.run.threadCount; ++worker) {
            for (auto slot = std::uint32_t{}; slot < slots; ++slot) {
                const auto fileWorker = shared ? 0U : worker;
                const auto fileSlot = shared ? 0U : slot;
                el::io::printLine(
                    "record=scenario assignment=true scenario="_el,
                    scenario.id,
                    " worker="_el,
                    worker,
                    " file-index="_el,
                    fileSlot,
                    " assignment-kind="_el,
                    shared ? "shared"_el : "unique"_el,
                    " seed="_el,
                    scenarioSeed(_configuration.run.seed, scenario, fileWorker, fileSlot));
            }
        }
    }
    el::io::printLine(
        "record=summary action=dry-run scenarios="_el,
        _configuration.scenarios.size(),
        " threads="_el,
        _configuration.run.threadCount,
        " maximum-active-corpus-bytes="_el,
        corpusBytes,
        " config-md5="_el,
        el::ByteFormat::compact(),
        configurationDigest(_configuration));
}

auto WorkloadRunner::run() -> el::ExitCode {
    auto temporaryOptions = el::PathTempDirectoryOptions{};
    temporaryOptions.setPrefix("stream-file-profile-"_el).setRandomLength(el::CpLength{12U});
    auto workspaceRoot = el::Path::systemTempDirectoryOrThrow();
    if (!_configuration.run.workspace.isEmpty()) {
        workspaceRoot = el::Path{_configuration.run.workspace};
        if (!workspaceRoot.isValid()) {
            workloadError("The configured workspace is not a valid path."_el);
        }
        auto directoryOptions = el::PathCreateDirectoryOptions{};
        directoryOptions.setCreateParents(true).setCreationMode(el::PathCreateMode::CreateOrOverwrite);
        workspaceRoot.operations().createDirectoryOrThrow(directoryOptions);
    }
    auto workspace = workspaceRoot.operations().createTempDirectoryOrThrow(temporaryOptions);
    if (_configuration.run.keepFiles) {
        workspace->setRemoveOnDestroy(false);
    }
    el::io::printLine(
        "record=run profile=stream-file mode="_el,
        toString(_configuration.run.mode),
        " workload-threads="_el,
        _configuration.run.threadCount,
        " io-workers=process-wide-dynamic"_el,
        " seed="_el,
        _configuration.run.seed,
        " scenarios="_el,
        _configuration.scenarios.size(),
        " duration-ns="_el,
        _configuration.run.duration.count(),
        " workspace="_el,
        workspace->path().toString(),
        " config-md5="_el,
        el::ByteFormat::compact(),
        configurationDigest(_configuration));

    const auto runStart = Clock::now();
    const auto deadline = runStart + _configuration.run.duration;
    auto nextProgress = runStart + _configuration.run.progressInterval;
    auto totalBytes = std::uint64_t{};
    auto totalSamples = std::uint64_t{};
    auto measuredScenarios = std::set<el::String>{};
    for (const auto &scenario : _configuration.scenarios) {
        auto prepared = prepareScenario(_configuration, scenario, workspace);
        const auto validation = runSample(prepared, _configuration, 0U, true);
        if (_configuration.run.mode == RunMode::Profile) {
            printCoverage(scenario, validation);
            if (Clock::now() < deadline) {
                const auto sample = runSample(prepared, _configuration, 1U, false);
                printSample(_configuration, scenario, sample);
                totalBytes += sample.bytes;
                ++totalSamples;
                measuredScenarios.emplace(scenario.id);
                if (scenario.direction == Direction::Write) {
                    for (auto worker = std::uint32_t{}; worker < _configuration.run.threadCount; ++worker) {
                        validateFixtureOutput(prepared, worker, 1U);
                    }
                }
            }
            continue;
        }
        auto samples = std::vector<SampleResult>{};
        while (samples.size() < 5U && Clock::now() < deadline) {
            samples.emplace_back(runSample(prepared, _configuration, samples.size() + 1U, false));
            totalBytes += samples.back().bytes;
            ++totalSamples;
        }
        measuredScenarios.emplace(scenario.id);
        if (samples.size() < 5U) {
            workloadError(
                "The benchmark deadline was reached before five samples were collected for every scenario; "
                "increase the duration or select fewer scenarios."_el);
        }
        printBenchmark(_configuration, scenario, samples);
        if (scenario.direction == Direction::Write) {
            validateAllOutputs(prepared);
        }
    }

    if (_configuration.run.mode == RunMode::Profile) {
        auto sampleIndex = std::uint64_t{1U};
        while (Clock::now() < deadline) {
            for (const auto &scenario : _configuration.scenarios) {
                if (Clock::now() >= deadline) {
                    break;
                }
                auto prepared = prepareScenario(_configuration, scenario, workspace);
                auto lastSampleIndex = std::uint64_t{};
                for (auto repeat = std::uint32_t{}; repeat < scenario.weight && Clock::now() < deadline; ++repeat) {
                    lastSampleIndex = sampleIndex++;
                    const auto sample = runSample(prepared, _configuration, lastSampleIndex, false);
                    if (Clock::now() >= nextProgress) {
                        printSample(_configuration, scenario, sample);
                        nextProgress = Clock::now() + _configuration.run.progressInterval;
                    }
                    totalBytes += sample.bytes;
                    ++totalSamples;
                    measuredScenarios.emplace(scenario.id);
                }
                if (scenario.direction == Direction::Write && lastSampleIndex != 0U) {
                    for (auto worker = std::uint32_t{}; worker < _configuration.run.threadCount; ++worker) {
                        validateFixtureOutput(prepared, worker, lastSampleIndex);
                    }
                }
            }
        }
    } else {
        auto sampleIndex = std::uint64_t{1000U};
        while (Clock::now() < deadline) {
            for (const auto &scenario : _configuration.scenarios) {
                if (Clock::now() >= deadline) {
                    break;
                }
                auto prepared = prepareScenario(_configuration, scenario, workspace);
                const auto currentSampleIndex = sampleIndex++;
                const auto sample = runSample(prepared, _configuration, currentSampleIndex, false);
                if (Clock::now() >= nextProgress) {
                    printSample(_configuration, scenario, sample);
                    nextProgress = Clock::now() + _configuration.run.progressInterval;
                }
                totalBytes += sample.bytes;
                ++totalSamples;
                measuredScenarios.emplace(scenario.id);
                if (scenario.direction == Direction::Write) {
                    for (auto worker = std::uint32_t{}; worker < _configuration.run.threadCount; ++worker) {
                        validateFixtureOutput(prepared, worker, currentSampleIndex);
                    }
                }
            }
        }
    }
    el::io::printLine(
        "record=summary mode="_el,
        toString(_configuration.run.mode),
        " workload-threads="_el,
        _configuration.run.threadCount,
        " samples="_el,
        totalSamples,
        " covered-scenarios="_el,
        _configuration.scenarios.size(),
        " measured-scenarios="_el,
        measuredScenarios.size(),
        " bytes="_el,
        totalBytes,
        " elapsed-ns="_el,
        elapsedNanoseconds(runStart),
        " workspace="_el,
        workspace->path().toString(),
        " kept="_el,
        _configuration.run.keepFiles);
    return el::ExitCode::success();
}

}
