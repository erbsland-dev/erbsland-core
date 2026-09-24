// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardReverseBitWriter.hpp"

#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>

namespace erbsland::compression::impl {

using namespace text::literals;

void ZstandardReverseBitWriter::append(const uint32_t value, const uint8_t count) {
    if (count > 32U || (count < 32U && value >= (uint32_t{1U} << count))) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard bit field does not fit."_el};
    }
    auto remaining = static_cast<std::size_t>(count);
    while (remaining != 0U) {
        const auto bitOffset = _bitCount % 8U;
        if (bitOffset == 0U) {
            _bytes.append(mem::Byte{});
        }
        const auto chunk = std::min(remaining, 8U - bitOffset);
        const auto valueShift = remaining - chunk;
        const auto mask = static_cast<uint8_t>((uint32_t{1U} << chunk) - uint32_t{1U});
        const auto byteShift = 8U - bitOffset - chunk;
        const auto bits = static_cast<uint8_t>((value >> valueShift) & mask);
        const auto index = unit::ByteIndex{_bytes.length().toRawValue() - 1U};
        _bytes.setOrThrow(index, _bytes.getOrThrow(index) | mem::Byte{static_cast<uint8_t>(bits << byteShift)});
        _bitCount += chunk;
        remaining -= chunk;
    }
}

auto ZstandardReverseBitWriter::takeBytes() -> mem::ByteBuffer {
    const auto firstCount = _bitCount % 8U;
    const auto groupCount = (_bitCount - firstCount) / 8U;
    auto output = mem::ByteBuffer{};
    output.reserve(unit::ByteLength::fromSizeT(groupCount + 1U));
    for (auto group = groupCount; group > 0U; --group) {
        output.append(mem::Byte{read(firstCount + (group - 1U) * 8U, 8U)});
    }
    output.append(mem::Byte{static_cast<uint8_t>((uint32_t{1U} << firstCount) | read(0U, firstCount))});
    _bytes.clear();
    _bitCount = 0U;
    return output;
}

auto ZstandardReverseBitWriter::read(std::size_t position, std::size_t count) const noexcept -> uint8_t {
    auto result = uint8_t{};
    while (count != 0U) {
        const auto bitOffset = position % 8U;
        const auto chunk = std::min(count, 8U - bitOffset);
        const auto shift = 8U - bitOffset - chunk;
        const auto mask = static_cast<uint8_t>((uint32_t{1U} << chunk) - uint32_t{1U});
        const auto byte = _bytes.get(unit::ByteIndex::fromSizeT(position / 8U));
        result = static_cast<uint8_t>((result << chunk) | ((byte.toUInt8() >> shift) & mask));
        position += chunk;
        count -= chunk;
    }
    return result;
}

}
