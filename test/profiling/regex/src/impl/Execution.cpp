// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Execution.hpp"

#include <erbsland/EncodingMode.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathCreateMode.hpp>
#include <erbsland/PathReadTextOptions.hpp>
#include <erbsland/PathWriteTextOptions.hpp>
#include <erbsland/StringBomMode.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringEncoding.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/TempDirectory.hpp>

#include <algorithm>
#include <barrier>
#include <exception>
#include <thread>
#include <vector>

namespace app::regex::impl {

auto Execution::prepare(
    const Configuration &configuration, const Scenario &scenario, const el::TempDirectoryPtr &workspace)
    -> PreparedScenario {
    auto subject = scenario.subject;
    if (scenario.corpusSource == CorpusSource::File) {
        auto options = el::PathReadTextOptions{el::StringEncoding::Utf8};
        options.setBomMode(el::StringBomMode::Automatic).setEncodingMode(el::EncodingMode::Strict);
        subject = el::Path{scenario.sourceFile}.content().readTextOrThrow(options);
    }
    if (scenario.repetitionCount > 1U) {
        auto repeated = el::StringEditor{};
        repeated.reserve(subject.length() * scenario.repetitionCount);
        for (auto index = std::uint32_t{}; index < scenario.repetitionCount; ++index) {
            repeated.append(subject);
        }
        subject = repeated;
    }
    const auto estimatedMemory = subject.length().toRawValue() * 7ULL * configuration.run.threadCount;
    if (estimatedMemory > configuration.run.memoryLimit) {
        workloadError(
            el::StringFormat{"Scenario '{}' exceeds the configured fixture-memory limit."_el}.build(scenario.id));
    }
    auto result = PreparedScenario{
        .scenario = scenario,
        .pattern = makePattern(scenario),
        .subject8 = subject,
        .subject16 = el::StringConverter{subject}.toU16String(),
        .subject32 = el::StringConverter{subject}.toU32String(),
        .filePath = {},
        .expression = {},
        .standardPattern = el::StringConverter{scenario.pattern}.toStdString(),
        .standardSubject = el::StringConverter{subject}.toStdString(),
        .standardExpression = {},
        .validationSink = 0U,
        .validationMatches = 0U};
    if (isFileInput(scenario.inputKind)) {
        result.filePath = workspace->path() /
            el::StringFormat{"scenario-{}-{}.txt"_el}.build(scenario.id.toHash(), toString(scenario.inputKind));
        auto options = el::PathWriteTextOptions{encodingFor(scenario.inputKind)};
        options.setCreateParents(true)
            .setCreationMode(el::PathCreateMode::CreateOrOverwrite)
            .setBomMode(el::StringBomMode::Automatic);
        result.filePath.content().writeTextOrThrow(subject, options);
    }
    if (scenario.backend == Backend::Standard && !isCompilationCase(scenario.useCase)) {
        result.standardExpression = compileStandard(result.standardPattern, scenario);
    } else if (!isCompilationCase(scenario.useCase)) {
        result.expression = compile(result.pattern, scenario);
    }
    return result;
}

void Execution::validate() {
    auto sharedLazy = el::re::RegExPtr{};
    if (_prepared.scenario.useCase == UseCase::LazyContendedFirstUse) {
        sharedLazy = lazyCompile(preparedPattern(), _prepared.scenario);
    }
    const auto result = executeWorker(1U, sharedLazy);
    if (result.operations != 1U) {
        workloadError("Regex profiler validation did not complete one operation."_el);
    }
    _prepared.validationSink = result.sink;
    _prepared.validationMatches = result.matches;
}

void Execution::calibrate() {
    _operations = calibratedOperationCount();
}

auto Execution::calibratedOperationCount() const -> std::uint64_t {
    if (isFileInput(_prepared.scenario.inputKind) || _prepared.scenario.useCase == UseCase::LazyContendedFirstUse ||
        _prepared.scenario.useCase == UseCase::CollectAll) {
        return 1U;
    }
    const auto footprint = std::max<std::uint64_t>(
        1U, _prepared.subject8.length().toRawValue() + _prepared.scenario.pattern.length().toRawValue());
    const auto maximum = std::clamp<std::uint64_t>(
        _configuration.run.memoryLimit / _configuration.run.threadCount / footprint, 1U, 1'000'000U);
    auto operations = std::uint64_t{1U};
    for (;;) {
        const auto result = executeWorker(operations, {});
        if (result.nanoseconds >= _configuration.run.minimumSampleTime.count() || operations >= maximum) {
            return operations;
        }
        const auto elapsed = std::max<std::int64_t>(1, result.nanoseconds);
        const auto ratio = std::clamp<std::uint64_t>(
            static_cast<std::uint64_t>(_configuration.run.minimumSampleTime.count() / elapsed), 2U, 16U);
        operations = std::min(maximum, operations > maximum / ratio ? maximum : operations * ratio);
    }
}

auto Execution::runSample() const -> SampleResult {
    const auto operations = _operations;
    const auto threadCount = _configuration.run.threadCount;
    const auto failureVariable = el::system::EnvironmentVariables{}.get("ERBSLAND_REGEX_PROFILE_TEST_FAIL_WORKER"_el);
    const auto injectWorkerFailure = failureVariable.has_value() && *failureVariable == "1"_el;
    auto sharedLazy = el::re::RegExPtr{};
    if (_prepared.scenario.useCase == UseCase::LazyContendedFirstUse) {
        sharedLazy = lazyCompile(preparedPattern(), _prepared.scenario);
    }
    auto results = std::vector<WorkerResult>(threadCount);
    auto failures = std::vector<std::exception_ptr>(threadCount);
    auto wallStart = Clock::time_point{};
    auto startBarrier = std::barrier{
        static_cast<std::ptrdiff_t>(threadCount + 1U), [&wallStart]() noexcept { wallStart = Clock::now(); }};
    auto wallEnd = Clock::time_point{};
    auto finishBarrier =
        std::barrier{static_cast<std::ptrdiff_t>(threadCount + 1U), [&wallEnd]() noexcept { wallEnd = Clock::now(); }};
    auto threads = std::vector<std::jthread>{};
    threads.reserve(threadCount);
    for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
        threads.emplace_back([&, worker]() {
            try {
                startBarrier.arrive_and_wait();
                if (injectWorkerFailure && worker == threadCount - 1U) {
                    workloadError("Injected regex profiler worker failure."_el);
                }
                results[worker] = executeWorker(operations, sharedLazy);
            } catch (...) {
                failures[worker] = std::current_exception();
            }
            finishBarrier.arrive_and_wait();
        });
    }
    startBarrier.arrive_and_wait();
    finishBarrier.arrive_and_wait();
    const auto wallNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(wallEnd - wallStart).count();
    for (const auto &failure : failures) {
        if (failure) {
            std::rethrow_exception(failure);
        }
    }
    auto result = SampleResult{.workers = std::move(results), .wallNanoseconds = wallNanoseconds};
    for (const auto &worker : result.workers) {
        if (worker.operations != operations) {
            workloadError("A regex profiler worker produced an invalid operation count."_el);
        }
        result.operations += worker.operations;
        result.logicalBytes += worker.logicalBytes;
        result.logicalCodePoints += worker.logicalCodePoints;
        result.matches += worker.matches;
    }
    const auto &canonical = result.workers.front();
    for (const auto &worker : result.workers) {
        if (worker.sink != canonical.sink || worker.matches != canonical.matches) {
            workloadError("Regex profiler workers produced different logical results."_el);
        }
    }
    return result;
}

}
