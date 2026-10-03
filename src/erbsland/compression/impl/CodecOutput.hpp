// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../err/LogicFailure.hpp"
#include "../../err/OutOfRangeError.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../mem/ByteBuffer.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>
#include <functional>

namespace erbsland::compression::impl {

/// Bounded output sink retaining only the requested match history.
/// @tested{CodecBufferTest CompressionStreamingTest}
class CodecOutput final {
public:
    /// Synchronous output callback.
    using Write = std::function<void(mem::ConstByteSpan)>;

public:
    /// Create a collecting output.
    CodecOutput() = default;
    // defaults
    CodecOutput(CodecOutput &&) = default;
    auto operator=(CodecOutput &&) -> CodecOutput & = default;
    CodecOutput(const CodecOutput &) = delete;
    auto operator=(const CodecOutput &) -> CodecOutput & = delete;
    /// Create a streaming output with a history window.
    CodecOutput(Write write, std::size_t history, unit::ByteCount maximum) :
        _write{std::move(write)}, _maximum{maximum} {
        _history.resize(unit::ByteCount::fromSizeT(history));
    }

public:
    /// Append one literal without constructing a span or entering the bulk-copy loop.
    /// @throws err::OutOfRangeError If the appended bytes exceed the configured output limit.
    void append(mem::Byte value) {
        checkLength(unit::ByteCount::one());
        prepareStorage();
        if (!_history.isEmpty()) {
            _history.setOrThrow(unit::ByteIndex::end(_length % _history.length().toRawValue()), value);
        }
        _data.append(value);
        _length += unit::ByteCount::one();
        flushIfFull();
    }
    /// Append a repeated byte while enforcing the total output limit before writing.
    /// @throws err::OutOfRangeError If the appended bytes exceed the configured output limit.
    void append(mem::Byte value, unit::ByteCount count) {
        if (count == unit::ByteCount::one()) {
            append(value);
            return;
        }
        checkLength(count);
        while (count.toRawValue() != 0U) {
            const auto part = available(count);
            const auto offset = _data.length().toSizeT();
            _data.append(value, part);
            appended(offset);
            count -= part;
        }
    }
    /// Append a span in bounded batches.
    /// @throws err::OutOfRangeError If the appended bytes exceed the configured output limit.
    void append(mem::ConstByteSpan data) {
        checkLength(unit::ByteCount::fromSizeT(data.size()));
        while (!data.empty()) {
            const auto count = available(unit::ByteCount::fromSizeT(data.size())).toSizeTOrThrow();
            const auto offset = _data.length().toSizeT();
            _data.append(data.first(count));
            appended(offset);
            data = data.subspan(count);
        }
    }
    /// Copy an overlapping match from prior output.
    /// @throws err::OutOfRangeError If the appended bytes exceed the configured output limit.
    void appendRepeated(unit::ByteRange range, unit::ByteCount count) {
        using namespace text::literals;
        checkLength(count);
        if (count.toRawValue() == 0U) {
            return;
        }
        const auto distance = range.beginIndex().absoluteDistanceTo(unit::ByteIndex::end(_length));
        const auto historyLength = _history.length();
        if (range.beginIndex() >= unit::ByteIndex::end(_length) || (_write && distance > historyLength)) {
            throw err::LogicFailure{"Codec history reference is out of range."_el};
        }
        while (count.toRawValue() != 0U) {
            auto part = available(count);
            const auto offset = _data.length();
            if (distance <= offset) {
                // The editor already implements overlapping repetition with bulk copies.
                _data.appendRepeated(unit::ByteRange{unit::ByteIndex::end(offset).retreated(distance), distance}, part);
            } else {
                const auto index = (_length - distance) % historyLength.toRawValue();
                part = std::min({part, distance, historyLength - index});
                _data.append(_history.span().subspan(index.toSizeTOrThrow(), part.toSizeTOrThrow()));
            }
            appended(offset.toSizeTOrThrow());
            count -= part;
        }
    }
    /// Read a retained output byte.
    /// @throws err::OutOfRangeError If a buffered output index is outside the available bytes.
    auto getOrThrow(unit::ByteIndex index) const -> mem::Byte {
        using namespace text::literals;
        if (!_write) {
            return _data.getOrThrow(index);
        }
        if (index >= unit::ByteIndex::end(_length) ||
            index.absoluteDistanceTo(unit::ByteIndex::end(_length)) > _history.length()) {
            throw err::LogicFailure{"Codec history reference is out of range."_el};
        }
        return _history.getOrThrow(unit::ByteIndex::end(index.distanceFromZero() % _history.length().toRawValue()));
    }
    /// Deliver pending output.
    void flush() {
        if (_write && !_data.isEmpty()) {
            _write(_data.span());
            _data.clear();
        }
    }
    /// Get the total output length.
    auto length() const noexcept -> unit::ByteCount { return _length; }
    /// Test whether no output was produced.
    auto isEmpty() const noexcept -> bool { return _length.toRawValue() == 0; }
    /// Obtain collected output; streaming output is drained first.
    operator mem::ByteBlock() {
        flush();
        return _write ? mem::ByteBlock{} : mem::ByteBlock{_data};
    }

private:
    /// Validate a complete append before accepting any of it.
    /// @throws err::OutOfRangeError If the appended bytes exceed the configured output limit.
    void checkLength(unit::ByteCount count) const {
        using namespace text::literals;
        if (_length.addedOrThrow(count) > _maximum) {
            throw err::OutOfRangeError{"Codec output exceeds its limit."_el};
        }
    }
    /// Prepare reusable storage before its first write.
    void prepareStorage() {
        if (_data.isEmpty()) {
            _data.reserve(unit::ByteCount{65536U});
        }
    }
    /// Choose a bounded append length.
    auto available(unit::ByteCount count) -> unit::ByteCount {
        prepareStorage();
        return _write ? std::min(count, unit::ByteCount{65536U} - _data.length()) : count;
    }
    /// Retain the newest history bytes and publish a complete output buffer.
    void appended(std::size_t offset) {
        const auto bytes = _data.span().subspan(offset);
        if (!_history.isEmpty()) {
            const auto historySize = _history.length().toSizeTOrThrow();
            const auto retained = bytes.last(std::min(bytes.size(), historySize));
            const auto index =
                static_cast<std::size_t>((_length.toRawValue() + bytes.size() - retained.size()) % historySize);
            const auto first = std::min(retained.size(), historySize - index);
            _history.overwrite(unit::ByteIndex::fromSizeT(index), retained.first(first));
            _history.overwrite(retained.subspan(first));
        }
        _length += unit::ByteCount::fromSizeT(bytes.size());
        flushIfFull();
    }
    /// Publish a complete output buffer.
    void flushIfFull() {
        if (_write && _data.length().toRawValue() == 65536U) {
            flush();
        }
    }

private:
    Write _write;                                          ///< Output consumer, absent for collection.
    mem::ByteBlockEditor _data;                            ///< Pending or collected output.
    mem::ByteBuffer _history;                              ///< Circular match history.
    unit::ByteCount _length;                               ///< Cumulative output length.
    unit::ByteCount _maximum{unit::ByteCount::infinite()}; ///< Total output limit.
};

}
