// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/profiling/Definitions.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace app::string {

/// The profiler execution mode.
enum class RunMode : std::uint8_t { Profile, Benchmark };
/// A profiled string width.
enum class StringWidth : std::uint8_t { U8, U16, U32 };
/// A profiled string value category.
enum class StringType : std::uint8_t { String, StringEditor };
/// A performance-relevant public API family.
enum class UseCase : std::uint8_t {
    Create,
    Copy,
    Move,
    TypeConvert,
    WidthConvert,
    SliceSplit,
    Storage,
    Inspect,
    ReadIndexed,
    Traverse,
    Compare,
    Hash,
    Search,
    Append,
    Insert,
    Replace,
    RemoveKeep,
    Trim,
    Truncate,
    Transform,
    EscapeSafe,
    Join,
    CowStress,
    EditStress,
    SensitiveStorage,
};
/// A deterministic source-text profile.
enum class ContentProfile : std::uint8_t { Ascii, Mixed, Supplementary, MalformedSparse, MalformedDense };
/// The configured primary-size expansion mode.
enum class SizeMode : std::uint8_t { Fixed, Incrementing, Exponential, Random, Boundaries };
/// The effective sensitive-storage state.
enum class SensitiveMode : std::uint8_t { NotApplicable, Normal, Sensitive };
/// A selection applied before sensitivity expansion.
enum class SensitiveSelection : std::uint8_t { Normal, Sensitive, All };
/// The logical unit represented by throughput.
enum class WorkUnit : std::uint8_t { Operations, CodePoints, NativeBytes };

/// One supported implementation-path variant and its public API coverage.
/// @notest{Verified by string profiler coverage and smoke CTest entries.}
struct UseCaseDescriptor {
    StringType type{};                       ///< The string value category.
    UseCase useCase{};                       ///< The use case.
    el::String variant;                      ///< The implementation-path variant.
    std::vector<el::String> apiFamilies{};   ///< Public API families represented by this path.
    WorkUnit workUnit{WorkUnit::CodePoints}; ///< The normalization unit.
    bool u8Only{};                           ///< Whether this path is available only for UTF-8.
};

/// Settings shared by a complete profiler run.
/// @notest{Verified by string profiler configuration and smoke CTest entries.}
struct RunSettings {
    RunMode mode{RunMode::Benchmark};                                          ///< The execution mode.
    el::String suite{"snapshot"};                                              ///< The built-in suite.
    std::chrono::nanoseconds duration{std::chrono::minutes{5}};                ///< Hard run deadline.
    std::uint32_t threadCount{4U};                                             ///< Workload thread count.
    std::uint64_t seed{0x535452494e475052ULL};                                 ///< Global deterministic seed.
    std::uint32_t warmupSamples{1U};                                           ///< Warm-up samples per scenario.
    std::uint32_t samples{9U};                                                 ///< Measured benchmark samples.
    std::chrono::nanoseconds minimumSampleTime{std::chrono::milliseconds{20}}; ///< Calibration target.
    std::uint64_t memoryLimit{256ULL * 1024ULL * 1024ULL};                     ///< Aggregate fixture-memory limit.
    std::chrono::nanoseconds progressInterval{std::chrono::seconds{10}};       ///< Progress interval.
    SensitiveSelection sensitiveSelection{SensitiveSelection::Normal};         ///< U8 sensitivity expansion.
};

/// One fully expanded profiler scenario.
/// @notest{Verified by string profiler dry-run and smoke CTest entries.}
struct Scenario {
    el::String id;                                             ///< Stable expanded scenario ID.
    el::String group;                                          ///< User-facing scenario group.
    StringWidth width{StringWidth::U8};                        ///< The string width.
    StringType type{StringType::String};                       ///< The value category.
    UseCase useCase{UseCase::Create};                          ///< The use case.
    el::String variant{"copy-source"};                         ///< The implementation-path variant.
    ContentProfile contentProfile{ContentProfile::Mixed};      ///< The source-text profile.
    SizeMode sizeMode{SizeMode::Fixed};                        ///< The source size-expansion mode.
    SensitiveMode sensitiveMode{SensitiveMode::NotApplicable}; ///< Effective sensitive-storage state.
    std::uint64_t size{4096U};                                 ///< Primary decoded code-point count.
    std::uint64_t operandSize{64U};                            ///< Secondary decoded code-point count.
    std::uint32_t weight{1U};                                  ///< Profile-mode repetition weight.
    WorkUnit workUnit{WorkUnit::CodePoints};                   ///< Measurement normalization unit.
};

/// A validated and expanded configuration.
/// @notest{Verified by string profiler configuration CTest entries.}
struct Configuration {
    RunSettings run;                   ///< Run settings.
    std::vector<Scenario> scenarios{}; ///< Expanded scenarios.
};

/// One worker's measured result.
/// @notest{Verified by string profiler smoke CTest entries.}
struct WorkerResult {
    std::uint64_t operations{};         ///< Completed logical operations.
    std::uint64_t logicalCodePoints{};  ///< Processed logical code points.
    std::uint64_t logicalNativeBytes{}; ///< Processed logical native bytes.
    std::uint64_t capacityChanges{};    ///< Observed capacity changes.
    std::int64_t nanoseconds{};         ///< Timed worker duration.
    std::uint64_t sink{};               ///< Optimizer-resistant result sink.
    el::ByteBlock digest;               ///< Final validation digest.
};

/// One synchronized concurrent sample.
/// @notest{Verified by string profiler smoke CTest entries.}
struct SampleResult {
    std::vector<WorkerResult> workers;  ///< Per-worker results.
    std::uint64_t operations{};         ///< Aggregate operations.
    std::uint64_t logicalCodePoints{};  ///< Aggregate logical code points.
    std::uint64_t logicalNativeBytes{}; ///< Aggregate logical native bytes.
    std::uint64_t capacityChanges{};    ///< Aggregate capacity changes.
    std::int64_t wallNanoseconds{};     ///< Synchronized wall duration.
};

/// Access the central use-case and API coverage registry.
/// @return All supported value-type/use-case/variant descriptors.
/// @notest{Verified by string profiler coverage CTest entry.}
[[nodiscard]] auto coverageRegistry() -> const std::vector<UseCaseDescriptor> &;
/// Find a descriptor for an expanded path.
/// @return The descriptor, or `nullptr` if the combination is unsupported.
/// @notest{Verified by string profiler configuration CTest entries.}
[[nodiscard]] auto findDescriptor(StringType type, UseCase useCase, const el::String &variant)
    -> const UseCaseDescriptor *;

/// Convert a run mode to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(RunMode value) -> el::String;
/// Convert a string width to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(StringWidth value) -> el::String;
/// Convert a value category to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(StringType value) -> el::String;
/// Convert a use case to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(UseCase value) -> el::String;
/// Convert a content profile to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(ContentProfile value) -> el::String;
/// Convert a size mode to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(SizeMode value) -> el::String;
/// Convert a sensitive mode to its output spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(SensitiveMode value) -> el::String;
/// Convert a sensitivity selection to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(SensitiveSelection value) -> el::String;

/// Parse a string width name.
/// @notest{Verified by string profiler configuration CTest entries.}
[[nodiscard]] auto parseStringWidth(const el::String &value) -> std::optional<StringWidth>;
/// Parse a value-category name.
/// @notest{Verified by string profiler configuration CTest entries.}
[[nodiscard]] auto parseStringType(const el::String &value) -> std::optional<StringType>;
/// Parse a use-case name.
/// @notest{Verified by string profiler configuration CTest entries.}
[[nodiscard]] auto parseUseCase(const el::String &value) -> std::optional<UseCase>;
/// Parse a content-profile name.
/// @notest{Verified by string profiler configuration CTest entries.}
[[nodiscard]] auto parseContentProfile(const el::String &value) -> std::optional<ContentProfile>;
/// Parse a size-mode name.
/// @notest{Verified by string profiler configuration CTest entries.}
[[nodiscard]] auto parseSizeMode(const el::String &value) -> std::optional<SizeMode>;
/// Parse a sensitivity-selection name.
/// @notest{Verified by string profiler configuration CTest entries.}
[[nodiscard]] auto parseSensitiveSelection(const el::String &value) -> std::optional<SensitiveSelection>;
/// Calculate the deterministic native payload bytes for a content prefix.
/// @notest{Verified indirectly by string profiler dry-run and workload validation.}
[[nodiscard]] auto nativeBytesFor(StringWidth width, ContentProfile profile, std::uint64_t codePoints) noexcept
    -> std::uint64_t;

}
