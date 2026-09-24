// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardForwardBitReader.hpp"

#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <limits>

namespace erbsland::compression::impl {

using namespace text::literals;

ZstandardForwardBitReader::ZstandardForwardBitReader(const mem::ConstByteSpan data, const std::size_t position) noexcept
    :
    _data{data}, _position{position} {
}

auto ZstandardForwardBitReader::ensure(const std::size_t count) -> bool {
    if (count > 32U) {
        return false;
    }
    while (_count < count && _position < _data.size()) {
        _bits |= _data[_position++].toUInt64() << _count;
        _count += 8U;
    }
    return _count >= count;
}

auto ZstandardForwardBitReader::peek(const std::size_t count) -> uint32_t {
    if (!ensure(count)) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard table description is truncated."_el};
    }
    const auto mask = count == 32U ? std::numeric_limits<uint32_t>::max() : ((uint32_t{1U} << count) - 1U);
    return static_cast<uint32_t>(_bits) & mask;
}

auto ZstandardForwardBitReader::read(const std::size_t count) -> uint32_t {
    const auto result = peek(count);
    _bits >>= count;
    _count -= count;
    return result;
}

auto ZstandardForwardBitReader::consumedBytePosition() const noexcept -> std::size_t {
    return _position - _count / 8U;
}

}
