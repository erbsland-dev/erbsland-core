// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WorkloadRunner.hpp"

#include "BuildIdentity.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteFormat.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathCreateDirectoryOptions.hpp>
#include <erbsland/PathCreateMode.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PathTempDirectoryOptions.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/TempDirectory.hpp>

namespace app::regex::impl {

void WorkloadRunner::printDryRun() const {
    for (const auto &scenario : _configuration.scenarios) {
        el::io::printLine(
            "record=scenario id="_el,
            scenario.id,
            " group="_el,
            scenario.group,
            " use-case="_el,
            toString(scenario.useCase),
            " input="_el,
            toString(scenario.inputKind),
            " backend="_el,
            toString(scenario.backend),
            " replacement="_el,
            toString(scenario.replacementMode),
            " pattern="_el,
            scenario.patternName,
            " pattern-code-points="_el,
            scenario.pattern.characterLength(),
            " corpus="_el,
            scenario.corpusName,
            " corpus-source="_el,
            scenario.corpusSource == CorpusSource::File ? "file"_el : "inline"_el,
            " repeat="_el,
            scenario.repetitionCount,
            " timeout-ms="_el,
            scenario.timeout.count(),
            " weight="_el,
            scenario.weight);
    }
    el::io::printLine(
        "record=summary action=dry-run scenarios="_el,
        _configuration.scenarios.size(),
        " threads="_el,
        _configuration.run.threadCount,
        " suite="_el,
        _configuration.run.suite,
        " duration-ns="_el,
        _configuration.run.duration.count(),
        " seed="_el,
        _configuration.run.seed,
        " warmup-samples="_el,
        _configuration.run.warmupSamples,
        " samples="_el,
        _configuration.run.samples,
        " minimum-sample-time-ns="_el,
        _configuration.run.minimumSampleTime.count(),
        " memory-limit="_el,
        _configuration.run.memoryLimit,
        " progress-interval-ns="_el,
        _configuration.run.progressInterval.count(),
        " config-md5="_el,
        el::ByteFormat::compact(),
        configurationDigest());
}

void WorkloadRunner::printCoverage() {
    for (const auto &descriptor : coverageRegistry()) {
        el::io::printLine(
            "record=coverage use-case="_el,
            toString(descriptor.useCase),
            " input="_el,
            toString(descriptor.inputKind),
            " replacement="_el,
            toString(descriptor.replacementMode),
            " api="_el,
            descriptor.apiFamily);
    }
    el::io::printLine("record=summary action=coverage paths="_el, coverageRegistry().size());
}

auto WorkloadRunner::run() -> el::ExitCode {
    _executions.clear();
    _workspace = createWorkspace();
    el::io::printLine(
        "record=run profile=regex-api mode="_el,
        toString(_configuration.run.mode),
        " executable=regex-api-profile build-type="_el,
        el::String{cBuildType},
        " compiler="_el,
        el::String{cCompilerId},
        " compiler-version="_el,
        el::String{cCompilerVersion},
        " system="_el,
        el::String{cSystemName},
        " processor="_el,
        el::String{cSystemProcessor},
        " suite="_el,
        _configuration.run.suite,
        " workload-threads="_el,
        _configuration.run.threadCount,
        " seed="_el,
        _configuration.run.seed,
        " scenarios="_el,
        _configuration.scenarios.size(),
        " duration-ns="_el,
        _configuration.run.duration.count(),
        " workspace="_el,
        _workspace->path().toString(),
        " config-md5="_el,
        el::ByteFormat::compact(),
        configurationDigest());

    _executions.reserve(_configuration.scenarios.size());
    for (const auto &scenario : _configuration.scenarios) {
        _executions.emplace_back(_configuration, scenario, _workspace);
        _executions.back().validate();
    }
    for (auto &execution : _executions) {
        execution.calibrate();
    }

    const auto runStart = Clock::now();
    const auto deadline = runStart + _configuration.run.duration;
    auto nextProgress = runStart + _configuration.run.progressInterval;
    for (auto index = std::size_t{}; index < _executions.size(); ++index) {
        const auto &execution = _executions[index];
        for (auto warmup = std::uint32_t{}; warmup < _configuration.run.warmupSamples; ++warmup) {
            requireBeforeDeadline(deadline, "The regex profiler deadline was reached during warm-up."_el);
            execution.runSample();
        }
    }

    auto completed = std::size_t{};
    auto comparisons = std::vector<ComparisonResult>{};
    if (_configuration.run.mode == RunMode::Benchmark) {
        for (auto index = std::size_t{}; index < _executions.size(); ++index) {
            auto samples = std::vector<SampleResult>{};
            samples.reserve(_configuration.run.samples);
            for (auto sample = std::uint32_t{}; sample < _configuration.run.samples; ++sample) {
                requireBeforeDeadline(
                    deadline,
                    "The benchmark deadline was reached before every scenario collected its required samples."_el);
                samples.emplace_back(_executions[index].runSample());
            }
            const auto median = printBenchmark(_executions[index].prepared(), samples);
            recordComparison(_executions[index].prepared(), median, comparisons);
            if (_executions[index].prepared().scenario.useCase == UseCase::LazyContendedFirstUse) {
                printContention(_executions[index].prepared(), samples);
            }
            ++completed;
            printProgressIfDue(nextProgress, completed, _executions.size(), _configuration.run.progressInterval);
        }
    } else {
        while (Clock::now() < deadline) {
            for (auto index = std::size_t{}; index < _executions.size() && Clock::now() < deadline; ++index) {
                for (
                    auto repeat = std::uint32_t{};
                    repeat < _executions[index].prepared().scenario.weight && Clock::now() < deadline;
                    ++repeat) {
                    const auto result = _executions[index].runSample();
                    if (Clock::now() >= nextProgress) {
                        printSample(_executions[index].prepared(), result);
                        nextProgress = Clock::now() + _configuration.run.progressInterval;
                    }
                }
                if (completed < _executions.size()) {
                    ++completed;
                }
            }
        }
    }
    el::io::printLine(
        "record=summary covered-scenarios="_el,
        completed,
        " total-scenarios="_el,
        _executions.size(),
        " elapsed-ns="_el,
        elapsedNanoseconds(runStart),
        " workspace="_el,
        _workspace->path().toString(),
        " files-kept="_el,
        _configuration.run.keepFiles ? "yes"_el : "no"_el);
    return el::ExitCode::success();
}

void WorkloadRunner::workloadError(const el::String &message) {
    throw el::ApplicationError{message};
}

auto WorkloadRunner::elapsedNanoseconds(const Clock::time_point start) -> std::int64_t {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
}

void WorkloadRunner::requireBeforeDeadline(const Clock::time_point deadline, const el::String &message) {
    if (Clock::now() >= deadline) {
        workloadError(message);
    }
}

auto WorkloadRunner::createWorkspace() const -> el::TempDirectoryPtr {
    auto options = el::PathTempDirectoryOptions{};
    options.setPrefix("regex-api-profile-"_el).setRandomLength(el::CpCount{12U});
    auto root = el::Path::systemTempDirectoryOrThrow();
    if (!_configuration.run.workspace.isEmpty()) {
        root = el::Path{_configuration.run.workspace};
        if (!root.isValid()) {
            workloadError("The configured workspace is not a valid path."_el);
        }
        auto directoryOptions = el::PathCreateDirectoryOptions{};
        directoryOptions.setCreateParents(true).setCreationMode(el::PathCreateMode::CreateOrOverwrite);
        root.operations().createDirectoryOrThrow(directoryOptions);
    }
    auto workspace = root.operations().createTempDirectoryOrThrow(options);
    if (_configuration.run.keepFiles) {
        workspace->setRemoveOnDestroy(false);
    }
    return workspace;
}

auto WorkloadRunner::configurationDigest() const -> el::ByteBlock {
    auto text = el::StringEditor{};
    text.append(
        el::StringFormat{"{} {} {} {} {} {} {} {} {} {} {}\n"_el}.build(
            toString(_configuration.run.mode),
            _configuration.run.suite,
            _configuration.run.duration.count(),
            _configuration.run.threadCount,
            _configuration.run.seed,
            _configuration.run.warmupSamples,
            _configuration.run.samples,
            _configuration.run.minimumSampleTime.count(),
            _configuration.run.memoryLimit,
            _configuration.run.regexTimeout.count(),
            _configuration.run.keepFiles));
    for (const auto &scenario : _configuration.scenarios) {
        text.append(
            el::StringFormat{"{} {} {} {} {} {} {} {} {}\n"_el}.build(
                scenario.id,
                toString(scenario.useCase),
                toString(scenario.inputKind),
                toString(scenario.replacementMode),
                scenario.pattern,
                scenario.corpusName,
                scenario.subject.length(),
                scenario.sourceFile,
                scenario.repetitionCount));
        text.append(el::StringFormat{" {} {}\n"_el}.build(toString(scenario.backend), scenario.comparisonName));
    }
    auto hasher = el::cryptology::Hasher{el::cryptology::HashAlgorithm::Md5};
    hasher.update(text);
    return hasher.finalize();
}

void WorkloadRunner::printProgressIfDue(
    Clock::time_point &nextProgress,
    const std::size_t completed,
    const std::size_t total,
    const std::chrono::nanoseconds interval) {
    if (Clock::now() >= nextProgress) {
        el::io::printLine("record=progress completed="_el, completed, " total="_el, total);
        nextProgress = Clock::now() + interval;
    }
}

}
