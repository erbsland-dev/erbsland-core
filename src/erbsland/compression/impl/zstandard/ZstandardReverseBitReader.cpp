// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardReverseBitReader.hpp"

#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>
#include <bit>
#include <limits>

namespace erbsland::compression::impl {

using namespace text::literals;

ZstandardReverseBitReader::ZstandardReverseBitReader(
    const mem::ConstByteSpan data, const std::size_t begin, const std::size_t end) :
    _data{data}, _begin{begin}, _position{end} {
    if (begin >= end || end > data.size()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard reverse bitstream is empty."_el};
    }
    const auto last = data[end - 1U].toUInt8();
    if (last == 0U) {
        throw CompressionError{
            CompressionErrorReason::MalformedData, "Zstandard reverse bitstream has no end marker."_el};
    }
    _position = end - 1U;
    _bits = last;
    _count = static_cast<std::size_t>(std::bit_width(last)) - 1U;
}

auto ZstandardReverseBitReader::canRead(const std::size_t count) noexcept -> bool {
    if (count > 32U) {
        return false;
    }
    while (_count < count && _position > _begin) {
        --_position;
        _bits = (_bits << 8U) | _data[_position].toUInt64();
        _count += 8U;
    }
    return _count >= count;
}

auto ZstandardReverseBitReader::read(const std::size_t count) -> uint32_t {
    if (!canRead(count)) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard reverse bitstream is truncated."_el};
    }
    _count -= count;
    const auto mask = count == 32U ? std::numeric_limits<uint32_t>::max() : ((uint64_t{1U} << count) - 1U);
    return static_cast<uint32_t>((_bits >> _count) & mask);
}

void ZstandardReverseBitReader::discard(const std::size_t count) {
    if (count > _count) {
        throw CompressionError{
            CompressionErrorReason::MalformedData, "Zstandard bit discard exceeds buffered bits."_el};
    }
    _count -= count;
}

auto ZstandardReverseBitReader::peek(const std::size_t count) -> uint32_t {
    if (!canRead(count)) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard reverse bitstream is truncated."_el};
    }
    const auto mask = count == 32U ? std::numeric_limits<uint32_t>::max() : ((uint64_t{1U} << count) - 1U);
    return static_cast<uint32_t>((_bits >> (_count - count)) & mask);
}

auto ZstandardReverseBitReader::peekPadded(const std::size_t count) -> uint32_t {
    if (count > 32U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard bit lookahead exceeds 32 bits."_el};
    }
    while (_position > _begin && _count < count) {
        --_position;
        _bits = (_bits << 8U) | _data[_position].toUInt64();
        _count += 8U;
    }
    const auto available = std::min(count, _count);
    const auto mask = available == 32U ? std::numeric_limits<uint32_t>::max() : ((uint64_t{1U} << available) - 1U);
    return static_cast<uint32_t>(((_bits >> (_count - available)) & mask) << (count - available));
}

auto ZstandardReverseBitReader::remainingBitCount() const noexcept -> std::size_t {
    return _count + (_position - _begin) * 8U;
}

auto ZstandardReverseBitReader::isAtEnd() const noexcept -> bool {
    return _count == 0U && _position == _begin;
}

}
