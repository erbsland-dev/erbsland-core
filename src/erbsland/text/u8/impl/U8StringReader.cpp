// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringReader.hpp"

#include "U8StringReadTools.hpp"

#include "../../AnyString.hpp"
#include "../../AnyStringEditor.hpp"

#include <utility>

namespace erbsland::text::impl {

using unit::ByteIndex;
using unit::CpIndex;
using unit::CpLength;
using util::LoopResult;
using util::LoopStatus;

U8StringReader::U8StringReader(U8String text) noexcept : _text{std::move(text)} {
}

auto U8StringReader::clone() const -> U8StringReader * {
    return new U8StringReader{*this};
}

auto U8StringReader::position() const noexcept -> CpIndex {
    return _cpPosition;
}

auto U8StringReader::save() const noexcept -> StringCharReaderState {
    return makeState(StringReaderBackendKind::U8, viewStorageId(_text), _position.toSizeT(), _cpPosition);
}

auto U8StringReader::restore(const StringCharReaderState state) noexcept -> bool {
    const auto raw = rawPosition(state);
    const auto rawIndex = ByteIndex::fromSizeT(raw);
    if (stateKind(state) != StringReaderBackendKind::U8 || storageId(state) != viewStorageId(_text) ||
        raw > _text.length().toSizeT() || _text.toCharIndex(rawIndex) != cpPosition(state)) {
        return false;
    }
    _position = rawIndex;
    _cpPosition = cpPosition(state);
    return true;
}

void U8StringReader::reset() noexcept {
    _position = ByteIndex::zero();
    _cpPosition = CpIndex::zero();
}

auto U8StringReader::read() noexcept -> Char {
    const auto result = U8StringReadTools{_text.dataView()}.read(_position);
    if (!result.isSignal()) {
        ++_cpPosition;
    }
    return result;
}

auto U8StringReader::readIf(const Char expected) noexcept -> bool {
    auto position = _position;
    const auto result = U8StringReadTools{_text.dataView()}.read(position);
    if (result.isSignal() || result != expected) {
        return false;
    }
    _position = position;
    ++_cpPosition;
    return true;
}

auto U8StringReader::readIf(const CharSet &expected) noexcept -> std::optional<Char> {
    auto position = _position;
    const auto result = U8StringReadTools{_text.dataView()}.read(position);
    if (result.isSignal() || !expected.contains(result)) {
        return {};
    }
    _position = position;
    ++_cpPosition;
    return result;
}

auto U8StringReader::peek() const noexcept -> Char {
    return U8StringReadTools{_text.dataView()}.charAt(_position);
}

auto U8StringReader::isAtEnd() const noexcept -> bool {
    return _position.distanceFromZero() >= _text.length();
}

auto U8StringReader::canRead(const CpLength count) const noexcept -> bool {
    if (count.isZero()) {
        return true;
    }
    if (count.isInfinite()) {
        return false;
    }
    auto position = _position;
    const auto tools = U8StringReadTools{_text.dataView()};
    for (auto i = CpLength{0}; i < count; ++i) {
        if (tools.read(position).isSignal()) {
            return false;
        }
    }
    return true;
}

auto U8StringReader::advance(const CpLength count) noexcept -> bool {
    if (count.isZero() || isAtEnd()) {
        return false;
    }
    for (auto i = CpLength{0}; i < count; ++i) {
        _text.advance(_position);
        ++_cpPosition;
        if (isAtEnd()) {
            break;
        }
    }
    return true;
}

auto U8StringReader::advanceIf(const Char expected) noexcept -> bool {
    return readIf(expected);
}

auto U8StringReader::advanceIf(const CharSet &expected) noexcept -> bool {
    return readIf(expected).has_value();
}

auto U8StringReader::readWhile(const ReadFn &readFn, const CharSet &expected, CpLength maximum) noexcept -> LoopResult {
    return readLoop(readFn, expected, maximum, false).first;
}

auto U8StringReader::readUntil(const ReadFn &readFn, const CharSet &stopSet, CpLength maximum) noexcept -> LoopResult {
    return readLoop(readFn, stopSet, maximum, true).first;
}

auto U8StringReader::advanceWhile(const CharSet &expected, const CpLength maximum) noexcept -> CpLength {
    return readLoop({}, expected, maximum, false).second;
}

auto U8StringReader::advanceUntil(const CharSet &stopSet, const CpLength maximum) noexcept -> CpLength {
    return readLoop({}, stopSet, maximum, true).second;
}

void U8StringReader::startCapture() noexcept {
    _captureStart = _position;
}

auto U8StringReader::takeCapture() noexcept -> AnyString {
    auto result = AnyString{};
    if (_captureStart.isNoIndex() || _captureStart >= _position) {
        _captureStart = _position;
        return result;
    }
    result = _text.slice({_captureStart, _position});
    _captureStart = _position;
    return result;
}

void U8StringReader::clearBuffer() noexcept {
    _buffer.clear();
    _bufferLength = CpLength::zero();
}

auto U8StringReader::takeBuffer() -> AnyString {
    auto result = AnyStringEditor{std::move(_buffer)};
    _buffer = U8StringEditor{};
    _bufferLength = CpLength::zero();
    return result;
}

auto U8StringReader::bufferView() const noexcept -> AnyString {
    return AnyString{_buffer};
}

auto U8StringReader::bufferCharacterLength() const noexcept -> CpLength {
    return _bufferLength;
}

auto U8StringReader::isBufferEmpty() const noexcept -> bool {
    return _bufferLength.isZero();
}

void U8StringReader::setBuffer(const AnyString &text) {
    _buffer = U8StringEditor{text.toU8String()};
    _bufferLength = text.characterLength();
}

void U8StringReader::appendToBuffer(const Char character) {
    if (!character.isValidUnicode()) {
        return;
    }
    _buffer.append(character);
    ++_bufferLength;
}

void U8StringReader::appendToBuffer(const AnyString &text) {
    if (text.isEmpty()) {
        return;
    }
    _buffer.append(text.toU8String());
    _bufferLength += text.characterLength();
}

void U8StringReader::appendCaptureToBuffer() {
    appendToBuffer(takeCapture());
}

auto U8StringReader::readToBuffer() -> Char {
    const auto character = peek();
    if (character.isSignal()) {
        return character;
    }
    appendToBuffer(character);
    if (!advance(CpLength::one())) {
        std::terminate();
    }
    return character;
}

auto U8StringReader::readToBufferIf(const Char expected) -> bool {
    const auto character = peek();
    if (character.isSignal() || character != expected) {
        return false;
    }
    appendToBuffer(character);
    if (!advance(CpLength::one())) {
        std::terminate();
    }
    return true;
}

auto U8StringReader::readToBufferIf(const CharSet &expected) -> std::optional<Char> {
    const auto character = peek();
    if (character.isSignal() || !expected.contains(character)) {
        return {};
    }
    appendToBuffer(character);
    if (!advance(CpLength::one())) {
        std::terminate();
    }
    return character;
}

auto U8StringReader::readToBufferWhile(const CharSet &expected, const CpLength maximum) -> LoopResult {
    return readToBufferLoop(expected, maximum, false);
}

auto U8StringReader::readToBufferUntil(const CharSet &stopSet, const CpLength maximum) -> LoopResult {
    return readToBufferLoop(stopSet, maximum, true);
}

auto U8StringReader::readLoop(const ReadFn &readFn, const CharSet &charSet, CpLength maximum, bool stopOnMatch) noexcept
    -> ReadLoopOutcome {
    auto count = CpLength::zero();
    while (true) {
        const auto lastPosition = _position;
        const auto lastCpPosition = _cpPosition;
        const auto character = read();
        if (character.isEndOfData()) {
            return {LoopResult::EndOfData, count};
        }
        if (charSet.contains(character) == stopOnMatch) {
            _position = lastPosition;
            _cpPosition = lastCpPosition;
            return {LoopResult::Success, count};
        }
        if (!maximum.isInfinite() && count >= maximum) {
            _position = lastPosition;
            _cpPosition = lastCpPosition;
            return {LoopResult::LimitReached, count};
        }
        const auto result = readFn != nullptr ? readFn(character) : LoopStatus::Continue;
        if (result != LoopStatus::Continue) {
            _position = lastPosition;
            _cpPosition = lastCpPosition;
            return {result == LoopStatus::Error ? LoopResult::Error : LoopResult::Stopped, count};
        }
        ++count;
    }
}

auto U8StringReader::readToBufferLoop(const CharSet &charSet, CpLength maximum, bool stopOnMatch) -> LoopResult {
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
        if (!advance(CpLength::one())) {
            std::terminate();
        }
        ++count;
    }
}

}
