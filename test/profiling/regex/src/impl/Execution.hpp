// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Execution_fwd.hpp"
#include "PreparedScenario.hpp"

#include "../ProfileTypes.hpp"

#include <erbsland/AnyString.hpp>
#include <erbsland/re/CaptureGroupIndex.hpp>
#include <erbsland/re/Settings.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/String.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/TextInputStream.hpp>

#include <chrono>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <regex>
#include <string>
#include <string_view>

namespace app::regex::impl {

using namespace el::text::literals;

/// Prepares and executes regular-expression profiling scenarios.
class Execution final {
    using Clock = std::chrono::steady_clock;

public:
    /// Prepare an execution with its immutable fixtures.
    /// @param configuration The run configuration, which must outlive this execution.
    /// @param scenario The source scenario.
    /// @param workspace The workspace for file-backed fixtures.
    Execution(const Configuration &configuration, const Scenario &scenario, const el::TempDirectoryPtr &workspace) :
        _configuration{configuration}, _prepared{prepare(configuration, scenario, workspace)} {}

public: // accessors
    /// Get the prepared inputs and validation results.
    [[nodiscard]] auto prepared() const noexcept -> const PreparedScenario & { return _prepared; }

public: // execution
    /// Validate one prepared scenario before profiling it.
    void validate();
    /// Determine the operation count for one timing sample.
    void calibrate();

public: // sampling
    /// Execute one concurrent timing sample.
    /// @return Timing and logical results from all workers.
    auto runSample() const -> SampleResult;

private:
    /// Prepare the input data and expressions for one scenario.
    [[nodiscard]] static auto prepare(
        const Configuration &configuration, const Scenario &scenario, const el::TempDirectoryPtr &workspace)
        -> PreparedScenario;
    /// Measure the number of operations required for one sample.
    [[nodiscard]] auto calibratedOperationCount() const -> std::uint64_t;
    /// Throw a workload-validation error with `message`.
    [[noreturn]] static void workloadError(const el::String &message);
    /// Get the nanoseconds elapsed since `start`.
    [[nodiscard]] static auto elapsedNanoseconds(const Clock::time_point start) -> std::int64_t;
    /// Mix `value` into a deterministic result seed.
    [[nodiscard]] static auto mixSeed(std::uint64_t seed, const std::uint64_t value) noexcept -> std::uint64_t;
    /// Test whether `useCase` performs expression compilation.
    [[nodiscard]] static auto isCompilationCase(const UseCase useCase) noexcept -> bool;
    /// Convert the scenario pattern to its requested text width.
    [[nodiscard]] static auto makePattern(const Scenario &scenario) -> el::AnyString;
    /// Access the prepared expression pattern.
    [[nodiscard]] auto preparedPattern() const -> const el::AnyString &;
    /// Create the regex settings for `scenario`.
    [[nodiscard]] static auto settings(const Scenario &scenario) -> el::re::Settings;
    /// Compile an Erbsland regex expression.
    [[nodiscard]] static auto compile(const el::AnyString &pattern, const Scenario &scenario) -> el::re::RegExPtr;
    /// Create a lazily compiled Erbsland regex expression.
    [[nodiscard]] static auto lazyCompile(const el::AnyString &pattern, const Scenario &scenario) -> el::re::RegExPtr;
    /// Convert scenario flags to standard-library regex flags.
    [[nodiscard]] static auto standardFlags(const Scenario &scenario) -> std::regex_constants::syntax_option_type;
    /// Compile a standard-library regex expression.
    [[nodiscard]] static auto compileStandard(const std::string &pattern, const Scenario &scenario)
        -> std::shared_ptr<const std::regex>;
    /// Incorporate one Erbsland match into the sample result.
    template <typename MatchPtr>
    static void consumeMatch(
        const MatchPtr &match, const bool includePositions, std::uint64_t &sink, std::uint64_t &matches);
    /// Incorporate one standard-library match into the sample result.
    template <typename MatchType>
    static void consumeStandardMatch(
        const MatchType *match, const std::string &subject, std::uint64_t &sink, std::uint64_t &matches);
    /// Execute a standard-library regex workload.
    void executeStandard(std::uint64_t &sink, std::uint64_t &matches) const;
    /// Execute an Erbsland regex workload against an in-memory string.
    template <typename StringType>
    void executeOnString(
        const StringType &subject,
        const el::re::RegExPtr &expression,
        std::uint64_t &sink,
        std::uint64_t &matches) const;
    /// Execute an Erbsland regex workload against a text stream.
    void executeOnStream(
        const el::stream::TextInputStreamPtr &stream,
        const el::re::RegExPtr &expression,
        std::uint64_t &sink,
        std::uint64_t &matches) const;
    /// Execute the configured replacement workload.
    void executeReplacement(const el::re::RegExPtr &expression, std::uint64_t &sink) const;
    /// Execute the first use of a lazily compiled expression.
    void executeLazyFirstUse(const el::re::RegExPtr &expression, std::uint64_t &sink, std::uint64_t &matches) const;
    /// Execute all operations assigned to one profiling worker.
    [[nodiscard]] auto executeWorker(const std::uint64_t operations, const el::re::RegExPtr &sharedLazy) const
        -> WorkerResult;

private:
    const Configuration &_configuration; ///< Immutable run settings owned by the workload runner.
    PreparedScenario _prepared;          ///< Prepared inputs and validated result signature.
    std::uint64_t _operations{1U};       ///< Calibrated operations per timing sample.
};

}

#include "Execution_templates.tpp"
