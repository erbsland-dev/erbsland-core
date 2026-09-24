// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZstandardFseEntry.hpp"

#include "../../../mem/ByteSpan.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::compression::impl {

/// A validated finite-state entropy decoding table.
/// @tested{ZstandardInternalTest}
class ZstandardFseTable final {
public:
    /// Read a normalized-count description and replace this table.
    void read(mem::ConstByteSpan data, std::size_t &position, uint8_t maximumSymbol, uint8_t maximumAccuracyLog);
    /// Build a table from normalized counts.
    void build(std::span<const int16_t> normalizedCounts, uint8_t accuracyLog);
    /// Build the one-state table used by RLE mode.
    void setRle(uint8_t symbol) noexcept;

public: // access
    /// Access a state, rejecting an out-of-range state.
    [[nodiscard]] auto entry(uint32_t state) const -> const ZstandardFseEntry &;
    /// Return the accuracy log used by this table.
    [[nodiscard]] auto accuracyLog() const noexcept -> uint8_t { return _accuracyLog; }
    /// Test whether a table has been installed.
    [[nodiscard]] auto isValid() const noexcept -> bool { return _size != 0U; }

private:
    std::array<ZstandardFseEntry, 512U> _entries{}; ///< State decoding table.
    std::size_t _size{};                            ///< Number of active states.
    uint8_t _accuracyLog{};                         ///< Logarithm of `_size`.
};

}
