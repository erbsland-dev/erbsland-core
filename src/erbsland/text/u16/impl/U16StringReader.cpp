// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringReader.hpp"

#include "U16StringReadTools.hpp"

#include "../../AnyString.hpp"
#include "../../AnyStringEditor.hpp"

#include <utility>

namespace erbsland::text::impl {

using unit::CpIndex;
using unit::CpLength;
using unit::U16DataIndex;
using util::LoopResult;
using util::LoopStatus;

U16StringReader::U16StringReader(U16String text) noexcept : _text{std::move(text)} {
}

auto U16StringReader::clone() const -> U16StringReader * {
    return new U16StringReader{*this};
}

auto U16StringReader::position() const noexcept -> CpIndex {
    return _cpPosition;
}

auto U16StringReader::save() const noexcept -> StringCharReaderState {
    return makeState(StringReaderBackendKind::U16, viewStorageId(_text), _position.toSizeT(), _cpPosition);
}

auto U16StringReader::restore(const StringCharReaderState state) noexcept -> bool {
    const auto raw = rawPosition(state);
    const auto rawIndex = U16DataIndex::fromSizeT(raw);
    if (stateKind(state) != StringReaderBackendKind::U16 || storageId(state) != viewStorageId(_text) ||
        raw > _text.length().toSizeT() || _text.toCharIndex(rawIndex) != cpPosition(state)) {
        return false;
    }
    _position = rawIndex;
    _cpPosition = cpPosition(state);
    return true;
}

void U16StringReader::reset() noexcept {
    _position = U16DataIndex::zero();
    _cpPosition = CpIndex::zero();
}

auto U16StringReader::read() noexcept -> Char {
    const auto result = U16StringReadTools{_text.dataView()}.read(_position);
    if (!result.isSignal()) {
        ++_cpPosition;
    }
    return result;
}

auto U16StringReader::readIf(const Char expected) noexcept -> bool {
    auto position = _position;
    const auto result = U16StringReadTools{_text.dataView()}.read(position);
    if (result.isSignal() || result != expected) {
        return false;
    }
    _position = position;
    ++_cpPosition;
    return true;
}

auto U16StringReader::readIf(const CharSet &expected) noexcept -> std::optional<Char> {
    auto position = _position;
    const auto result = U16StringReadTools{_text.dataView()}.read(position);
    if (result.isSignal() || !expected.contains(result)) {
        return {};
    }
    _position = position;
    ++_cpPosition;
    return result;
}

auto U16StringReader::peek() const noexcept -> Char {
    return U16StringReadTools{_text.dataView()}.charAt(_position);
}

auto U16StringReader::readOrThrow() -> Char {
    const auto result = U16StringReadTools{_text.dataView()}.readOrThrow(_position);
    ++_cpPosition;
    return result;
}

auto U16StringReader::readIfOrThrow(const Char expected) -> bool {
    auto position = _position;
    const auto result = U16StringReadTools{_text.dataView()}.readOrThrow(position);
    if (result != expected) {
        return false;
    }
    _position = position;
    ++_cpPosition;
    return true;
}

auto U16StringReader::readIfOrThrow(const CharSet &expected) -> std::optional<Char> {
    auto position = _position;
    const auto result = U16StringReadTools{_text.dataView()}.readOrThrow(position);
    if (!expected.contains(result)) {
        return {};
    }
    _position = position;
    ++_cpPosition;
    return result;
}

auto U16StringReader::peekOrThrow() const -> Char {
    return U16StringReadTools{_text.dataView()}.charAtOrThrow(_position);
}

auto U16StringReader::isAtEnd() const noexcept -> bool {
    return peek().isEndOfData();
}

auto U16StringReader::canRead(CpLength count) const noexcept -> bool {
    if (count.isZero()) {
        return true;
    }
    if (count.isInfinite()) {
        return false;
    }
    auto position = _position;
    while (!count.isZero()) {
        const auto result = U16StringReadTools{_text.dataView()}.charAt(position);
        if (result.isSignal()) {
            return false;
        }
        _text.advance(position);
        --count;
    }
    return true;
}

auto U16StringReader::advance(CpLength count) noexcept -> bool {
    if (count.isZero()) {
        return false;
    }
    auto result = false;
    auto tools = U16StringReadTools{_text.dataView()};
    while (count.isInfinite() || !count.isZero()) {
        if (!tools.advance(_position)) {
            break;
        }
        ++_cpPosition;
        result = true;
        if (!count.isInfinite()) {
            --count;
        }
    }
    return result;
}

auto U16StringReader::advanceIf(const Char expected) noexcept -> bool {
    return readIf(expected);
}

auto U16StringReader::advanceIf(const CharSet &expected) noexcept -> bool {
    return readIf(expected).has_value();
}

auto U16StringReader::advanceIfOrThrow(const Char expected) -> bool {
    return readIfOrThrow(expected);
}

auto U16StringReader::advanceIfOrThrow(const CharSet &expected) -> bool {
    return readIfOrThrow(expected).has_value();
}

auto U16StringReader::readWhile(const ReadFn &readFn, const CharSet &expected, CpLength maximum) noexcept
    -> LoopResult {
    return readLoop(readFn, expected, maximum, false);
}

auto U16StringReader::readUntil(const ReadFn &readFn, const CharSet &stopSet, CpLength maximum) noexcept -> LoopResult {
    return readLoop(readFn, stopSet, maximum, true);
}

void U16StringReader::startCapture() noexcept {
    _captureStart = _position;
}

auto U16StringReader::takeCapture() noexcept -> AnyString {
    auto result = AnyString{};
    if (_captureStart.isNoIndex() || _captureStart >= _position) {
        _captureStart = _position;
        return result;
    }
    result = _text.slice({_captureStart, _position});
    _captureStart = _position;
    return result;
}

void U16StringReader::clearBuffer() noexcept {
    _buffer.clear();
    _bufferLength = CpLength::zero();
}

auto U16StringReader::takeBuffer() -> AnyString {
    auto result = AnyStringEditor{std::move(_buffer)};
    _buffer = U16StringEditor{};
    _bufferLength = CpLength::zero();
    return result;
}

auto U16StringReader::bufferView() const noexcept -> AnyString {
    return AnyString{_buffer};
}

auto U16StringReader::bufferCharacterLength() const noexcept -> CpLength {
    return _bufferLength;
}

auto U16StringReader::isBufferEmpty() const noexcept -> bool {
    return _bufferLength.isZero();
}

void U16StringReader::setBuffer(const AnyString &text) {
    _buffer = U16StringEditor{text.toU16String()};
    _bufferLength = text.characterLength();
}

void U16StringReader::appendToBuffer(const Char character) {
    if (!character.isValidUnicode()) {
        return;
    }
    _buffer.append(character);
    ++_bufferLength;
}

void U16StringReader::appendToBuffer(const AnyString &text) {
    if (text.isEmpty()) {
        return;
    }
    _buffer.append(text.toU16String());
    _bufferLength += text.characterLength();
}

void U16StringReader::appendCaptureToBuffer() {
    appendToBuffer(takeCapture());
}

auto U16StringReader::readToBuffer() -> Char {
    const auto character = peek();
    if (character.isSignal()) {
        return character;
    }
    appendToBuffer(character);
    static_cast<void>(advance(CpLength::one()));
    return character;
}

auto U16StringReader::readToBufferIf(const Char expected) -> bool {
    const auto character = peek();
    if (character.isSignal() || character != expected) {
        return false;
    }
    appendToBuffer(character);
    static_cast<void>(advance(CpLength::one()));
    return true;
}

auto U16StringReader::readToBufferIf(const CharSet &expected) -> std::optional<Char> {
    const auto character = peek();
    if (character.isSignal() || !expected.contains(character)) {
        return {};
    }
    appendToBuffer(character);
    static_cast<void>(advance(CpLength::one()));
    return character;
}

auto U16StringReader::readToBufferWhile(const CharSet &expected, const CpLength maximum) -> LoopResult {
    return readToBufferLoop(expected, maximum, false);
}

auto U16StringReader::readToBufferUntil(const CharSet &stopSet, const CpLength maximum) -> LoopResult {
    return readToBufferLoop(stopSet, maximum, true);
}

auto U16StringReader::readLoop(
    const ReadFn &readFn, const CharSet &charSet, CpLength maximum, bool stopOnMatch) noexcept -> LoopResult {
    auto count = CpLength::zero();
    while (true) {
        const auto lastPosition = _position;
        const auto lastCpPosition = _cpPosition;
        const auto character = read();
        if (character.isEndOfData()) {
            return LoopResult::EndOfData;
        }
        if (charSet.contains(character) == stopOnMatch) {
            _position = lastPosition;
            _cpPosition = lastCpPosition;
            return LoopResult::Success;
        }
        if (!maximum.isInfinite() && count >= maximum) {
            _position = lastPosition;
            _cpPosition = lastCpPosition;
            return LoopResult::LimitReached;
        }
        const auto result = readFn != nullptr ? readFn(character) : LoopStatus::Continue;
        if (result != LoopStatus::Continue) {
            _position = lastPosition;
            _cpPosition = lastCpPosition;
            return result == LoopStatus::Error ? LoopResult::Error : LoopResult::Stopped;
        }
        ++count;
    }
}

auto U16StringReader::readToBufferLoop(const CharSet &charSet, CpLength maximum, bool stopOnMatch) -> LoopResult {
    auto count = CpLength::zero();
    while (true) {
        const auto character = peek();
        if (character.isEndOfData()) {
            return LoopResult::EndOfData;
        }
        if (charSet.contains(character) == stopOnMatch) {
            return LoopResult::Success;
        }
        if (!maximum.isInfinite() && count >= maximum) {
            return LoopResult::LimitReached;
        }
        appendToBuffer(character);
        static_cast<void>(advance(CpLength::one()));
        ++count;
    }
}

}
