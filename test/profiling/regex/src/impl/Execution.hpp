// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Execution_fwd.hpp"
#include "PreparedScenario.hpp"

#include <erbsland/system/EnvironmentVariables.hpp>

#include <algorithm>
#include <atomic>
#include <barrier>
#include <exception>
#include <limits>
#include <regex>
#include <thread>

namespace app::regex::impl {

using namespace el::text::literals;

/// Prepares and executes regular-expression profiling scenarios.
/// @notest{Covered by regex profiler CTest entries.}
class Execution final {
    using Clock = std::chrono::steady_clock;

public:
    /// Prepare the input data and expressions for one scenario.
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
            result.expression = compile(preparedPattern(result), scenario);
        }
        return result;
    }

    /// Validate one prepared scenario before profiling it.
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

    /// Determine the operation count for one timing sample.
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

    /// Execute one concurrent timing sample.
    static auto runSample(
        const Configuration &configuration,
        const PreparedScenario &prepared,
        const std::uint64_t,
        const std::uint64_t operations) -> SampleResult {
        const auto threadCount = configuration.run.threadCount;
        const auto failureVariable =
            el::system::EnvironmentVariables{}.get("ERBSLAND_REGEX_PROFILE_TEST_FAIL_WORKER"_el);
        const auto injectWorkerFailure = failureVariable.has_value() && *failureVariable == "1"_el;
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
                    if (injectWorkerFailure && worker == threadCount - 1U) {
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
    /// Throw a workload-validation error with `message`.
    [[noreturn]] static void workloadError(const el::String &message) { throw el::ApplicationError{message}; }

    /// Get the nanoseconds elapsed since `start`.
    [[nodiscard]] static auto elapsedNanoseconds(const Clock::time_point start) -> std::int64_t {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
    }

    /// Mix `value` into a deterministic result seed.
    [[nodiscard]] static auto mixSeed(std::uint64_t seed, const std::uint64_t value) noexcept -> std::uint64_t {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
        seed ^= seed >> 30U;
        seed *= 0xbf58476d1ce4e5b9ULL;
        seed ^= seed >> 27U;
        seed *= 0x94d049bb133111ebULL;
        return seed ^ (seed >> 31U);
    }

    /// Test whether `useCase` performs expression compilation.
    [[nodiscard]] static auto isCompilationCase(const UseCase useCase) noexcept -> bool {
        return useCase == UseCase::Compile || useCase == UseCase::LazyCompile || useCase == UseCase::LazyFirstUse ||
            useCase == UseCase::LazyContendedFirstUse;
    }

    /// Convert the scenario pattern to its requested text width.
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

    /// Access the prepared expression pattern.
    [[nodiscard]] static auto preparedPattern(const PreparedScenario &prepared) -> const el::text::AnyString & {
        return prepared.pattern;
    }

    /// Create the regex settings for `scenario`.
    [[nodiscard]] static auto settings(const Scenario &scenario) -> el::re::Settings {
        auto result = el::re::Settings{};
        result.setTimeout(scenario.timeout);
        if (scenario.pattern.isEmpty()) {
            result.enableFeature(el::re::Feature::EmptyGroups);
        }
        return result;
    }

    /// Compile an Erbsland regex expression.
    [[nodiscard]] static auto compile(const el::text::AnyString &pattern, const Scenario &scenario)
        -> el::re::RegExPtr {
        return el::re::RegEx::compile(pattern, scenario.flags, settings(scenario));
    }

    /// Create a lazily compiled Erbsland regex expression.
    [[nodiscard]] static auto lazyCompile(const el::text::AnyString &pattern, const Scenario &scenario)
        -> el::re::RegExPtr {
        return el::re::RegEx::lazyCompile(pattern, scenario.flags, settings(scenario));
    }

    /// Convert scenario flags to standard-library regex flags.
    [[nodiscard]] static auto standardFlags(const Scenario &scenario) -> std::regex_constants::syntax_option_type {
        auto result = std::regex_constants::ECMAScript;
        if (scenario.flags.isSet(el::re::Flag::IgnoreCase)) {
            result |= std::regex_constants::icase;
        }
        return result;
    }

    /// Compile a standard-library regex expression.
    [[nodiscard]] static auto compileStandard(const std::string &pattern, const Scenario &scenario)
        -> std::shared_ptr<const std::regex> {
        return std::make_shared<const std::regex>(pattern, standardFlags(scenario));
    }

    /// Incorporate one Erbsland match into the sample result.
    template <typename MatchPtr>
    static void consumeMatch(
        const MatchPtr &match, const bool includePositions, std::uint64_t &sink, std::uint64_t &matches) {
        if (!match) {
            sink = mixSeed(sink, std::numeric_limits<std::uint64_t>::max());
            return;
        }
        ++matches;
        const auto groupCount = match->groupCount();
        sink = mixSeed(sink, groupCount);
        if (includePositions) {
            sink = mixSeed(sink, match->begin());
            sink = mixSeed(sink, match->end());
        }
        const auto content = match->content();
        sink = mixSeed(sink, content.characterLength().toRawValue());
        sink = mixSeed(sink, content.toHash());
        for (auto index = std::size_t{}; index < groupCount; ++index) {
            const auto groupIndex = static_cast<el::re::CaptureGroupIndex>(index);
            const auto &group = match->group(groupIndex);
            if (includePositions) {
                sink = mixSeed(sink, group.begin());
                sink = mixSeed(sink, group.end());
            }
            sink = mixSeed(sink, group.name().toHash());
            const auto groupContent = match->content(groupIndex);
            sink = mixSeed(sink, groupContent.characterLength().toRawValue());
            sink = mixSeed(sink, groupContent.toHash());
        }
    }

    /// Incorporate one standard-library match into the sample result.
    template <typename MatchType>
    static void consumeStandardMatch(
        const MatchType *match, const std::string &subject, std::uint64_t &sink, std::uint64_t &matches) {
        if (match == nullptr) {
            sink = mixSeed(sink, std::numeric_limits<std::uint64_t>::max());
            return;
        }
        ++matches;
        sink = mixSeed(sink, match->size());
        const auto matchBegin = static_cast<std::size_t>(std::distance(subject.cbegin(), (*match)[0].first));
        const auto matchEnd = static_cast<std::size_t>(std::distance(subject.cbegin(), (*match)[0].second));
        sink = mixSeed(sink, matchBegin);
        sink = mixSeed(sink, matchEnd);
        const auto content = el::String{std::string_view{(*match)[0].first, (*match)[0].second}};
        sink = mixSeed(sink, content.characterLength().toRawValue());
        sink = mixSeed(sink, content.toHash());
        for (auto index = std::size_t{}; index < match->size(); ++index) {
            const auto &group = (*match)[index];
            const auto groupBegin = static_cast<std::size_t>(std::distance(subject.cbegin(), group.first));
            const auto groupEnd = static_cast<std::size_t>(std::distance(subject.cbegin(), group.second));
            sink = mixSeed(sink, groupBegin);
            sink = mixSeed(sink, groupEnd);
            sink = mixSeed(sink, el::String{}.toHash());
            const auto groupContent = el::String{std::string_view{group.first, group.second}};
            sink = mixSeed(sink, groupContent.characterLength().toRawValue());
            sink = mixSeed(sink, groupContent.toHash());
        }
    }

    /// Execute a standard-library regex workload.
    static void executeStandard(const PreparedScenario &prepared, std::uint64_t &sink, std::uint64_t &matches) {
        const auto &subject = prepared.standardSubject;
        const auto &expression = *prepared.standardExpression;
        auto match = std::smatch{};
        switch (prepared.scenario.useCase) {
        case UseCase::Match:
            if (std::regex_search(subject, match, expression, std::regex_constants::match_continuous)) {
                consumeStandardMatch(&match, subject, sink, matches);
            } else {
                consumeStandardMatch<std::smatch>(nullptr, subject, sink, matches);
            }
            break;
        case UseCase::FullMatch:
            if (std::regex_match(subject, match, expression)) {
                consumeStandardMatch(&match, subject, sink, matches);
            } else {
                consumeStandardMatch<std::smatch>(nullptr, subject, sink, matches);
            }
            break;
        case UseCase::FindFirst:
            if (std::regex_search(subject, match, expression)) {
                consumeStandardMatch(&match, subject, sink, matches);
            } else {
                consumeStandardMatch<std::smatch>(nullptr, subject, sink, matches);
            }
            break;
        case UseCase::FindAll:
        case UseCase::CollectAll:
            for (
                auto iterator = std::sregex_iterator{subject.begin(), subject.end(), expression};
                iterator != std::sregex_iterator{};
                ++iterator) {
                consumeStandardMatch(&*iterator, subject, sink, matches);
            }
            break;
        default:
            workloadError("Unsupported standard-library regex workload path."_el);
        }
    }

    /// Execute an Erbsland regex workload against an in-memory string.
    template <typename StringType>
    static void executeOnString(
        const PreparedScenario &prepared,
        const StringType &subject,
        const el::re::RegExPtr &expression,
        std::uint64_t &sink,
        std::uint64_t &matches) {
        const auto includePositions = !prepared.scenario.comparisonName.isEmpty();
        switch (prepared.scenario.useCase) {
        case UseCase::Match:
            consumeMatch(expression->match(subject), includePositions, sink, matches);
            break;
        case UseCase::FullMatch:
            consumeMatch(expression->fullMatch(subject), includePositions, sink, matches);
            break;
        case UseCase::FindFirst:
            consumeMatch(expression->findFirst(subject), includePositions, sink, matches);
            break;
        case UseCase::FindAll:
            for (const auto &match : expression->findAll(subject)) {
                consumeMatch(match, includePositions, sink, matches);
            }
            break;
        case UseCase::CollectAll:
            for (const auto &match : expression->collectAll(subject)) {
                consumeMatch(match, includePositions, sink, matches);
            }
            break;
        default:
            workloadError("Unsupported string regex workload path."_el);
        }
    }

    /// Execute an Erbsland regex workload against a text stream.
    static void executeOnStream(
        const PreparedScenario &prepared,
        const el::stream::TextInputStreamPtr &stream,
        const el::re::RegExPtr &expression,
        std::uint64_t &sink,
        std::uint64_t &matches) {
        const auto includePositions = !prepared.scenario.comparisonName.isEmpty();
        switch (prepared.scenario.useCase) {
        case UseCase::Match:
            consumeMatch(expression->match(stream), includePositions, sink, matches);
            break;
        case UseCase::FullMatch:
            consumeMatch(expression->fullMatch(stream), includePositions, sink, matches);
            break;
        case UseCase::FindFirst:
            consumeMatch(expression->findFirst(stream), includePositions, sink, matches);
            break;
        case UseCase::FindAll:
            for (const auto &match : expression->findAll(stream)) {
                consumeMatch(match, includePositions, sink, matches);
            }
            break;
        case UseCase::CollectAll:
            for (const auto &match : expression->collectAll(stream)) {
                consumeMatch(match, includePositions, sink, matches);
            }
            break;
        default:
            workloadError("Unsupported stream regex workload path."_el);
        }
    }

    /// Execute the configured replacement workload.
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

    /// Execute the first use of a lazily compiled expression.
    static void executeLazyFirstUse(
        const PreparedScenario &prepared,
        const el::re::RegExPtr &expression,
        std::uint64_t &sink,
        std::uint64_t &matches) {
        const auto includePositions = !prepared.scenario.comparisonName.isEmpty();
        if (prepared.scenario.inputKind == InputKind::StringUtf16) {
            consumeMatch(expression->fullMatch(prepared.subject16), includePositions, sink, matches);
        } else if (prepared.scenario.inputKind == InputKind::StringUtf32) {
            consumeMatch(expression->fullMatch(prepared.subject32), includePositions, sink, matches);
        } else {
            consumeMatch(expression->fullMatch(prepared.subject8), includePositions, sink, matches);
        }
    }

    /// Execute all operations assigned to one profiling worker.
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
            if (prepared.scenario.backend == Backend::Standard) {
                if (prepared.scenario.useCase == UseCase::Compile) {
                    const auto expression = compileStandard(prepared.standardPattern, prepared.scenario);
                    sink = mixSeed(sink, prepared.scenario.pattern.characterLength().toRawValue());
                } else {
                    executeStandard(prepared, sink, matches);
                }
                continue;
            }
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
                        if (!stream->setPosition(el::ByteIndex::zero()).isSuccess()) {
                            workloadError("Failed to rewind the profiling input stream."_el);
                        }
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
            if (!stream->close().isClosed()) {
                workloadError("Failed to close the profiling input stream."_el);
            }
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
};

}
