// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZstandardFseTable.hpp"
#include "ZstandardSequenceCode.hpp"
#include "ZstandardSequenceEntry.hpp"

#include "../../../mem/ByteSpan.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace erbsland::compression::impl {

/// A sequence-code table with predefined, RLE, compressed, and repeat modes.
/// @tested{ZstandardInternalTest}
class ZstandardSequenceTable final {
public:
    /// Install the format-defined predefined table.
    void setPredefined(ZstandardSequenceCode kind);
    /// Install a single-symbol RLE table.
    void setRle(ZstandardSequenceCode kind, uint8_t symbol);
    /// Read and install an FSE-compressed table.
    void readCompressed(mem::ConstByteSpan data, std::size_t &position, ZstandardSequenceCode kind);

public: // access
    /// Access a state, rejecting an out-of-range state.
    [[nodiscard]] auto entry(uint32_t state) const -> const ZstandardSequenceEntry &;
    /// Return the number of bits in an initial state.
    [[nodiscard]] auto accuracyLog() const noexcept -> uint8_t { return _accuracyLog; }
    /// Return the number of active states.
    [[nodiscard]] auto stateCount() const noexcept -> std::size_t { return _size; }
    /// Test whether a previous table is available for repeat mode.
    [[nodiscard]] auto isValid() const noexcept -> bool { return _size != 0U; }

private:
    /// Convert a raw FSE table into decoded sequence values.
    void convert(const ZstandardFseTable &table, ZstandardSequenceCode kind);
    /// Convert one symbol into its decoded value representation.
    [[nodiscard]] static auto valueFor(uint8_t symbol, ZstandardSequenceCode kind) -> ZstandardSequenceEntry;

private:
    std::array<ZstandardSequenceEntry, 512U> _entries{}; ///< Sequence decoding table.
    std::size_t _size{};                                 ///< Number of active states.
    uint8_t _accuracyLog{};                              ///< Logarithm of `_size`.
};

}
