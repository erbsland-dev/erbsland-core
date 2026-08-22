// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharStream.hpp"

#include "CharClass.hpp"

#include "../../../mem/ByteSpan.hpp"
#include "../../../text/EncodingError.hpp"
#include "../../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/u8/impl/U8Encoding.hpp"
#include "../../../unit/LineCount.hpp"

#include <span>
#include <utility>

namespace erbsland::conf::impl {

using namespace text::literals;

auto CharStream::create(SourcePtr source) noexcept -> CharStreamPtr {
    assert(source != nullptr);
    return std::make_shared<CharStream>(std::move(source));
}

CharStream::CharStream(SourcePtr source) noexcept : _source{std::move(source)} {
    assert(_source != nullptr);
}

auto CharStream::captureTo(const unit::ByteIndex endPosition) -> text::String {
    if (endPosition > _lineCharacterStartIndex) {
        throwInternalError("Invalid capture position. End after actual read position."_el);
    }
    if (endPosition <= _captureStartIndex) {
        throwInternalError("Invalid capture position. End before start index."_el);
    }
    const auto startPosition = std::exchange(_captureStartIndex, endPosition);
    return _line.slice(unit::ByteRange{startPosition, endPosition});
}

auto CharStream::captureToEndOfLine() noexcept -> text::String {
    const auto startPosition = std::exchange(_captureStartIndex, _lineEndIndex);
    return _line.slice(unit::ByteRange{startPosition, _lineEndIndex});
}

auto CharStream::captureRange(const unit::ByteIndex startPosition, const unit::ByteIndex endPosition) const noexcept
    -> text::String {
    assert(startPosition <= endPosition);
    assert(endPosition <= _lineEndIndex);
    return _line.slice(unit::ByteRange{startPosition, endPosition});
}

void CharStream::restore(
    const unit::ByteIndex nextReadIndex,
    const unit::ByteIndex characterStartIndex,
    const unit::CodeLocation position) noexcept {
    assert(nextReadIndex <= _lineEndIndex);
    assert(characterStartIndex <= nextReadIndex);
    _lineReadIndex = nextReadIndex;
    _lineCharacterStartIndex = characterStartIndex;
    _position = position;
    _endOfData = false;
}

auto CharStream::next() -> DecodedChar {
    if (_source == nullptr) {
        throw ConfError{ConfErrorCategory::Internal, "The source is null."_el};
    }
    if (_endOfData) { // once we reached the end, prevent polling the source further.
        return createEndOfData();
    }
    if (isAtEndOfLine()) { // If we reached the end, try to get more data.
        if (_source->atEnd()) {
            _lineCharacterStartIndex = _lineEndIndex;
            return createEndOfData();
        }
        if (!readNextLine()) {
            return createEndOfData();
        }
        _position.nextLine();
    } else {
        _position.nextColumn();
    }
    auto result = decodeNext();
    // filter invalid control sequences in the configuration language.
    if (result.character() != CharClass::ValidLang) {
        throwCharacterError("Invalid control character."_el);
    }
    // Pass-through everything else.
    return result;
}

auto CharStream::readNextLine() -> bool {
    // Keep the COW line returned by the source and let Core perform all character decoding.
    auto line = _source->readLine();
    if (line.isEmpty()) {
        return false;
    }
    _line = std::move(line);
    _lineBytes = text::impl::UnsafeU8StringAccess{_line}.dataSpan();
    _lineEndIndex = unit::ByteIndex::fromSizeT(_lineBytes.size());
    _lineReadIndex = {};
    // Important: As the char stream is not only used to verify, but also to create document signatures,
    // `_hashEnabled` can be set manually. In these cases, when re-signing a document that already has a
    // `\@signature` line - the first line must be skipped when building the hash.
    if (_position.line().isNoIndex() && isSignatureLine()) {
        // 1. Enable hashing if this was the first line, and we found a `@signature` value.
        // (line counter starts at zero, as it is increased *after* reading the line.)
        // 2. Also, skipping this line for hash-calculation.
        _hashEnabled = true;
    } else if (_hashEnabled && !_line.isEmpty()) {
        _hash.update(mem::toConstByteSpan(_lineBytes));
    }
    _lineCharacterStartIndex = {};
    _captureStartLine =
        _position.line().isNoIndex() ? unit::LineIndex::zero() : _position.line().advanced(unit::LineCount::one());
    _captureStartIndex = {}; // Reset the capture start.
    return true;
}

auto CharStream::decodeNext() -> DecodedChar {
    _lineCharacterStartIndex = _lineReadIndex;
    try {
        const auto character = text::impl::utf8::decodeCharOrThrow(_lineBytes, _lineReadIndex);
        return DecodedChar{character, _lineCharacterStartIndex, _position};
    } catch (const text::EncodingError &error) {
        throw ConfError{
            ConfErrorCategory::Encoding,
            "Decoding the Configuration Document Failed"_el,
            error.reason(),
            _source,
            Location{_source->identifier(), _position},
            std::current_exception()};
    }
}

auto CharStream::createEndOfData() -> DecodedChar {
    if (!_endOfData) {
        // Fix the file position when returning the end of data mark for the first time.
        if (_position.isUndefined()) {
            _position = unit::CodeLocation{unit::LineIndex::zero(), unit::ColumnIndex::zero(), unit::CpIndex::zero()};
        } else {
            _position.nextColumn();
        }
        _endOfData = true;
        if (_hashEnabled && !_hashFinalized) {
            _digest = _hash.finalize();
            _hashFinalized = true;
        }
    }
    return DecodedChar{text::Char::endOfData(), _lineEndIndex, _position};
}

void CharStream::throwCharacterError(text::String message) const {
    throw ConfError{
        ConfErrorCategory::Character, std::move(message), _source, Location{_source->identifier(), _position}};
}

void CharStream::throwInternalError(text::String message) const {
    throw ConfError{
        ConfErrorCategory::Internal, std::move(message), _source, Location{_source->identifier(), _position}};
}

auto CharStream::isSignatureLine() const noexcept -> bool {
    // Detect the signature at the decoded-character level, but intentionally retain ASCII-only case folding.
    // If a document has no signature, it makes no sense to calculate a hash for it.
    // The timing of handling this in the parser is not optimal: While the initial implementation would allow it,
    // it is possible that the lexer could read-ahead in a later version of the parser.
    return _line.startsWith("@signature"_el, text::Char::compareAsciiFolded);
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const CharStream &object) -> InternalViewPtr {
    auto result = InternalView::create();
    result->setValue("source", *object._source);
    result->setValue("endOfData", object._endOfData);
    result->setValue("line", object._line);
    result->setValue("lineLength", object._lineEndIndex.toSizeT());
    result->setValue("lineCurrentIndex", object._lineReadIndex.toSizeT());
    result->setValue("lineCharacterStartIndex", object._lineCharacterStartIndex.toSizeT());
    result->setValue("captureStartLine", object._captureStartLine.toSizeT());
    result->setValue("captureStartIndex", object._captureStartIndex.toSizeT());
    result->setValue("location", object._position.toString());
    return result;
}
#endif

}
