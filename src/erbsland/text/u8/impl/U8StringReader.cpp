// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringReader.hpp"

#include "U8StringReadTools.hpp"

#include "../../AnyStringView.hpp"

#include <utility>

namespace erbsland::text::impl {

U8StringReader::U8StringReader(U8StringView text) noexcept : _text{std::move(text)} {
}

auto U8StringReader::clone() const -> U8StringReader * {
    return new U8StringReader{*this};
}

auto U8StringReader::position() const noexcept -> unit::CpIndex {
    return _cpPosition;
}

auto U8StringReader::save() const noexcept -> StringCharReaderState {
    return makeState(StringReaderBackendKind::U8, viewStorageId(_text), _position.toSizeT(), _cpPosition);
}

auto U8StringReader::restore(const StringCharReaderState state) noexcept -> bool {
    const auto raw = rawPosition(state);
    const auto rawIndex = unit::ByteIndex::fromSizeT(raw);
    if (stateKind(state) != StringReaderBackendKind::U8 || storageId(state) != viewStorageId(_text) ||
        raw > _text.length().toSizeT() || _text.toCharIndex(rawIndex) != cpPosition(state)) {
        return false;
    }
    _position = rawIndex;
    _cpPosition = cpPosition(state);
    return true;
}

void U8StringReader::reset() noexcept {
    _position = unit::ByteIndex::zero();
    _cpPosition = unit::CpIndex::zero();
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

auto U8StringReader::readOrThrow() -> Char {
    const auto result = U8StringReadTools{_text.dataView()}.readOrThrow(_position);
    if (!result.isSignal()) {
        ++_cpPosition;
    }
    return result;
}

auto U8StringReader::readIfOrThrow(const Char expected) -> bool {
    auto position = _position;
    const auto result = U8StringReadTools{_text.dataView()}.readOrThrow(position);
    if (result != expected) {
        return false;
    }
    _position = position;
    ++_cpPosition;
    return true;
}

auto U8StringReader::readIfOrThrow(const CharSet &expected) -> std::optional<Char> {
    auto position = _position;
    const auto result = U8StringReadTools{_text.dataView()}.readOrThrow(position);
    if (!expected.contains(result)) {
        return {};
    }
    _position = position;
    ++_cpPosition;
    return result;
}

auto U8StringReader::peekOrThrow() const -> Char {
    return U8StringReadTools{_text.dataView()}.charAtOrThrow(_position);
}

auto U8StringReader::isAtEnd() const noexcept -> bool {
    return _position.distanceFromZero() >= _text.length();
}

auto U8StringReader::canRead(const unit::CpLength count) const noexcept -> bool {
    if (count.isZero()) {
        return true;
    }
    if (count.isInfinite()) {
        return false;
    }
    auto position = _position;
    const auto tools = U8StringReadTools{_text.dataView()};
    for (auto i = unit::CpLength{0}; i < count; ++i) {
        if (tools.read(position).isSignal()) {
            return false;
        }
    }
    return true;
}

auto U8StringReader::advance(const unit::CpLength count) noexcept -> bool {
    if (count.isZero() || isAtEnd()) {
        return false;
    }
    for (auto i = unit::CpLength{0}; i < count; ++i) {
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

auto U8StringReader::advanceIfOrThrow(const Char expected) -> bool {
    return readIfOrThrow(expected);
}

auto U8StringReader::advanceIfOrThrow(const CharSet &expected) -> bool {
    return readIfOrThrow(expected).has_value();
}

auto U8StringReader::readWhile(const ReadFn &readFn, const CharSet &expected, unit::CpLength maximum) noexcept
    -> util::LoopResult {
    return readLoop(readFn, expected, maximum, false);
}

auto U8StringReader::readUntil(const ReadFn &readFn, const CharSet &stopSet, unit::CpLength maximum) noexcept
    -> util::LoopResult {
    return readLoop(readFn, stopSet, maximum, true);
}

void U8StringReader::startCapture() noexcept {
    _captureStart = _position;
}

auto U8StringReader::takeCapture() noexcept -> AnyStringView {
    auto result = AnyStringView{};
    if (_captureStart.isNoIndex() || _captureStart >= _position) {
        _captureStart = _position;
        return result;
    }
    result = _text.slice({_captureStart, _position});
    _captureStart = _position;
    return result;
}

auto U8StringReader::readLoop(
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

}
