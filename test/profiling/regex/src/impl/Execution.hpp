// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Execution_fwd.hpp"
#include "PreparedScenario.hpp"

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstdlib>
#include <exception>
#include <limits>
#include <thread>

namespace app::regex::impl {

using namespace el::text::literals;

/// Prepares and executes regular-expression profiling scenarios.
/// @notest{Covered by regex profiler CTest entries.}
class Execution final {
    using Clock = std::chrono::steady_clock;

public:
    [[nodiscard]] static auto prepare(
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
            subject = el::String{repeated};
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
        if (!isCompilationCase(scenario.useCase)) {
            result.expression = compile(preparedPattern(result), scenario);
        }
        return result;
    }

    static void validate(PreparedScenario &prepared) {
        auto sharedLazy = el::re::RegExPtr{};
        if (prepared.scenario.useCase == UseCase::LazyContendedFirstUse) {
            sharedLazy = lazyCompile(preparedPattern(prepared), prepared.scenario);
        }
        const auto result = executeWorker(prepared, 1U, sharedLazy);
        if (result.operations != 1U) {
            workloadError("Regex profiler validation did not complete one operation."_el);
        }
        prepared.validationSink = result.sink;
        prepared.validationMatches = result.matches;
    }

    [[nodiscard]] static auto calibrate(const Configuration &configuration, const PreparedScenario &prepared)
        -> std::uint64_t {
        if (isFileInput(prepared.scenario.inputKind) || prepared.scenario.useCase == UseCase::LazyContendedFirstUse ||
            prepared.scenario.useCase == UseCase::CollectAll) {
            return 1U;
        }
        const auto footprint = std::max<std::uint64_t>(
            1U, prepared.subject8.length().toRawValue() + prepared.scenario.pattern.length().toRawValue());
        const auto maximum = std::clamp<std::uint64_t>(
            configuration.run.memoryLimit / configuration.run.threadCount / footprint, 1U, 1'000'000U);
        auto operations = std::uint64_t{1U};
        for (;;) {
            const auto result = executeWorker(prepared, operations, {});
            if (result.nanoseconds >= configuration.run.minimumSampleTime.count() || operations >= maximum) {
                return operations;
            }
            const auto elapsed = std::max<std::int64_t>(1, result.nanoseconds);
            const auto ratio = std::clamp<std::uint64_t>(
                static_cast<std::uint64_t>(configuration.run.minimumSampleTime.count() / elapsed), 2U, 16U);
            operations = std::min(maximum, operations > maximum / ratio ? maximum : operations * ratio);
        }
    }

    [[nodiscard]] static auto runSample(
        const Configuration &configuration,
        const PreparedScenario &prepared,
        const std::uint64_t sampleIndex,
        const std::uint64_t operations) -> SampleResult {
        static_cast<void>(sampleIndex);
        const auto threadCount = configuration.run.threadCount;
        auto sharedLazy = el::re::RegExPtr{};
        if (prepared.scenario.useCase == UseCase::LazyContendedFirstUse) {
            sharedLazy = lazyCompile(preparedPattern(prepared), prepared.scenario);
        }
        auto results = std::vector<WorkerResult>(threadCount);
        auto failures = std::vector<std::exception_ptr>(threadCount);
        auto wallStart = Clock::time_point{};
        auto startBarrier = std::barrier{
            static_cast<std::ptrdiff_t>(threadCount + 1U), [&wallStart]() noexcept { wallStart = Clock::now(); }};
        auto wallEnd = Clock::time_point{};
        auto finishBarrier = std::barrier{
            static_cast<std::ptrdiff_t>(threadCount + 1U), [&wallEnd]() noexcept { wallEnd = Clock::now(); }};
        auto threads = std::vector<std::jthread>{};
        threads.reserve(threadCount);
        for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
            threads.emplace_back([&, worker]() {
                try {
                    startBarrier.arrive_and_wait();
                    if (injectWorkerFailure() && worker == threadCount - 1U) {
                        workloadError("Injected regex profiler worker failure."_el);
                    }
                    results[worker] = executeWorker(prepared, operations, sharedLazy);
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

private:
    [[noreturn]] static void workloadError(const el::String &message) { throw el::ApplicationError{message}; }

    [[nodiscard]] static auto elapsedNanoseconds(const Clock::time_point start) -> std::int64_t {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
    }

    [[nodiscard]] static auto mixSeed(std::uint64_t seed, const std::uint64_t value) noexcept -> std::uint64_t {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
        seed ^= seed >> 30U;
        seed *= 0xbf58476d1ce4e5b9ULL;
        seed ^= seed >> 27U;
        seed *= 0x94d049bb133111ebULL;
        return seed ^ (seed >> 31U);
    }

    [[nodiscard]] static auto isCompilationCase(const UseCase useCase) noexcept -> bool {
        return useCase == UseCase::Compile || useCase == UseCase::LazyCompile || useCase == UseCase::LazyFirstUse ||
            useCase == UseCase::LazyContendedFirstUse;
    }

    [[nodiscard]] static auto makePattern(const Scenario &scenario) -> el::text::AnyString {
        switch (scenario.inputKind) {
        case InputKind::StringUtf16:
            return el::text::AnyString{el::StringConverter{scenario.pattern}.toU16String()};
        case InputKind::StringUtf32:
            return el::text::AnyString{el::StringConverter{scenario.pattern}.toU32String()};
        default:
            return el::text::AnyString{scenario.pattern};
        }
    }

    [[nodiscard]] static auto preparedPattern(const PreparedScenario &prepared) -> const el::text::AnyString & {
        return prepared.pattern;
    }

    [[nodiscard]] static auto settings(const Scenario &scenario) -> el::re::Settings {
        auto result = el::re::Settings{};
        result.setTimeout(scenario.timeout);
        if (scenario.pattern.isEmpty()) {
            result.enableFeature(el::re::Feature::EmptyGroups);
        }
        return result;
    }

    [[nodiscard]] static auto compile(const el::text::AnyString &pattern, const Scenario &scenario)
        -> el::re::RegExPtr {
        return el::re::RegEx::compile(pattern, scenario.flags, settings(scenario));
    }

    [[nodiscard]] static auto lazyCompile(const el::text::AnyString &pattern, const Scenario &scenario)
        -> el::re::RegExPtr {
        return el::re::RegEx::lazyCompile(pattern, scenario.flags, settings(scenario));
    }

    template <typename MatchPtr>
    static void consumeMatch(const MatchPtr &match, std::uint64_t &sink, std::uint64_t &matches) {
        if (!match) {
            sink = mixSeed(sink, std::numeric_limits<std::uint64_t>::max());
            return;
        }
        ++matches;
        const auto groupCount = match->groupCount();
        sink = mixSeed(sink, groupCount);
        const auto content = match->content();
        sink = mixSeed(sink, content.characterLength().toRawValue());
        sink = mixSeed(sink, content.toHash());
        for (auto index = std::size_t{}; index < groupCount; ++index) {
            const auto groupIndex = static_cast<el::re::CaptureGroupIndex>(index);
            const auto &group = match->group(groupIndex);
            sink = mixSeed(sink, group.name().toHash());
            const auto groupContent = match->content(groupIndex);
            sink = mixSeed(sink, groupContent.characterLength().toRawValue());
            sink = mixSeed(sink, groupContent.toHash());
        }
    }

    template <typename StringType>
    static void executeOnString(
        const PreparedScenario &prepared,
        const StringType &subject,
        const el::re::RegExPtr &expression,
        std::uint64_t &sink,
        std::uint64_t &matches) {
        switch (prepared.scenario.useCase) {
        case UseCase::Match:
            consumeMatch(expression->match(subject), sink, matches);
            break;
        case UseCase::FullMatch:
            consumeMatch(expression->fullMatch(subject), sink, matches);
            break;
        case UseCase::FindFirst:
            consumeMatch(expression->findFirst(subject), sink, matches);
            break;
        case UseCase::FindAll:
            for (const auto &match : expression->findAll(subject)) {
                consumeMatch(match, sink, matches);
            }
            break;
        case UseCase::CollectAll:
            for (const auto &match : expression->collectAll(subject)) {
                consumeMatch(match, sink, matches);
            }
            break;
        default:
            workloadError("Unsupported string regex workload path."_el);
        }
    }

    static void executeOnStream(
        const PreparedScenario &prepared,
        const el::stream::TextInputStreamPtr &stream,
        const el::re::RegExPtr &expression,
        std::uint64_t &sink,
        std::uint64_t &matches) {
        switch (prepared.scenario.useCase) {
        case UseCase::Match:
            consumeMatch(expression->match(stream), sink, matches);
            break;
        case UseCase::FullMatch:
            consumeMatch(expression->fullMatch(stream), sink, matches);
            break;
        case UseCase::FindFirst:
            consumeMatch(expression->findFirst(stream), sink, matches);
            break;
        case UseCase::FindAll:
            for (const auto &match : expression->findAll(stream)) {
                consumeMatch(match, sink, matches);
            }
            break;
        case UseCase::CollectAll:
            for (const auto &match : expression->collectAll(stream)) {
                consumeMatch(match, sink, matches);
            }
            break;
        default:
            workloadError("Unsupported stream regex workload path."_el);
        }
    }

    static void executeReplacement(
        const PreparedScenario &prepared, const el::re::RegExPtr &expression, std::uint64_t &sink) {
        auto result = el::String{};
        if (prepared.scenario.replacementMode == ReplacementMode::Expression) {
            result = expression->replaceAll(prepared.subject8, prepared.scenario.replacement);
        } else {
            result = expression->replaceAll(
                prepared.subject8, [](const el::re::MatchPtr &match) -> el::String { return match->content(); });
        }
        sink = mixSeed(sink, result.length().toRawValue());
        sink = mixSeed(sink, result.toHash());
    }

    static void executeLazyFirstUse(
        const PreparedScenario &prepared,
        const el::re::RegExPtr &expression,
        std::uint64_t &sink,
        std::uint64_t &matches) {
        if (prepared.scenario.inputKind == InputKind::StringUtf16) {
            consumeMatch(expression->fullMatch(prepared.subject16), sink, matches);
        } else if (prepared.scenario.inputKind == InputKind::StringUtf32) {
            consumeMatch(expression->fullMatch(prepared.subject32), sink, matches);
        } else {
            consumeMatch(expression->fullMatch(prepared.subject8), sink, matches);
        }
    }

    [[nodiscard]] static auto executeWorker(
        const PreparedScenario &prepared, const std::uint64_t operations, const el::re::RegExPtr &sharedLazy)
        -> WorkerResult {
        auto sink = std::uint64_t{0x455242534c414e44ULL};
        auto matches = std::uint64_t{};
        auto stream = el::stream::TextInputStreamPtr{};
        if (isFileInput(prepared.scenario.inputKind)) {
            auto options = el::PathReadTextOptions{encodingFor(prepared.scenario.inputKind)};
            options.setBomMode(el::StringBomMode::Automatic)
                .setEncodingMode(el::EncodingMode::Strict)
                .setTimeout(el::TimeDelta::seconds(30));
            stream = prepared.filePath.content().openTextInputStream(options);
        }
        auto lazyExpressions = std::vector<el::re::RegExPtr>{};
        if (prepared.scenario.useCase == UseCase::LazyFirstUse) {
            lazyExpressions.reserve(static_cast<std::size_t>(operations));
            for (auto operation = std::uint64_t{}; operation < operations; ++operation) {
                lazyExpressions.emplace_back(lazyCompile(preparedPattern(prepared), prepared.scenario));
            }
        }
        const auto start = Clock::now();
        for (auto operation = std::uint64_t{}; operation < operations; ++operation) {
            switch (prepared.scenario.useCase) {
            case UseCase::Compile: {
                const auto expression = compile(preparedPattern(prepared), prepared.scenario);
                sink = mixSeed(sink, expression->pattern().characterLength().toRawValue());
                break;
            }
            case UseCase::LazyCompile: {
                const auto expression = lazyCompile(preparedPattern(prepared), prepared.scenario);
                sink = mixSeed(sink, expression->isCompiled() ? 1U : 0U);
                break;
            }
            case UseCase::LazyFirstUse:
                executeLazyFirstUse(prepared, lazyExpressions[operation], sink, matches);
                break;
            case UseCase::LazyContendedFirstUse:
                executeLazyFirstUse(prepared, sharedLazy, sink, matches);
                break;
            case UseCase::ReplaceAll:
                executeReplacement(prepared, prepared.expression, sink);
                break;
            default:
                if (stream) {
                    if (operation != 0U) {
                        static_cast<void>(stream->setPosition(el::ByteIndex::zero()));
                    }
                    executeOnStream(prepared, stream, prepared.expression, sink, matches);
                } else if (prepared.scenario.inputKind == InputKind::StringUtf16) {
                    executeOnString(prepared, prepared.subject16, prepared.expression, sink, matches);
                } else if (prepared.scenario.inputKind == InputKind::StringUtf32) {
                    executeOnString(prepared, prepared.subject32, prepared.expression, sink, matches);
                } else {
                    executeOnString(prepared, prepared.subject8, prepared.expression, sink, matches);
                }
                break;
            }
        }
        const auto elapsed = elapsedNanoseconds(start);
        if (stream) {
            static_cast<void>(stream->close());
        }
        return WorkerResult{
            .operations = operations,
            .logicalBytes =
                (isCompilationCase(prepared.scenario.useCase) ? prepared.scenario.pattern.length().toRawValue()
                                                              : prepared.subject8.length().toRawValue()) *
                operations,
            .logicalCodePoints =
                (isCompilationCase(prepared.scenario.useCase) ? prepared.scenario.pattern.characterLength().toRawValue()
                                                              : prepared.subject8.characterLength().toRawValue()) *
                operations,
            .matches = matches,
            .nanoseconds = elapsed,
            .sink = sink};
    }

    [[nodiscard]] static auto injectWorkerFailure() noexcept -> bool {
#if defined(_WIN32)
        auto *value = static_cast<char *>(nullptr);
        auto size = std::size_t{};
        const auto status = _dupenv_s(&value, &size, "ERBSLAND_REGEX_PROFILE_TEST_FAIL_WORKER");
        const auto result = status == 0 && value != nullptr;
        std::free(value);
        return result;
#else
        return std::getenv("ERBSLAND_REGEX_PROFILE_TEST_FAIL_WORKER") != nullptr;
#endif
    }
};

}
