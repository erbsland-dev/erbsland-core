// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RetainedTextBuffer.hpp"

#include "../../err/LogicError.hpp"
#include "../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../text/impl/UnsafeU8StringBuffer.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>

namespace erbsland::stream::impl {

using namespace text;
using namespace text::literals;
using namespace unit;

void RetainedTextBuffer::setSensitive(const bool sensitive) noexcept {
    clear();
    _sensitive = sensitive;
}

void RetainedTextBuffer::clear() noexcept {
    _chunks.clear();
    _length = {};
    _lineScanLength = {};
    _firstLineLength.reset();
}

void RetainedTextBuffer::append(String text, const CpLength textLength, const bool lineScanned) {
    if (text.isEmpty() != textLength.isZero()) {
        throw err::LogicError{"Decoded text and its character length do not match"_el};
    }
    if (text.isEmpty()) {
        return;
    }
    if (text.isSensitive() != _sensitive) {
        throw err::LogicError{"Decoded text sensitivity does not match the retained stream policy"_el};
    }
    if (lineScanned && _lineScanLength == _length) {
        _lineScanLength += textLength;
        const auto bytes = text::impl::UnsafeU8StringAccess{text}.dataView().dataSpan();
        if (!bytes.empty() && bytes.back() == '\n') {
            _firstLineLength = _lineScanLength;
        }
    }
    _length += textLength;
    _chunks.emplace_back(std::move(text), textLength);
}

auto RetainedTextBuffer::lineIsComplete(const CpLength maximum) noexcept -> bool {
    scanForLineEnd(maximum);
    return _length >= maximum || (_firstLineLength && *_firstLineLength <= maximum);
}

auto RetainedTextBuffer::takeLength(const CpLength maximum, const bool line) noexcept -> CpLength {
    if (line) {
        scanForLineEnd(maximum);
    }
    auto result = std::min(maximum, _length);
    if (line && _firstLineLength) {
        result = std::min(result, *_firstLineLength);
    }
    return result;
}

void RetainedTextBuffer::scanForLineEnd(const CpLength maximum) noexcept {
    if (_firstLineLength || _lineScanLength >= maximum || _lineScanLength >= _length) {
        return;
    }
    const auto target = std::min(maximum, _length);
    auto chunkOffset = CpLength{};
    for (const auto &chunk : _chunks) {
        if (chunkOffset + chunk.length <= _lineScanLength) {
            chunkOffset += chunk.length;
            continue;
        }
        const auto skip = _lineScanLength > chunkOffset ? _lineScanLength - chunkOffset : CpLength{};
        auto localLength = CpLength{};
        const auto bytes = text::impl::UnsafeU8StringAccess{chunk.text}.dataView().dataSpan();
        for (const auto byte : bytes) {
            const auto value = static_cast<std::uint8_t>(byte);
            if ((value & 0xc0U) == 0x80U) {
                continue;
            }
            if (localLength++ < skip) {
                continue;
            }
            if (_lineScanLength >= target) {
                return;
            }
            ++_lineScanLength;
            if (byte == '\n') {
                _firstLineLength = _lineScanLength;
                return;
            }
        }
        chunkOffset += chunk.length;
    }
}

void RetainedTextBuffer::didConsume(const CpLength length) noexcept {
    _length -= length;
    if (_firstLineLength && length < *_firstLineLength) {
        *_firstLineLength -= length;
        _lineScanLength -= length;
        return;
    }
    if (_firstLineLength) {
        _firstLineLength.reset();
        _lineScanLength = {};
        return;
    }
    _lineScanLength = length < _lineScanLength ? _lineScanLength - length : CpLength{};
}

void RetainedTextBuffer::consumePrefix(const CpLength characterLength) {
    auto remaining = characterLength;
    while (!remaining.isZero()) {
        auto &chunk = _chunks.front();
        const auto count = std::min(remaining, chunk.length);
        if (count == chunk.length) {
            _chunks.pop_front();
        } else {
            chunk.text = chunk.text.slice(CpRange{CpIndex{count.toRawValue()}, CpLength::infinite()});
            chunk.length -= count;
        }
        remaining -= count;
    }
    didConsume(characterLength);
}

auto RetainedTextBuffer::takeChar() -> Char {
    if (isEmpty()) {
        return {};
    }
    const auto result = _chunks.front().text.charAt(CpIndex{});
    consumePrefix(CpLength::one());
    return result;
}

auto RetainedTextBuffer::take(const CpLength maximum, const bool line) -> String {
    const auto count = takeLength(maximum, line);
    if (count.isZero()) {
        return {};
    }
    if (count == _chunks.front().length) {
        auto result = std::move(_chunks.front().text);
        _chunks.pop_front();
        didConsume(count);
        return result;
    }
    if (count < _chunks.front().length) {
        auto [result, suffix] = _chunks.front().text.splitAt(CpIndex{count.toRawValue()});
        _chunks.front().text = std::move(suffix);
        _chunks.front().length -= count;
        didConsume(count);
        return result;
    }
    const auto resultByteLength = byteLength(count);
    auto buffer = text::impl::UnsafeU8StringBuffer{resultByteLength, _sensitive};
    copyAndConsume(count, std::span<char>{buffer.data(), resultByteLength.toSizeT()});
    return String{buffer.take(resultByteLength)};
}

auto RetainedTextBuffer::byteLength(const CpLength characterLength) const -> ByteLength {
    auto remaining = characterLength;
    auto result = ByteLength{};
    for (const auto &chunk : _chunks) {
        const auto count = std::min(remaining, chunk.length);
        result += chunk.text.slice(CpRange{CpIndex{}, count}).length();
        remaining -= count;
        if (remaining.isZero()) {
            break;
        }
    }
    return result;
}

void RetainedTextBuffer::copyAndConsume(const CpLength characterLength, const std::span<char> destination) {
    auto remaining = characterLength;
    auto offset = std::size_t{};
    while (!remaining.isZero()) {
        auto &chunk = _chunks.front();
        const auto count = std::min(remaining, chunk.length);
        const auto prefix = chunk.text.slice(CpRange{CpIndex{}, count});
        const auto source = text::impl::UnsafeU8StringAccess{prefix}.dataView().dataSpan();
        std::memcpy(destination.data() + offset, source.data(), source.size());
        offset += source.size();
        remaining -= count;
        if (count == chunk.length) {
            _chunks.pop_front();
        } else {
            chunk.text = chunk.text.slice(CpRange{CpIndex{count.toRawValue()}, CpLength::infinite()});
            chunk.length -= count;
        }
    }
    didConsume(characterLength);
}

}
