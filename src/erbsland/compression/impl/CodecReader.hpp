// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CompressionError.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>
#include <functional>
#include <utility>

namespace erbsland::compression::impl {

/// Synchronous bounded byte source used by codec algorithms.
/// @tested{CodecBufferTest CompressionStreamingTest}
class CodecReader final {
public:
    /// Callback returning the next bounded chunk; empty means end.
    using Read = std::function<mem::ByteBlock()>;

public:
    /// Create a callback-backed reader.
    explicit CodecReader(Read read) : _read{std::move(read)} {}

public:
    /// Read a byte, rejecting truncation.
    auto byte() -> mem::Byte {
        using namespace text::literals;
        if (atEnd()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Compressed input is truncated."_el};
        }
        return _data.get(unit::ByteIndex::fromSizeT(_position++));
    }
    /// Test source exhaustion without consuming a byte.
    auto atEnd() -> bool {
        if (_position == _data.length().toSizeT() && !_finished) {
            _data = _read();
            _position = 0;
            _finished = _data.isEmpty();
        }
        return _finished;
    }
    /// Take the unread part of the current chunk without copying its storage.
    auto take() -> mem::ByteBlock {
        if (atEnd()) {
            return {};
        }
        const auto result = _data.slice(unit::ByteIndex::fromSizeT(_position), _data.length());
        _position = _data.length().toSizeT();
        return result;
    }
    /// Read at most one bounded block, assembling short source chunks.
    auto block(const std::size_t maximum) -> mem::ByteBlock {
        if (maximum == 0U || atEnd()) {
            return {};
        }
        if (_data.length().toSizeT() - _position >= maximum) {
            const auto result =
                _data.slice(unit::ByteIndex::fromSizeT(_position), unit::ByteLength::fromSizeT(maximum));
            _position += maximum;
            return result;
        }
        mem::ByteBlockEditor result;
        result.reserve(unit::ByteLength::fromSizeT(maximum));
        while (result.length().toSizeT() < maximum && !atEnd()) {
            const auto count = std::min(maximum - result.length().toSizeT(), _data.length().toSizeT() - _position);
            result.append(_data.span().subspan(_position, count));
            _position += count;
        }
        return result;
    }
    /// Read an exact little-endian integer field.
    auto little(const std::size_t count) -> uint64_t {
        using namespace text::literals;
        if (count > sizeof(uint64_t)) {
            throw err::OutOfRangeError{"Codec integer field exceeds 64 bits."_el};
        }
        uint64_t value{};
        for (std::size_t i{}; i < count; ++i) {
            value |= byte().toUInt64() << (i * 8U);
        }
        return value;
    }
    /// Require the end of the supplied representation.
    void requireEnd() {
        using namespace text::literals;
        if (!atEnd()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Compressed input has trailing data."_el};
        }
    }

private:
    Read _read;              ///< Source callback.
    mem::ByteBlock _data;    ///< Current bounded chunk.
    std::size_t _position{}; ///< Position in chunk.
    bool _finished{};        ///< Source exhausted.
};

}
