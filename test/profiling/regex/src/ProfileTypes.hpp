// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/profiling/Definitions.hpp>
#include <erbsland/re/all.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace app::regex {

/// The profiler execution mode.
enum class RunMode : std::uint8_t { Profile, Benchmark };
/// A regular-expression API use case.
enum class UseCase : std::uint8_t {
    Compile,
    LazyCompile,
    LazyFirstUse,
    LazyContendedFirstUse,
    Match,
    FullMatch,
    FindFirst,
    FindAll,
    CollectAll,
    ReplaceAll,
};
/// A concrete public input representation.
enum class InputKind : std::uint8_t { StringUtf8, StringUtf16, StringUtf32, FileUtf8, FileUtf16, FileUtf32 };
/// The replacement overload selected by a replace-all scenario.
enum class ReplacementMode : std::uint8_t { NotApplicable, Expression, Callback };
/// The source for a scenario corpus.
enum class CorpusSource : std::uint8_t { Inline, File };

/// One public API coverage path.
/// @notest{Verified by regex profiler coverage CTest entries.}
struct CoverageDescriptor {
    UseCase useCase{};                 ///< The measured use case.
    InputKind inputKind{};             ///< The input or pattern representation.
    ReplacementMode replacementMode{}; ///< The selected replacement overload.
    el::String apiFamily;              ///< The represented public API family.
};

/// Settings shared by a profiler run.
/// @notest{Verified by regex profiler configuration CTest entries.}
struct RunSettings {
    RunMode mode{RunMode::Benchmark};                                          ///< The execution mode.
    el::String suite{"snapshot"};                                              ///< The built-in suite.
    std::chrono::nanoseconds duration{std::chrono::minutes{5}};                ///< Hard run deadline.
    std::uint32_t threadCount{4U};                                             ///< Workload thread count.
    std::uint64_t seed{0x455242534c414e44ULL};                                 ///< Global deterministic seed.
    std::uint32_t warmupSamples{1U};                                           ///< Warm-up samples per scenario.
    std::uint32_t samples{9U};                                                 ///< Measured samples per scenario.
    std::chrono::nanoseconds minimumSampleTime{std::chrono::milliseconds{20}}; ///< Calibration target.
    std::uint64_t memoryLimit{256ULL * 1024ULL * 1024ULL};                     ///< Aggregate fixture-memory limit.
    std::chrono::nanoseconds progressInterval{std::chrono::seconds{10}};       ///< Progress interval.
    std::chrono::milliseconds regexTimeout{std::chrono::seconds{30}};          ///< Per-engine timeout.
    el::String workspace;                                                      ///< Optional workspace root.
    bool keepFiles{false};                                                     ///< Preserve generated files.
};

/// One fully expanded profiling scenario.
/// @notest{Verified by regex profiler dry-run and configuration CTest entries.}
struct Scenario {
    el::String id;                                                   ///< Stable expanded identifier.
    el::String group;                                                ///< User-facing group.
    UseCase useCase{UseCase::Match};                                 ///< Measured API use case.
    InputKind inputKind{InputKind::StringUtf8};                      ///< Input or pattern representation.
    ReplacementMode replacementMode{ReplacementMode::NotApplicable}; ///< Replacement overload.
    el::String patternName;                                          ///< Stable pattern identifier.
    el::String pattern;                                              ///< UTF-8 source pattern.
    el::re::Flags flags;                                             ///< Initial pattern flags.
    el::String corpusName;                                           ///< Stable corpus identifier.
    CorpusSource corpusSource{CorpusSource::Inline};                 ///< Corpus source kind.
    el::String subject;                                              ///< Inline UTF-8 subject.
    el::String sourceFile;                                           ///< External UTF-8 source path.
    std::uint32_t repetitionCount{1U};                               ///< Corpus repetition count.
    el::String replacement{"{0}"};                                   ///< Replacement expression.
    std::chrono::milliseconds timeout{std::chrono::seconds{30}};     ///< Scenario engine timeout.
    std::uint32_t weight{1U};                                        ///< Profile-mode weight.
};

/// A validated and expanded profiler configuration.
/// @notest{Verified by regex profiler dry-run and configuration CTest entries.}
struct Configuration {
    RunSettings run;                 ///< Run settings.
    std::vector<Scenario> scenarios; ///< Expanded scenarios.
};

/// One worker result.
/// @notest{Verified by regex profiler smoke CTest entries.}
struct WorkerResult {
    std::uint64_t operations{};        ///< Completed logical operations.
    std::uint64_t logicalBytes{};      ///< Processed logical UTF-8 bytes.
    std::uint64_t logicalCodePoints{}; ///< Processed logical code points.
    std::uint64_t matches{};           ///< Observed matches.
    std::int64_t nanoseconds{};        ///< Timed worker duration.
    std::uint64_t sink{};              ///< Optimizer-resistant validation sink.
};

/// One synchronized concurrent sample.
/// @notest{Verified by regex profiler smoke CTest entries.}
struct SampleResult {
    std::vector<WorkerResult> workers; ///< Worker results.
    std::uint64_t operations{};        ///< Aggregate operations.
    std::uint64_t logicalBytes{};      ///< Aggregate logical UTF-8 bytes.
    std::uint64_t logicalCodePoints{}; ///< Aggregate logical code points.
    std::uint64_t matches{};           ///< Aggregate matches.
    std::int64_t wallNanoseconds{};    ///< Synchronized wall duration.
};

/// Access the central public-API coverage registry.
/// @return All supported use-case/input/replacement paths.
/// @notest{Verified by regex profiler coverage CTest entries.}
[[nodiscard]] auto coverageRegistry() -> const std::vector<CoverageDescriptor> &;
/// Test whether a use-case and input combination is supported.
/// @notest{Verified by regex profiler configuration CTest entries.}
[[nodiscard]] auto isCompatible(UseCase useCase, InputKind inputKind, ReplacementMode replacementMode) noexcept -> bool;
/// Test whether an input kind represents a file stream.
/// @notest{Trivial enum classification.}
[[nodiscard]] auto isFileInput(InputKind value) noexcept -> bool;
/// Resolve a file input's text encoding.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto encodingFor(InputKind value) -> el::StringEncoding;

/// Convert a run mode to its stable spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(RunMode value) -> el::String;
/// Convert a use case to its stable spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(UseCase value) -> el::String;
/// Convert an input kind to its stable spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(InputKind value) -> el::String;
/// Convert a replacement mode to its stable spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(ReplacementMode value) -> el::String;
/// Parse a use-case name.
/// @notest{Verified by regex profiler configuration CTest entries.}
[[nodiscard]] auto parseUseCase(const el::String &value) -> std::optional<UseCase>;
/// Parse an input-kind name.
/// @notest{Verified by regex profiler configuration CTest entries.}
[[nodiscard]] auto parseInputKind(const el::String &value) -> std::optional<InputKind>;
/// Parse a replacement-mode name.
/// @notest{Verified by regex profiler configuration CTest entries.}
[[nodiscard]] auto parseReplacementMode(const el::String &value) -> std::optional<ReplacementMode>;
/// Parse a regular-expression flag name.
/// @notest{Verified by regex profiler configuration CTest entries.}
[[nodiscard]] auto parseFlag(const el::String &value) -> std::optional<el::re::Flag>;

}
