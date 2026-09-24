// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"

#include <erbsland/all.hpp>

#include <cstdint>

namespace app::compression {

/// Loads and generates the deterministic compression corpus.
/// @notest{Covered by the compression profiler coverage and smoke tests.}
class Corpus final {
public:
    /// Create the stable compression corpus catalog.
    Corpus();

    /// Access the stable corpus catalog.
    [[nodiscard]] auto entries() const noexcept -> const erbsland::List<CorpusEntry> &;
    /// Find a corpus descriptor by identifier.
    [[nodiscard]] auto find(const erbsland::String &id) const noexcept -> const CorpusEntry *;
    /// Build an exact-size corpus value for the given global run seed.
    [[nodiscard]] auto build(const CorpusEntry &entry, erbsland::ByteLength size, std::uint64_t globalSeed) const
        -> erbsland::ByteBlock;
    /// Calculate the stable FNV-1a validation digest of a byte block.
    [[nodiscard]] auto digest(const erbsland::ByteBlock &data) const noexcept -> std::uint64_t;

private:
    /// Load one checked-in seed file.
    [[nodiscard]] auto loadSeed(const erbsland::String &fileName) const -> erbsland::ByteBlock;
    /// Expand a non-empty seed to an exact byte length.
    [[nodiscard]] auto expandSeed(
        const erbsland::ByteBlock &seed, erbsland::ByteLength size, std::uint64_t markerSeed) const
        -> erbsland::ByteBlock;
    /// Build varied byte runs.
    [[nodiscard]] auto buildRuns(erbsland::ByteLength size, std::uint64_t seed) const -> erbsland::ByteBlock;
    /// Build near and long-distance matches.
    [[nodiscard]] auto buildMatches(erbsland::ByteLength size, std::uint64_t seed) const -> erbsland::ByteBlock;
    /// Build sparse binary records.
    [[nodiscard]] auto buildSparseBinary(erbsland::ByteLength size, std::uint64_t seed) const -> erbsland::ByteBlock;
    /// Build uniformly distributed bytes.
    [[nodiscard]] auto buildEntropy(erbsland::ByteLength size, std::uint64_t seed) const -> erbsland::ByteBlock;
    /// Build a balanced mixture of corpus shapes.
    [[nodiscard]] auto buildMixed(erbsland::ByteLength size, std::uint64_t seed) const -> erbsland::ByteBlock;

private:
    erbsland::List<CorpusEntry> _entries; ///< Stable descriptors for all built-in corpora.
};

}
