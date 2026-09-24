// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CodecReader.hpp"

#include "../../mem/BitReader.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::compression::impl {

/// Adapts bounded codec chunks to the shared memory bit reader.
/// @tested{CodecBufferTest CompressionStreamingTest}
class CodecBitReader final {
public:
    /// Create an adapter over borrowed codec input.
    CodecBitReader(CodecReader &source, mem::BitOrder order = mem::BitOrder::MostSignificantFirst) :
        _reader{mem::ByteBlock{}, order}, _source{source} {}

public:
    /// Buffer a field, retaining partial bytes across input chunks.
    auto canRead(std::size_t count) -> bool {
        if (count > 64U) {
            return false;
        }
        while (!_reader.canRead(count) && !_source.atEnd()) {
            _reader.refill(_source.take());
        }
        return _reader.canRead(count);
    }
    /// Expose buffered bits for bounded Huffman lookahead.
    auto buffered(std::size_t count) -> mem::BitReader & {
        canRead(count);
        return _reader;
    }
    /// Read a field or reject truncation.
    auto readBits(std::size_t count) -> uint64_t {
        using namespace text::literals;
        if (!canRead(count)) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Truncated codec bit field."_el};
        }
        return _reader.readBits(count);
    }
    /// Read a byte.
    auto readByte() -> mem::Byte { return mem::Byte{static_cast<uint8_t>(readBits(8U))}; }
    /// Read a nonempty bounded portion of an aligned payload, rejecting truncation.
    auto readBytes(std::size_t maximum) -> mem::ByteBlock {
        using namespace text::literals;
        if (maximum == 0U) {
            return {};
        }
        if (!canRead(8U)) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Truncated codec byte field."_el};
        }
        return _reader.readBytesOrThrow(
            unit::ByteLength::fromSizeT(std::min(maximum, _reader.remainingBitCount() / 8U)));
    }
    /// Check a byte field; larger streamed fields are validated as they are read.
    auto canReadBytes(std::size_t count) -> bool { return count <= 8U ? canRead(count * 8U) : true; }
    /// Align to a byte boundary.
    void alignToByte() { _reader.alignToByte(); }
    /// Require no unread complete bytes.
    void requireEnd() {
        using namespace text::literals;
        if (_reader.consumedByteCount() != _reader.byteCount()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Trailing codec bytes."_el};
        }
        _source.requireEnd();
    }

private:
    mem::BitReader _reader; ///< Shared bit extraction and cursor implementation.
    CodecReader &_source;   ///< Borrowed chunk source.
};

}
