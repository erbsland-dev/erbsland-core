// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringReader.hpp"

#include "U32StringReadTools.hpp"

#include "../../AnyString.hpp"
#include "../../AnyStringEditor.hpp"

#include <utility>

namespace erbsland::text::impl {

using unit::CpIndex;
using unit::CpLength;
using util::LoopResult;
using util::LoopStatus;

U32StringReader::U32StringReader(U32String text) noexcept : _text{std::move(text)} {
}

auto U32StringReader::clone() const -> U32StringReader * {
    return new U32StringReader{*this};
}

auto U32StringReader::position() const noexcept -> CpIndex {
    return _cpPosition;
}

auto U32StringReader::save() const noexcept -> StringCharReaderState {
    return makeState(StringReaderBackendKind::U32, viewStorageId(_text), _position.toSizeT(), _cpPosition);
}

auto U32StringReader::restore(const StringCharReaderState state) noexcept -> bool {
    const auto raw = rawPosition(state);
    const auto rawIndex = CpIndex::fromSizeT(raw);
    if (stateKind(state) != StringReaderBackendKind::U32 || storageId(state) != viewStorageId(_text) ||
        raw > _text.length().toSizeT() || _text.toCharIndex(rawIndex) != cpPosition(state)) {
        return false;
    }
    _position = rawIndex;
    _cpPosition = cpPosition(state);
    return true;
}

void U32StringReader::reset() noexcept {
    _position = CpIndex::zero();
    _cpPosition = CpIndex::zero();
}

auto U32StringReader::read() noexcept -> Char {
    const auto result = U32StringReadTools{_text.dataView()}.read(_position);
    if (!result.isSignal()) {
        ++_cpPosition;
    }
    return result;
}

auto U32StringReader::readIf(const Char expected) noexcept -> bool {
    auto position = _position;
    const auto result = U32StringReadTools{_text.dataView()}.read(position);
    if (result.isSignal() || result != expected) {
        return false;
    }
    _position = position;
    ++_cpPosition;
    return true;
}

auto U32StringReader::readIf(const CharSet &expected) noexcept -> std::optional<Char> {
    auto position = _position;
    const auto result = U32StringReadTools{_text.dataView()}.read(position);
    if (result.isSignal() || !expected.contains(result)) {
        return {};
    }
    _position = position;
    ++_cpPosition;
    return result;
}

auto U32StringReader::peek() const noexcept -> Char {
    return U32StringReadTools{_text.dataView()}.charAt(_position);
}

auto U32StringReader::isAtEnd() const noexcept -> bool {
    return peek().isEndOfData();
}

auto U32StringReader::canRead(CpLength count) const noexcept -> bool {
    if (count.isZero()) {
        return true;
    }
    if (count.isInfinite()) {
        return false;
    }
    auto position = _position;
    while (!count.isZero()) {
        const auto result = U32StringReadTools{_text.dataView()}.charAt(position);
        if (result.isSignal()) {
            return false;
        }
        _text.advance(position);
        --count;
    }
    return true;
}

auto U32StringReader::advance(CpLength count) noexcept -> bool {
    if (count.isZero()) {
        return false;
    }
    const auto result = U32StringReadTools{_text.dataView()}.advance(_position, count);
    if (!result) {
        return false;
    }
    _cpPosition = _position;
    return true;
}

auto U32StringReader::advanceIf(const Char expected) noexcept -> bool {
    return readIf(expected);
}

auto U32StringReader::advanceIf(const CharSet &expected) noexcept -> bool {
    return readIf(expected).has_value();
}

auto U32StringReader::readWhile(const ReadFn &readFn, const CharSet &expected, CpLength maximum) noexcept
    -> LoopResult {
    return readLoop(readFn, expected, maximum, false).first;
}

auto U32StringReader::readUntil(const ReadFn &readFn, const CharSet &stopSet, CpLength maximum) noexcept -> LoopResult {
    return readLoop(readFn, stopSet, maximum, true).first;
}

auto U32StringReader::advanceWhile(const CharSet &expected, const CpLength maximum) noexcept -> CpLength {
    return readLoop({}, expected, maximum, false).second;
}

auto U32StringReader::advanceUntil(const CharSet &stopSet, const CpLength maximum) noexcept -> CpLength {
    return readLoop({}, stopSet, maximum, true).second;
}

void U32StringReader::startCapture() noexcept {
    _captureStart = _position;
}

auto U32StringReader::takeCapture() noexcept -> AnyString {
    auto result = AnyString{};
    if (_captureStart.isNoIndex() || _captureStart >= _position) {
        _captureStart = _position;
        return result;
    }
    result = _text.slice({_captureStart, _position});
    _captureStart = _position;
    return result;
}

void U32StringReader::clearBuffer() noexcept {
    _buffer.clear();
}

auto U32StringReader::takeBuffer() -> AnyString {
    auto result = AnyStringEditor{std::move(_buffer)};
    _buffer = U32StringEditor{};
    return result;
}

auto U32StringReader::bufferView() const noexcept -> AnyString {
    return AnyString{_buffer};
}

auto U32StringReader::bufferCharacterLength() const noexcept -> CpLength {
    return _buffer.length();
}

auto U32StringReader::isBufferEmpty() const noexcept -> bool {
    return _buffer.isEmpty();
}

void U32StringReader::setBuffer(const AnyString &text) {
    _buffer = U32StringEditor{text.toU32String()};
}

void U32StringReader::appendToBuffer(const Char character) {
    if (!character.isValidUnicode()) {
        return;
    }
    _buffer.append(character);
}

void U32StringReader::appendToBuffer(const AnyString &text) {
    if (text.isEmpty()) {
        return;
    }
    _buffer.append(text.toU32String());
}

void U32StringReader::appendCaptureToBuffer() {
    appendToBuffer(takeCapture());
}

auto U32StringReader::readToBuffer() -> Char {
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

auto U32StringReader::readToBufferIf(const Char expected) -> bool {
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

auto U32StringReader::readToBufferIf(const CharSet &expected) -> std::optional<Char> {
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

auto U32StringReader::readToBufferWhile(const CharSet &expected, const CpLength maximum) -> LoopResult {
    return readToBufferLoop(expected, maximum, false);
}

auto U32StringReader::readToBufferUntil(const CharSet &stopSet, const CpLength maximum) -> LoopResult {
    return readToBufferLoop(stopSet, maximum, true);
}

auto U32StringReader::readLoop(
    const ReadFn &readFn, const CharSet &charSet, CpLength maximum, bool stopOnMatch) noexcept -> ReadLoopOutcome {
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

auto U32StringReader::readToBufferLoop(const CharSet &charSet, CpLength maximum, bool stopOnMatch) -> LoopResult {
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
