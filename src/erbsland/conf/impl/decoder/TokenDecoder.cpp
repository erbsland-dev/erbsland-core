// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TokenDecoder.hpp"

#include "../char/NamedChars.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

auto TokenDecoder::create(CharStreamPtr decoder) noexcept -> TokenDecoderPtr {
    return std::make_shared<TokenDecoder>(std::move(decoder));
}

TokenDecoder::TokenDecoder(CharStreamPtr decoder) noexcept : _decoder{std::move(decoder)} {
    assert(_decoder != nullptr);
}

void TokenDecoder::initialize() {
    nextToken();
}

auto TokenDecoder::location() const -> Location {
    return Location{_decoder->source()->identifier(), characterPosition()};
}

auto TokenDecoder::sourceIdentifier() const noexcept -> SourceIdentifierPtr {
    return _decoder->source()->identifier();
}

auto TokenDecoder::digest() const noexcept -> mem::ByteBlock {
    return _decoder->digest();
}

void TokenDecoder::nextToken() {
    next();
    resetTokenStartPosition();
}

auto TokenDecoder::tokenSize() const noexcept -> int {
    assert(characterPosition().line() == _tokenStartPosition.line());
    return static_cast<int>(characterPosition().column().toSizeT() - _tokenStartPosition.column().toSizeT());
}

void TokenDecoder::expectMoreInLine(text::String message) const {
    if (_currentCharacter.character() == CharClass::LineBreak) {
        throwSyntaxError(std::move(message));
    }
    if (_currentCharacter.character().isEndOfData()) {
        throwUnexpectedEndOfDataError(std::move(message));
    }
}

auto TokenDecoder::createEndOfLineToken() -> LexerToken {
    auto token = LexerToken{
        TokenType::LineBreak, tokenStartPosition(), characterPosition(), _decoder->captureToEndOfLine(), NoContent{}};
    nextToken();
    return token;
}

auto TokenDecoder::createEndOfDataToken() -> LexerToken {
    return LexerToken{TokenType::EndOfData, {}, {}, {}, NoContent{}};
}

void TokenDecoder::next() {
    if (_currentCharacter.character().isError()) {
        throw err::LogicError("TokenDecoder: An error was not correctly handled.");
    }
    try {
        // Transactions store a checkpoint into this line instead of copying characters. Therefore they cannot cross
        // a line boundary; doing so would invalidate the COW slice used for captured text and the source cannot rewind.
        if (hasActiveTransaction() && _currentCharacter.character() == CharClass::LineBreakOrEnd) {
            throwInternalError("There is an open transaction at the end of the line."_el);
        }
        _currentCharacter = _decoder->next();
    } catch (const ConfError &error) {
        if (error.category() == ConfErrorCategory::Encoding || error.category() == ConfErrorCategory::Character) {
            // Delay encoding and (control-)character errors by setting the current character to the error mark.
            _hasUpcomingError = true;
            _currentCharacter = DecodedChar{
                text::Char::error(),
                _decoder->lastCharacterStartIndex(),
                error.context().location().value_or(unit::CodeLocation{})};
            _currentError = error.context();
        } else {
            // Throw all other errors (IO, Internal) immediately.
            throw;
        }
    }
}

void TokenDecoder::checkForErrorAndThrowIt() const {
    if (_hasUpcomingError) {
        throw ConfError{_currentError};
    }
}

auto TokenDecoder::decoderState() const noexcept -> DecoderState {
    return DecoderState{
        _currentCharacter.character(),
        _currentCharacter.index(),
        _decoder->nextReadIndex(),
        _currentCharacter.codeLocation()};
}

void TokenDecoder::restoreDecoderState(const DecoderState &state) noexcept {
    _currentCharacter = DecodedChar{state.character(), state.characterIndex(), state.location()};
    _decoder->restore(state.nextByteIndex(), state.characterIndex(), state.location());
    // Keep an error encountered while speculatively reading scheduled. The old replay buffer did the same: syntax
    // checks made while replaying earlier characters still propagated the more precise encoding or character error.
}

auto TokenDecoder::captureFromDecoderState(const DecoderState &state) const noexcept -> text::String {
    assert(state.location().line() == _currentCharacter.codeLocation().line());
    return _decoder->captureRange(state.characterIndex(), _currentCharacter.index());
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const TokenDecoder &object) -> InternalViewPtr {
    auto result = InternalView::create();
    result->setValue("decoder", *object._decoder);
    result->setValue("currentCharacter", object._currentCharacter);
    result->setValue("tokenStartPosition", object._tokenStartPosition.toString());
    result->setValue("currentIndentationPattern", object._currentIndentationPattern);
    auto currentError = InternalView::create();
    currentError->setValue("category", object._currentError.category().toText());
    currentError->setValue("message", object._currentError.description());
    currentError->setValue("location", object._currentError.location().value_or(unit::CodeLocation{}).toString());
    result->setValue("currentError", currentError);
    return result;
}
#endif

}
