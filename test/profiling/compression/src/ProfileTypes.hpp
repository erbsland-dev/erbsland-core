// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>

#include <cstdint>

namespace app::compression {

/// The one-shot operation measured by a compression workload.
enum class ProfileOperation : std::uint8_t {
    Compress,   ///< Compress an immutable source block.
    Decompress, ///< Decompress a prepared encoded block.
};

/// The construction strategy for a corpus entry.
enum class CorpusKind : std::uint8_t {
    SeedFile,     ///< Repeat a checked-in seed file with deterministic separators.
    Runs,         ///< Generate runs of varied lengths and values.
    Matches,      ///< Generate near and long-distance matches with mutations.
    SparseBinary, ///< Generate zero-heavy binary records and random islands.
    Entropy,      ///< Generate uniformly distributed deterministic bytes.
    Mixed,        ///< Combine representative segments from all generated shapes.
};

/// One stable compression-corpus descriptor.
/// @notest{Covered by the compression profiler coverage and smoke tests.}
struct CorpusEntry {
    erbsland::String id;                  ///< Stable corpus identifier.
    erbsland::String description;         ///< Human-readable compression characteristics.
    CorpusKind kind{CorpusKind::Entropy}; ///< Data construction strategy.
    erbsland::String sourceFile{};        ///< Optional file relative to the corpus directory.
    std::uint64_t canonicalDigest{};      ///< FNV-1a digest for the canonical seed and size.
};

}
