// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringReader.hpp"

#include "U32StringReadTools.hpp"

#include "../../AnyString.hpp"
#include "../../AnyStringView.hpp"

#include <utility>

namespace erbsland::text::impl {

U32StringReader::U32StringReader(U32StringView text) noexcept : _text{std::move(text)} {
}

auto U32StringReader::clone() const -> U32StringReader * {
    return new U32StringReader{*this};
}

auto U32StringReader::position() const noexcept -> unit::CpIndex {
    return _cpPosition;
}

auto U32StringReader::save() const noexcept -> StringCharReaderState {
    return makeState(StringReaderBackendKind::U32, viewStorageId(_text), _position.toSizeT(), _cpPosition);
}

auto U32StringReader::restore(const StringCharReaderState state) noexcept -> bool {
    const auto raw = rawPosition(state);
    const auto rawIndex = unit::CpIndex::fromSizeT(raw);
    if (stateKind(state) != StringReaderBackendKind::U32 || storageId(state) != viewStorageId(_text) ||
        raw > _text.length().toSizeT() || _text.toCharIndex(rawIndex) != cpPosition(state)) {
        return false;
    }
    _position = rawIndex;
    _cpPosition = cpPosition(state);
    return true;
}

void U32StringReader::reset() noexcept {
    _position = unit::CpIndex::zero();
    _cpPosition = unit::CpIndex::zero();
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

auto U32StringReader::readOrThrow() -> Char {
    const auto result = U32StringReadTools{_text.dataView()}.readOrThrow(_position);
    ++_cpPosition;
    return result;
}

auto U32StringReader::readIfOrThrow(const Char expected) -> bool {
    auto position = _position;
    const auto result = U32StringReadTools{_text.dataView()}.readOrThrow(position);
    if (result != expected) {
        return false;
    }
    _position = position;
    ++_cpPosition;
    return true;
}

auto U32StringReader::readIfOrThrow(const CharSet &expected) -> std::optional<Char> {
    auto position = _position;
    const auto result = U32StringReadTools{_text.dataView()}.readOrThrow(position);
    if (!expected.contains(result)) {
        return {};
    }
    _position = position;
    ++_cpPosition;
    return result;
}

auto U32StringReader::peekOrThrow() const -> Char {
    return U32StringReadTools{_text.dataView()}.charAtOrThrow(_position);
}

auto U32StringReader::isAtEnd() const noexcept -> bool {
    return peek().isEndOfData();
}

auto U32StringReader::canRead(unit::CpLength count) const noexcept -> bool {
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

auto U32StringReader::advance(unit::CpLength count) noexcept -> bool {
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

auto U32StringReader::advanceIfOrThrow(const Char expected) -> bool {
    return readIfOrThrow(expected);
}

auto U32StringReader::advanceIfOrThrow(const CharSet &expected) -> bool {
    return readIfOrThrow(expected).has_value();
}

auto U32StringReader::readWhile(const ReadFn &readFn, const CharSet &expected, unit::CpLength maximum) noexcept
    -> util::LoopResult {
    return readLoop(readFn, expected, maximum, false);
}

auto U32StringReader::readUntil(const ReadFn &readFn, const CharSet &stopSet, unit::CpLength maximum) noexcept
    -> util::LoopResult {
    return readLoop(readFn, stopSet, maximum, true);
}

void U32StringReader::startCapture() noexcept {
    _captureStart = _position;
}

auto U32StringReader::takeCapture() noexcept -> AnyStringView {
    auto result = AnyStringView{};
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
    auto result = AnyString{std::move(_buffer)};
    _buffer = U32String{};
    return result;
}

auto U32StringReader::bufferView() const noexcept -> AnyStringView {
    return AnyStringView{_buffer};
}

auto U32StringReader::bufferCharacterLength() const noexcept -> unit::CpLength {
    return _buffer.length();
}

auto U32StringReader::isBufferEmpty() const noexcept -> bool {
    return _buffer.isEmpty();
}

void U32StringReader::setBuffer(const AnyStringView &text) {
    _buffer = text.toU32String();
}

void U32StringReader::appendToBuffer(const Char character) {
    if (!character.isValidUnicode()) {
        return;
    }
    _buffer.append(character);
}

void U32StringReader::appendToBuffer(const AnyStringView &text) {
    if (text.isEmpty()) {
        return;
    }
    _buffer.append(text.toU32StringView());
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
    static_cast<void>(advance(unit::CpLength::one()));
    return character;
}

auto U32StringReader::readToBufferIf(const Char expected) -> bool {
    const auto character = peek();
    if (character.isSignal() || character != expected) {
        return false;
    }
    appendToBuffer(character);
    static_cast<void>(advance(unit::CpLength::one()));
    return true;
}

auto U32StringReader::readToBufferIf(const CharSet &expected) -> std::optional<Char> {
    const auto character = peek();
    if (character.isSignal() || !expected.contains(character)) {
        return {};
    }
    appendToBuffer(character);
    static_cast<void>(advance(unit::CpLength::one()));
    return character;
}

auto U32StringReader::readToBufferWhile(const CharSet &expected, const unit::CpLength maximum) -> util::LoopResult {
    return readToBufferLoop(expected, maximum, false);
}

auto U32StringReader::readToBufferUntil(const CharSet &stopSet, const unit::CpLength maximum) -> util::LoopResult {
    return readToBufferLoop(stopSet, maximum, true);
}

auto U32StringReader::readLoop(
    const ReadFn &readFn, const CharSet &charSet, unit::CpLength maximum, bool stopOnMatch) noexcept
    -> util::LoopResult {
    auto count = unit::CpLength::zero();
    while (true) {
        const auto lastPosition = _position;
        const auto lastCpPosition = _cpPosition;
        const auto character = read();
        if (character.isEndOfData()) {
            return util::LoopResult::EndOfData;
        }
        if (charSet.contains(character) == stopOnMatch) {
            _position = lastPosition;
            _cpPosition = lastCpPosition;
            return util::LoopResult::Success;
        }
        if (!maximum.isInfinite() && count >= maximum) {
            _position = lastPosition;
            _cpPosition = lastCpPosition;
            return util::LoopResult::LimitReached;
        }
        const auto result = readFn != nullptr ? readFn(character) : util::LoopStatus::Continue;
        if (result != util::LoopStatus::Continue) {
            _position = lastPosition;
            _cpPosition = lastCpPosition;
            return result == util::LoopStatus::Error ? util::LoopResult::Error : util::LoopResult::Stopped;
        }
        ++count;
    }
}

auto U32StringReader::readToBufferLoop(const CharSet &charSet, unit::CpLength maximum, bool stopOnMatch)
    -> util::LoopResult {
    auto count = unit::CpLength::zero();
    while (true) {
        const auto character = peek();
        if (character.isEndOfData()) {
            return util::LoopResult::EndOfData;
        }
        if (charSet.contains(character) == stopOnMatch) {
            return util::LoopResult::Success;
        }
        if (!maximum.isInfinite() && count >= maximum) {
            return util::LoopResult::LimitReached;
        }
        appendToBuffer(character);
        static_cast<void>(advance(unit::CpLength::one()));
        ++count;
    }
}

}
