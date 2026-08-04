// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/profiling/Definitions.hpp>

#include <chrono>
#include <cstdint>
#include <vector>

namespace app::stream {

using namespace el::text::literals;

/// The measurement mode.
enum class RunMode : std::uint8_t { Profile, Benchmark };
/// The stream transfer direction.
enum class Direction : std::uint8_t { Read, Write };
/// The physical file and text encoding type.
enum class FileType : std::uint8_t { Binary, Utf8, Utf16, Utf32 };
/// The file selection pattern between repeated samples.
enum class Locality : std::uint8_t { Hot, Rotating };
/// The transfer chunk scheduling strategy.
enum class ChunkMode : std::uint8_t { Fixed, Incrementing, Random };
/// A sequential stream API operation.
enum class Method : std::uint8_t {
    ReadByte,
    ReadSpan,
    ReadBlock,
    ReadExact,
    ReadAll,
    WriteByte,
    WriteSpan,
    WriteBlock,
    ReadChar,
    ReadText,
    ReadLine,
    ReadAllText,
    WriteChar,
    WriteText,
    WriteLine,
};

/// Settings that apply to a complete profiler run.
/// @notest{Covered through command-line dry runs and profiler smoke tests.}
struct RunSettings {
    RunMode mode{RunMode::Profile};                                      ///< The measurement mode.
    el::String suite{"cover-all"_el};                                    ///< The built-in suite name.
    std::chrono::nanoseconds duration{std::chrono::minutes{8}};          ///< Total target duration.
    std::uint32_t threadCount{4U};                                       ///< Workload thread count.
    std::uint64_t seed{0x455242534c414e44ULL};                           ///< Global deterministic seed.
    std::uint64_t corpusLimit{256ULL * 1024ULL * 1024ULL};               ///< Maximum active corpus size.
    std::chrono::nanoseconds progressInterval{std::chrono::seconds{10}}; ///< Progress output interval.
    el::String workspace;                                                ///< Optional workspace root.
    bool keepFiles{false};                                               ///< Whether generated files are retained.
};

/// One fully expanded stream workload.
/// @notest{Covered through command-line dry runs and profiler smoke tests.}
struct Scenario {
    el::String id;                                                ///< Stable expanded identifier.
    el::String group;                                             ///< User-facing scenario group.
    Direction direction{Direction::Read};                         ///< Transfer direction.
    FileType fileType{FileType::Binary};                          ///< File type and encoding.
    Method method{Method::ReadSpan};                              ///< Stream method to call.
    Locality locality{Locality::Hot};                             ///< File selection locality.
    ChunkMode chunkMode{ChunkMode::Fixed};                        ///< Chunk scheduling strategy.
    std::uint64_t fileSizeMinimum{8ULL * 1024ULL * 1024ULL};      ///< Minimum file size.
    std::uint64_t fileSizeMaximum{8ULL * 1024ULL * 1024ULL};      ///< Maximum file size.
    std::uint64_t chunkSizeMinimum{64ULL * 1024ULL};              ///< Minimum transfer chunk.
    std::uint64_t chunkSizeMaximum{64ULL * 1024ULL};              ///< Maximum transfer chunk.
    el::StreamBuffering buffering{el::StreamBuffering::Balanced}; ///< Stream buffering intention.
    std::uint64_t backBufferLimit{}; ///< Explicit output back-buffer limit, or zero for the preset default.
    std::uint32_t weight{1U};        ///< Relative profile repetition weight.
};

/// The measurements and validation state produced by one workload thread.
/// @notest{Covered through profiler smoke tests.}
struct WorkerResult {
    std::uint64_t bytes{};
    std::uint64_t codePoints{};
    std::uint64_t calls{};
    std::uint64_t timeouts{};
    std::int64_t openNanoseconds{};
    std::int64_t transferNanoseconds{};
    std::int64_t closeNanoseconds{};
    std::uint64_t sink{};
    el::ByteBlock digest;
};

/// The synchronized aggregate from one concurrent sample.
/// @notest{Covered through profiler smoke tests.}
struct SampleResult {
    std::vector<WorkerResult> workers;
    std::int64_t wallNanoseconds{};
    std::uint64_t bytes{};
    std::uint64_t calls{};
    std::uint64_t timeouts{};
};

/// Convert a run mode into its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(RunMode value) -> el::String;
/// Convert a direction into its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(Direction value) -> el::String;
/// Convert a file type into its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(FileType value) -> el::String;
/// Convert a locality into its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(Locality value) -> el::String;
/// Convert a chunk mode into its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(ChunkMode value) -> el::String;
/// Convert a method into its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(Method value) -> el::String;
/// Convert a stream buffering intention into its configuration spelling.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto toString(el::StreamBuffering value) -> el::String;
/// Resolve the text encoding for a file type.
/// @notest{Trivial enum conversion.}
[[nodiscard]] auto encodingFor(FileType value) -> el::StringEncoding;
/// Test if a file type contains encoded text.
/// @notest{Trivial enum classification.}
[[nodiscard]] auto isTextFile(FileType value) noexcept -> bool;
/// Test if a method belongs to a text stream.
/// @notest{Trivial enum classification.}
[[nodiscard]] auto isTextMethod(Method value) noexcept -> bool;
/// Resolve the transfer direction for a method.
/// @notest{Trivial enum classification.}
[[nodiscard]] auto methodDirection(Method value) noexcept -> Direction;

}
