// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BitReaderImpl.hpp"

#include <algorithm>

namespace erbsland::mem::impl {

/// Bit reader implementation specialized for one bit order.
/// @tested{BitReaderTest}
template <BitOrder tBitOrder>
class BitReaderImplFor final : public BitReaderImpl {
public:
    using BitReaderImpl::BitReaderImpl;

public:
    /// Get the compile-time bit order.
    [[nodiscard]] auto bitOrder() const noexcept -> BitOrder override { return tBitOrder; }

protected:
    /// Read a field using the compile-time bit order.
    [[nodiscard]] auto readBitsUnchecked(const std::size_t bitCountValue) noexcept -> uint64_t override {
        auto result = uint64_t{};
        auto remaining = bitCountValue;
        auto resultShift = std::size_t{};
        while (remaining != 0U) {
            const auto byteIndex = _bitPosition / 8U;
            const auto bitOffset = _bitPosition % 8U;
            const auto count = std::min(remaining, 8U - bitOffset);
            const auto mask = static_cast<uint8_t>((uint32_t{1U} << count) - uint32_t{1U});
            if constexpr (tBitOrder == BitOrder::MostSignificantFirst) {
                const auto shift = 8U - bitOffset - count;
                const auto value = (byteAt(byteIndex).toUInt8() >> shift) & mask;
                result = (result << count) | static_cast<uint64_t>(value);
            } else {
                const auto value = (byteAt(byteIndex).toUInt8() >> bitOffset) & mask;
                result |= static_cast<uint64_t>(value) << resultShift;
                resultShift += count;
            }
            _bitPosition += count;
            remaining -= count;
        }
        return result;
    }
};

}
