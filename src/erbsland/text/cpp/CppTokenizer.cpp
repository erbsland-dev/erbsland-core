// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CppTokenizer.hpp"

#include "../AnyString.hpp"
#include "../StringEditor.hpp"

#include "../../err/ParseError.hpp"

namespace erbsland::text::cpp {

using namespace unit;
using namespace literals;

CppTokenizer::CppTokenizer(const String &source) : _reader{source} {
}

auto CppTokenizer::tokens(String source) -> util::CoGenerator<CppToken> {
    auto tokenizer = CppTokenizer{source};
    while (true) {
        auto token = tokenizer.next();
        if (token.kind() == CppToken::Kind::End) {
            co_return;
        }
        co_yield token;
    }
}

auto CppTokenizer::capture(const CppToken::Kind kind, const ByteIndex position, const CpIndex characterPosition)
    -> CppToken {
    const auto range = ByteRange{position, _reader.u8Position()};
    const auto characterRange = CpRange{characterPosition, _reader.position()};
    return {kind, _reader.takeCapture().toString(), range, characterRange};
}

auto CppTokenizer::next() -> CppToken {
    auto first = _reader.peek();
    while (first.isAsciiWhitespace()) {
        if (first == U'\n') {
            _lineStart = true;
        }
        _reader.advance();
        first = _reader.peek();
    }
    const auto position = _reader.u8Position();
    const auto characterPosition = _reader.position();
    if (first.isEndOfData()) {
        return {CppToken::Kind::End, {}, ByteRange::emptyAt(position), CpRange::emptyAt(characterPosition)};
    }
    _reader.startCapture();
    return capture(readToken(first), position, characterPosition);
}

auto CppTokenizer::readToken(const Char first) -> CppToken::Kind {
    if (first == U'/' && readComment()) {
        return CppToken::Kind::Comment;
    }
    if (_lineStart && first == U'#') {
        _reader.advance();
        readLine();
        return CppToken::Kind::Directive;
    }
    _lineStart = false;
    if (readLiteral(first)) {
        return CppToken::Kind::Literal;
    }
    if (first.isAsciiDigit()) {
        readNumber(first);
        return CppToken::Kind::Number;
    }
    if (first.isAsciiWord() || !first.isAscii()) {
        readIdentifier();
        return CppToken::Kind::Identifier;
    }
    readPunctuation(first);
    return CppToken::Kind::Punctuation;
}

auto CppTokenizer::readComment() -> bool {
    const auto start = _reader.save();
    _reader.advance();
    const auto second = _reader.peek();
    if (second == U'/') {
        _reader.advance();
        readLine();
        _lineStart = true;
        return true;
    }
    if (second == U'*') {
        _reader.advance();
        readBlockComment();
        return true;
    }
    _reader.restore(start);
    return false;
}

void CppTokenizer::readBlockComment() {
    while (true) {
        const auto character = _reader.read();
        if (character.isEndOfData()) {
            fail("Unterminated block comment."_el);
        }
        if (character == U'\n') {
            _lineStart = true;
        } else if (character == U'*' && _reader.advanceIf(U'/')) {
            return;
        }
    }
}

auto CppTokenizer::readLiteral(const Char first) -> bool {
    if (first == U'"' || first == U'\'') {
        _reader.advance();
        readQuoted(first);
        return true;
    }
    if (first != U'R' && first != U'u' && first != U'U' && first != U'L') {
        return false;
    }
    const auto start = _reader.save();
    _reader.advance();
    auto next = _reader.peek();
    if (first == U'u' && next == U'8') {
        _reader.advance();
        next = _reader.peek();
    }
    auto raw = first == U'R';
    if (!raw && next == U'R') {
        raw = true;
        _reader.advance();
        next = _reader.peek();
    }
    if (next == U'"' || (!raw && next == U'\'')) {
        _reader.advance();
        if (raw) {
            readRaw();
        } else {
            readQuoted(next);
        }
        return true;
    }
    _reader.restore(start);
    return false;
}

void CppTokenizer::readNumber(Char character) {
    auto previous = Char{};
    while (
        character.isAsciiWord() || character == U'.' || character == U'\'' ||
        ((character == U'+' || character == U'-') &&
            (previous == U'e' || previous == U'E' || previous == U'p' || previous == U'P'))) {
        _reader.advance();
        previous = character;
        character = _reader.peek();
    }
}

void CppTokenizer::readIdentifier() {
    while (true) {
        _reader.advance();
        const auto character = _reader.peek();
        if (character.isEndOfData() || (!character.isAsciiWord() && character.isAscii())) {
            return;
        }
    }
}

void CppTokenizer::readPunctuation(const Char first) {
    _reader.advance();
    switch (first.toRawValue()) {
    case U'+':
    case U'-':
    case U'&':
    case U'|': {
        const auto second = _reader.peek();
        if (second == first || second == U'=') {
            _reader.advance();
        } else if (first == U'-' && second == U'>') {
            _reader.advance();
            _reader.advanceIf(U'*');
        }
        break;
    }
    case U'<':
    case U'>': {
        const auto second = _reader.peek();
        if (second == first) {
            _reader.advance();
            _reader.advanceIf(U'=');
        } else if (second == U'=') {
            _reader.advance();
            if (first == U'<') {
                _reader.advanceIf(U'>');
            }
        }
        break;
    }
    case U':':
    case U'#':
        _reader.advanceIf(first);
        break;
    case U'=':
    case U'!':
    case U'*':
    case U'/':
    case U'%':
    case U'^':
        _reader.advanceIf(U'=');
        break;
    case U'.': {
        const auto second = _reader.peek();
        if (second == U'*') {
            _reader.advance();
        } else if (second == U'.') {
            const auto afterFirst = _reader.save();
            _reader.advance();
            if (!_reader.advanceIf(U'.')) {
                _reader.restore(afterFirst);
            }
        }
        break;
    }
    default:
        break;
    }
}

void CppTokenizer::readLine() {
    while (true) {
        const auto character = _reader.read();
        if (character.isEndOfData() || character == U'\n') {
            return;
        }
        if (character == U'\\') {
            _reader.advanceIf(U'\r');
            _reader.advanceIf(U'\n');
        }
    }
}

void CppTokenizer::readQuoted(const Char quote) {
    while (!_reader.isAtEnd()) {
        const auto character = _reader.read();
        if (character == quote) {
            return;
        }
        if (character == U'\\') {
            if (_reader.advanceIf(U'\r')) {
                _reader.advanceIf(U'\n');
            } else {
                _reader.advance();
            }
        } else if (character == U'\n' || character == U'\r') {
            break;
        }
    }
    fail("Unterminated quoted literal."_el);
}

void CppTokenizer::readRaw() {
    auto delimiter = StringEditor{")"_el};
    auto length = std::size_t{};
    while (true) {
        const auto character = _reader.peek();
        if (character == U'(') {
            _reader.advance();
            break;
        }
        if (character.isEndOfData() || ++length > 16 || character.isAsciiWhitespace() || character == U')' ||
            character == U'\\') {
            fail("Invalid raw string delimiter."_el);
        }
        delimiter.append(character);
        _reader.advance();
    }
    delimiter.append(U'"');
    const auto closing = String{delimiter};
    while (true) {
        const auto character = _reader.peek();
        if (character.isEndOfData()) {
            fail("Unterminated raw string literal."_el);
        }
        if (character == U')' && _reader.advanceIf(closing)) {
            return;
        }
        _reader.advance();
    }
}

void CppTokenizer::fail(const String &message) const {
    throw err::ParseError{message, _reader.position()};
}

}
