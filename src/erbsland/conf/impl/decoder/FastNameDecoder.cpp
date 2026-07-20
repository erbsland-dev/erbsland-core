// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FastNameDecoder.hpp"

#include "../char/NamedChars.hpp"
#include "../constants/Defaults.hpp"

#include "../../../text/EncodingError.hpp"
#include "../../../unit/ByteRange.hpp"

namespace erbsland::conf::impl {

FastNameDecoder::FastNameDecoder(Buffer buffer) : _buffer{std::move(buffer)} {
}

void FastNameDecoder::initialize() {
    _charIndex = {};
    _readIndex = {};
    _position = unit::CodeLocation{unit::LineIndex::zero(), unit::ColumnIndex::zero(), unit::CpIndex::zero()};
    if (_buffer.isEmpty()) {
        _currentChar = text::Char::endOfData();
        return;
    }
    readCurrentCharacter();
}

auto FastNameDecoder::location() const -> Location {
    return Location{SourceIdentifier::create(text::String{defaults::namePathIdentifier}, {}), _position};
}

auto FastNameDecoder::sourceIdentifier() const noexcept -> SourceIdentifierPtr {
    return SourceIdentifier::create(text::String{defaults::namePathIdentifier}, {});
}

void FastNameDecoder::next() {
    if (_currentChar.isEndOfData()) {
        return;
    }
    _position.nextColumn();
    _charIndex = _readIndex;
    if (_readIndex >= unit::ByteIndex::end(_buffer.length())) {
        _currentChar = text::Char::endOfData();
        return;
    }
    readCurrentCharacter();
}

auto FastNameDecoder::decoderState() const noexcept -> DecoderState {
    return DecoderState{_currentChar, _charIndex, _readIndex, _position};
}

void FastNameDecoder::restoreDecoderState(const DecoderState &state) noexcept {
    _currentChar = state.character();
    _charIndex = state.characterIndex();
    _readIndex = state.nextByteIndex();
    _position = state.location();
}

auto FastNameDecoder::captureFromDecoderState(const DecoderState &state) const noexcept -> text::String {
    return _buffer.slice(unit::ByteRange{state.characterIndex(), _charIndex});
}

void FastNameDecoder::readCurrentCharacter() {
    try {
        _currentChar = text::Char{_buffer.readCharAndAdvanceOrThrow(_readIndex)};
    } catch (const text::EncodingError &error) {
        throw ConfError{
            ConfErrorCategory::Encoding,
            "Decoding the Configuration Name Failed"_el,
            error.reason(),
            location(),
            std::current_exception()};
    }
}

}
