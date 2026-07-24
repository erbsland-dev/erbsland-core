// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/profiling/Definitions.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace app::byte {

/// The profiler execution mode.
enum class RunMode : std::uint8_t { Profile, Benchmark };
/// A profiled owning byte type.
enum class ByteType : std::uint8_t { Array, Block, BlockEditor, Buffer, RingBuffer };
/// A use-case rather than an individual overload.
enum class UseCase : std::uint8_t {
    Create,
    Copy,
    Move,
    Slice,
    Convert,
    ClearReset,
    ReserveShrink,
    Detach,
    SecureErase,
    ReadIndexed,
    WriteIndexed,
    Traverse,
    IntegerRead,
    IntegerWrite,
    Compare,
    PrefixSuffix,
    Find,
    Fill,
    Overwrite,
    Xor,
    Resize,
    Append,
    Insert,
    Replace,
    RemoveKeep,
    Join,
    Bitwise,
    ShiftRotate,
    PerByteShiftRotate,
    RingReserveGrow,
    RingTransfer,
    RingOwnedRead,
    RingWrappedCycle,
    RingInteger,
    RingClearShrinkSwap,
    CowStress,
    EditStress,
};
/// The configured primary-size expansion mode.
enum class SizeMode : std::uint8_t { Fixed, Incrementing, Exponential, Random, Boundaries };
/// The effective sensitive-storage state.
enum class SensitiveMode : std::uint8_t { NotApplicable, Normal, Sensitive };
/// A selection applied before sensitivity expansion.
enum class SensitiveSelection : std::uint8_t { Normal, Sensitive, All };
/// The logical unit represented by throughput.
enum class WorkUnit : std::uint8_t { Operations, Bytes };

/// One supported implementation-path variant and its public API coverage.
/// @notest{Verified by byte profiler dry-run and smoke CTest entries.}
struct UseCaseDescriptor {
    ByteType type{};                       ///< The owning byte type.
    UseCase useCase{};                     ///< The use case.
    el::String variant;                    ///< The implementation-path variant.
    std::vector<el::String> apiFamilies{}; ///< Public API families represented by this path.
    WorkUnit workUnit{WorkUnit::Bytes};    ///< The normalization unit.
};

/// Settings shared by a complete profiler run.
/// @notest{Verified by byte profiler dry-run and smoke CTest entries.}
struct RunSettings {
    RunMode mode{RunMode::Benchmark};                                          ///< The execution mode.
    el::String suite{"snapshot"};                                              ///< The built-in suite.
    std::chrono::nanoseconds duration{std::chrono::minutes{5}};                ///< Hard run deadline.
    std::uint32_t threadCount{4U};                                             ///< Workload thread count.
    std::uint64_t seed{0x455242534c414e44ULL};                                 ///< Global deterministic seed.
    std::uint32_t warmupSamples{1U};                                           ///< Warm-up samples per scenario.
    std::uint32_t samples{9U};                                                 ///< Measured benchmark samples.
    std::chrono::nanoseconds minimumSampleTime{std::chrono::milliseconds{20}}; ///< Calibration target.
    std::uint64_t memoryLimit{256ULL * 1024ULL * 1024ULL};                     ///< Aggregate fixture-memory limit.
    std::chrono::nanoseconds progressInterval{std::chrono::seconds{10}};       ///< Progress interval.
    SensitiveSelection sensitiveSelection{SensitiveSelection::All};            ///< Default sensitivity expansion.
};

/// One fully expanded profiler scenario.
/// @notest{Verified by byte profiler dry-run and smoke CTest entries.}
struct Scenario {
    el::String id;                                      ///< Stable expanded scenario ID.
    el::String group;                                   ///< User-facing scenario group.
    ByteType type{ByteType::Buffer};                    ///< The owning byte type.
    UseCase useCase{UseCase::Create};                   ///< The use case.
    el::String variant{"default"};                      ///< The implementation-path variant.
    SizeMode sizeMode{SizeMode::Fixed};                 ///< The source size-expansion mode.
    SensitiveMode sensitiveMode{SensitiveMode::Normal}; ///< Effective sensitive-storage state.
    std::uint64_t size{4096U};                          ///< Primary byte size.
    std::uint64_t operandSize{64U};                     ///< Secondary operand or chunk size.
    std::uint32_t weight{1U};                           ///< Profile-mode repetition weight.
    WorkUnit workUnit{WorkUnit::Bytes};                 ///< Measurement normalization unit.
};

/// A validated and expanded configuration.
/// @notest{Verified by byte profiler dry-run and smoke CTest entries.}
struct Configuration {
    RunSettings run;                   ///< Run settings.
    std::vector<Scenario> scenarios{}; ///< Expanded scenarios.
};

/// One worker's measured result.
/// @notest{Verified by byte profiler smoke CTest entries.}
struct WorkerResult {
    std::uint64_t operations{};      ///< Completed logical operations.
    std::uint64_t logicalBytes{};    ///< Processed logical bytes.
    std::uint64_t capacityChanges{}; ///< Observed capacity changes.
    std::int64_t nanoseconds{};      ///< Timed worker duration.
    std::uint64_t sink{};            ///< Optimizer-resistant result sink.
    el::ByteBlock digest;            ///< Final validation digest.
};

/// One synchronized concurrent sample.
/// @notest{Verified by byte profiler smoke CTest entries.}
struct SampleResult {
    std::vector<WorkerResult> workers; ///< Per-worker results.
    std::uint64_t operations{};        ///< Aggregate operations.
    std::uint64_t logicalBytes{};      ///< Aggregate logical bytes.
    std::uint64_t capacityChanges{};   ///< Aggregate capacity changes.
    std::int64_t wallNanoseconds{};    ///< Synchronized wall duration.
};

/// Access the central use-case and API coverage registry.
/// @return All supported type/use-case/variant descriptors.
/// @notest{Verified by byte profiler coverage CTest entry.}
[[nodiscard]] auto coverageRegistry() -> const std::vector<UseCaseDescriptor> &;
/// Find a descriptor for an expanded path.
/// @return The descriptor, or `nullptr` if the combination is unsupported.
/// @notest{Verified by byte profiler configuration CTest entries.}
[[nodiscard]] auto findDescriptor(ByteType type, UseCase useCase, const el::String &variant)
    -> const UseCaseDescriptor *;

/// Convert a run mode to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(RunMode value) -> el::String;
/// Convert a byte type to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(ByteType value) -> el::String;
/// Convert a use case to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(UseCase value) -> el::String;
/// Convert a size mode to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(SizeMode value) -> el::String;
/// Convert a sensitive mode to its output spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(SensitiveMode value) -> el::String;
/// Convert a sensitivity selection to its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(SensitiveSelection value) -> el::String;
/// Test whether a type supports persistent sensitive storage.
/// @notest{Trivial enum classification.}
[[nodiscard]] auto supportsSensitiveMode(ByteType value) noexcept -> bool;

/// Parse a byte type name.
/// @notest{Verified by byte profiler configuration CTest entries.}
[[nodiscard]] auto parseByteType(const el::String &value) -> std::optional<ByteType>;
/// Parse a use-case name.
/// @notest{Verified by byte profiler configuration CTest entries.}
[[nodiscard]] auto parseUseCase(const el::String &value) -> std::optional<UseCase>;
/// Parse a size-mode name.
/// @notest{Verified by byte profiler configuration CTest entries.}
[[nodiscard]] auto parseSizeMode(const el::String &value) -> std::optional<SizeMode>;
/// Parse a sensitivity-selection name.
/// @notest{Verified by byte profiler configuration CTest entries.}
[[nodiscard]] auto parseSensitiveSelection(const el::String &value) -> std::optional<SensitiveSelection>;

}
