// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Tokenizer.hpp"

#include "../RenderError.hpp"

#include "../../../unit/ColumnIndex.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../../unit/LineIndex.hpp"
#include "../../AnyString.hpp"
#include "../../IntegerBase.hpp"
#include "../../Literals.hpp"
#include "../../StringSide.hpp"
#include "../../u8/U8StringConstIterator.hpp"

namespace erbsland::text::render::impl {

using namespace unit;
using namespace text::literals;

Tokenizer::Tokenizer(String layout, String origin, String source, EnvironmentOptions options) :
    _layout{std::move(layout)},
    _origin{std::move(origin)},
    _source{std::move(source)},
    _options{std::move(options)},
    _expressionOpeningCharacter{_options.expressionDelimiters().begin().charAt(ByteIndex::zero())},
    _statementOpeningCharacter{_options.statementDelimiters().begin().charAt(ByteIndex::zero())},
    _commentOpeningCharacter{_options.commentDelimiters().begin().charAt(ByteIndex::zero())},
    _openingCharacters{_expressionOpeningCharacter, _statementOpeningCharacter, _commentOpeningCharacter},
    _reader{_source},
    _location{LineIndex::zero(), ColumnIndex::zero(), CpIndex::zero()} {
    for (
        const auto *delimiters :
        {&_options.expressionDelimiters(), &_options.statementDelimiters(), &_options.commentDelimiters()}) {
        if (!delimiters->line().isEmpty()) {
            _openingCharacters.add(delimiters->line().charAt(ByteIndex::zero()));
        }
    }
}

void Tokenizer::advance() {
    if (_pendingToken.has_value()) {
        _token = std::move(*_pendingToken);
        _pendingToken.reset();
        return;
    }
    _token = _mode == Mode::Text ? readTextToken() : readTagToken();
}

auto Tokenizer::readTextToken() -> Token {
    while (true) {
        const auto textLocation = _location;
        _reader.startCapture();
        while (!_reader.isAtEnd()) {
            [[maybe_unused]] const auto rawScanResult = _reader.readUntil(
                [this](const Char character) noexcept -> util::LoopStatus {
                    advanceLocation(character);
                    return util::LoopStatus::Continue;
                },
                _openingCharacters);
            if (_reader.isAtEnd()) {
                break;
            }
            const auto opening = openingAtCurrent();
            if (!opening.has_value()) {
                read();
                continue;
            }

            auto text = _reader.takeCapture().toString();
            const auto tagLocation = _location;
            _isLineTag = opening->isLine;
            if (_isLineTag) {
                text = text.trimmed(CharSet{" \t"_el}, StringSide::Back);
            }
            consume(_isLineTag ? opening->delimiters->line() : opening->delimiters->begin());
            const auto leftTrim = !_isLineTag && _reader.peek() == U'-';
            if (leftTrim) {
                read();
                text = text.trimmed(std::nullopt, StringSide::Back);
            }

            _activeDelimiters = opening->delimiters;
            _activeClosingCharacter = _activeDelimiters->end().charAt(ByteIndex::zero());
            _tagLocation = tagLocation;
            if (opening->isComment) {
                skipComment(tagLocation);
                _reader.startCapture();
                if (!text.isEmpty()) {
                    return Token{TokenKind::Text, std::move(text), textLocation};
                }
                break;
            }

            _mode = opening->mode;
            auto openingToken = Token{opening->tokenKind, {}, tagLocation};
            if (!text.isEmpty()) {
                _pendingToken = std::move(openingToken);
                return Token{TokenKind::Text, std::move(text), textLocation};
            }
            return openingToken;
        }

        if (_reader.isAtEnd()) {
            auto text = _reader.takeCapture().toString();
            if (!text.isEmpty()) {
                return Token{TokenKind::Text, std::move(text), textLocation};
            }
            return Token{TokenKind::End, {}, _location};
        }
    }
}

auto Tokenizer::readTagToken() -> Token {
    while (
        !_reader.isAtEnd() && _reader.peek().isAsciiWhitespace() && !isAtClosingDelimiter() &&
        !isAtRightTrimClosingDelimiter()) {
        read();
    }
    const auto location = _location;
    if (_reader.isAtEnd() && !_isLineTag) {
        throwSyntax("Invalid layout syntax"_el, "The layout tag has no closing delimiter."_el, _tagLocation);
    }
    if (isAtRightTrimClosingDelimiter()) {
        read();
        consumeClosingDelimiter();
        skipRawWhitespace();
        _activeDelimiters = nullptr;
        _mode = Mode::Text;
        return Token{TokenKind::TagEnd, {}, location};
    }
    if (isAtClosingDelimiter()) {
        consumeClosingDelimiter();
        _activeDelimiters = nullptr;
        _mode = Mode::Text;
        return Token{TokenKind::TagEnd, {}, location};
    }

    const auto character = _reader.peek();
    if (character.isAsciiLetter() || character == U'_') {
        return readIdentifier(location);
    }
    if (character.isAsciiDigit() || (character == U'.' && nextIsAsciiDigit())) {
        return readNumber(location);
    }
    if (character == U'\'' || character == U'"') {
        read();
        return readString(character, location);
    }

    read();
    switch (character.toRawValue()) {
    case U'(':
        return Token{TokenKind::LeftParen, {}, location};
    case U')':
        return Token{TokenKind::RightParen, {}, location};
    case U'[':
        return Token{TokenKind::LeftBracket, {}, location};
    case U']':
        return Token{TokenKind::RightBracket, {}, location};
    case U'{':
        return Token{TokenKind::LeftBrace, {}, location};
    case U'}':
        return Token{TokenKind::RightBrace, {}, location};
    case U'.':
        return Token{TokenKind::Dot, {}, location};
    case U',':
        return Token{TokenKind::Comma, {}, location};
    case U':':
        return Token{TokenKind::Colon, {}, location};
    case U'|':
        return Token{TokenKind::Pipe, {}, location};
    case U'+':
        return Token{TokenKind::Plus, {}, location};
    case U'-':
        return Token{TokenKind::Minus, {}, location};
    case U'*':
        return Token{TokenKind::Multiply, {}, location};
    case U'/':
        return Token{TokenKind::Divide, {}, location};
    case U'~':
        return Token{TokenKind::Concatenate, {}, location};
    case U'=':
        if (readIf(U'=')) {
            return Token{TokenKind::Equal, {}, location};
        }
        return Token{TokenKind::Assign, {}, location};
    case U'!':
        if (readIf(U'=')) {
            return Token{TokenKind::NotEqual, {}, location};
        }
        break;
    case U'>':
        if (readIf(U'=')) {
            return Token{TokenKind::GreaterEqual, {}, location};
        }
        return Token{TokenKind::Greater, {}, location};
    case U'<':
        if (readIf(U'=')) {
            return Token{TokenKind::LessEqual, {}, location};
        }
        return Token{TokenKind::Less, {}, location};
    default:
        break;
    }
    return Token{TokenKind::Unsupported, {}, location};
}

void Tokenizer::skipComment(const CodeLocation tagLocation) {
    while (!_reader.isAtEnd() || _isLineTag) {
        if (isAtRightTrimClosingDelimiter()) {
            read();
            consumeClosingDelimiter();
            skipRawWhitespace();
            _activeDelimiters = nullptr;
            return;
        }
        if (isAtClosingDelimiter()) {
            consumeClosingDelimiter();
            _activeDelimiters = nullptr;
            return;
        }
        read();
    }
    throwSyntax("Invalid layout syntax"_el, "The layout tag has no closing delimiter."_el, tagLocation);
}

auto Tokenizer::readIdentifier(const CodeLocation location) -> Token {
    _reader.startCapture();
    readWhile(AsciiCategory::Word);
    auto text = _reader.takeCapture().toString();
    auto kind = TokenKind::Identifier;
    switch (text.length().toRawValue()) {
    case 2U:
        if (text == "or"_el) {
            kind = TokenKind::Or;
        } else if (text == "in"_el) {
            kind = TokenKind::In;
        } else if (text == "is"_el) {
            kind = TokenKind::Is;
        }
        break;
    case 3U:
        if (text == "and"_el) {
            kind = TokenKind::And;
        } else if (text == "not"_el) {
            kind = TokenKind::Not;
        }
        break;
    case 4U:
        if (text == "true"_el) {
            kind = TokenKind::True;
        } else if (text == "none"_el || text == "null"_el) {
            kind = TokenKind::Null;
        }
        break;
    case 5U:
        if (text == "false"_el) {
            kind = TokenKind::False;
        }
        break;
    default:
        break;
    }
    return Token{kind, std::move(text), location};
}

auto Tokenizer::openingAtCurrent() -> std::optional<Opening> {
    const auto match = [this](
                           const Mode mode,
                           const TokenKind tokenKind,
                           const Delimiters &delimiters,
                           const Char openingCharacter,
                           const bool isComment = false) -> std::optional<Opening> {
        if (_reader.peek() == openingCharacter && matches(delimiters.begin())) {
            return Opening{mode, tokenKind, &delimiters, false, isComment};
        }
        if (_lineOnlyIndent && !delimiters.line().isEmpty() && matches(delimiters.line())) {
            return Opening{mode, tokenKind, &delimiters, true, isComment};
        }
        return std::nullopt;
    };
    if (const auto result = match(
            Mode::Expression, TokenKind::ExpressionBegin, _options.expressionDelimiters(), _expressionOpeningCharacter);
        result.has_value()) {
        return result;
    }
    if (const auto result = match(
            Mode::Statement, TokenKind::StatementBegin, _options.statementDelimiters(), _statementOpeningCharacter);
        result.has_value()) {
        return result;
    }
    return match(Mode::Text, TokenKind::End, _options.commentDelimiters(), _commentOpeningCharacter, true);
}

auto Tokenizer::isAtClosingDelimiter() -> bool {
    if (_isLineTag) {
        return _reader.isAtEnd() || _reader.peek() == U'\n' || _reader.peek() == U'\r';
    }
    return _activeDelimiters != nullptr && _reader.peek() == _activeClosingCharacter &&
        matches(_activeDelimiters->end());
}

auto Tokenizer::isAtRightTrimClosingDelimiter() -> bool {
    if (_isLineTag || _activeDelimiters == nullptr || _reader.peek() != U'-') {
        return false;
    }
    const auto state = _reader.save();
    _reader.advance();
    const auto result = _reader.peek() == _activeClosingCharacter && matches(_activeDelimiters->end());
    _reader.restore(state);
    return result;
}

auto Tokenizer::matches(const String &expected) -> bool {
    const auto state = _reader.save();
    const auto result = _reader.advanceIf(expected);
    _reader.restore(state);
    return result;
}

void Tokenizer::consume(const String &expected) {
    if (!_reader.advanceIf(expected)) {
        throwSyntax("Invalid layout syntax"_el, "The layout delimiter changed while it was read."_el, _location);
    }
    advanceLocation(expected);
}

void Tokenizer::consumeClosingDelimiter() {
    if (_isLineTag) {
        if (_reader.peek() == U'\r') {
            read();
        }
        if (_reader.peek() == U'\n') {
            read();
        }
        _isLineTag = false;
    } else {
        consume(_activeDelimiters->end());
    }
}

auto Tokenizer::read() -> Char {
    const auto character = _reader.read();
    advanceLocation(character);
    return character;
}

auto Tokenizer::readIf(const Char expected) -> bool {
    if (isAtClosingDelimiter() || isAtRightTrimClosingDelimiter() || _reader.peek() != expected) {
        return false;
    }
    read();
    return true;
}

auto Tokenizer::readWhile(const AsciiCategory category) -> CpCount {
    auto count = CpCount::zero();
    while (!isAtClosingDelimiter() && !isAtRightTrimClosingDelimiter() && _reader.peek().isAsciiCategory(category)) {
        read();
        ++count;
    }
    return count;
}

auto Tokenizer::nextIsAsciiDigit() -> bool {
    const auto state = _reader.save();
    const auto advanced = _reader.advance();
    const auto result = advanced && (_activeDelimiters == nullptr || !matches(_activeDelimiters->end())) &&
        _reader.peek().isAsciiDigit();
    _reader.restore(state);
    return result;
}

void Tokenizer::advanceLocation(const Char character) noexcept {
    if (character == U'\n') {
        _location.nextLine();
        _lineOnlyIndent = true;
    } else if (!character.isSignal()) {
        _location.nextColumn();
        if (character != U' ' && character != U'\t' && character != U'\r') {
            _lineOnlyIndent = false;
        }
    }
}

void Tokenizer::advanceLocation(const String &text) {
    for (const auto character : text) {
        advanceLocation(character);
    }
}

void Tokenizer::skipRawWhitespace() {
    while (_reader.peek().isAsciiWhitespace()) {
        read();
    }
}

void Tokenizer::throwSyntax(String title, String description, const CodeLocation location) const {
    throw RenderError{RenderErrorContext{RenderErrorCategory::Syntax, std::move(title), std::move(description)}
            .setLayout(_layout)
            .setOrigin(_origin)
            .setLocation(location)};
}

}
