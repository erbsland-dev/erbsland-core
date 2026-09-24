// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BitWriterImpl.hpp"

#include <algorithm>

namespace erbsland::mem::impl {

/// Bit writer implementation specialized for one bit order.
/// @tested{BitWriterTest}
template <BitOrder tBitOrder>
class BitWriterImplFor final : public BitWriterImpl {
public:
    /// Get the compile-time bit order.
    [[nodiscard]] auto bitOrder() const noexcept -> BitOrder override { return tBitOrder; }

protected:
    /// Write a field using the compile-time bit order.
    [[nodiscard]] auto writeBitsUnchecked(const uint64_t value, const std::size_t bitCountValue)
        -> std::size_t override {
        prepareWrite(bitCountValue);
        const auto endPosition = _bitPosition + bitCountValue;
        auto data = writableSpan();
        auto position = _bitPosition;
        auto written = std::size_t{};
        while (written < bitCountValue) {
            const auto byteIndex = position / 8U;
            const auto bitOffset = position % 8U;
            const auto count = std::min(bitCountValue - written, 8U - bitOffset);
            const auto valueMask = static_cast<uint8_t>((uint32_t{1U} << count) - uint32_t{1U});
            std::size_t valueShift;
            std::size_t byteShift;
            if constexpr (tBitOrder == BitOrder::MostSignificantFirst) {
                valueShift = bitCountValue - written - count;
                byteShift = 8U - bitOffset - count;
            } else {
                valueShift = written;
                byteShift = bitOffset;
            }
            const auto mask = static_cast<uint8_t>(valueMask << byteShift);
            const auto bits = static_cast<uint8_t>((value >> valueShift) & valueMask);
            auto byte = data[byteIndex].toUInt8();
            byte = static_cast<uint8_t>(byte & static_cast<uint8_t>(~mask));
            byte = static_cast<uint8_t>(byte | static_cast<uint8_t>(bits << byteShift));
            data[byteIndex] = Byte{byte};
            position += count;
            written += count;
        }
        return endPosition;
    }
};

}
