// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HtmlTokenizer.hpp"

#include "../../AnyStringEditor.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace erbsland::text::html::impl {

HtmlTokenizer::HtmlTokenizer(AnyString html) : _reader{std::move(html)} {
}

auto HtmlTokenizer::tokenize() -> TokenGenerator {
    while (!_reader.isAtEnd()) {
        _currentToken = {};
        if (_reader.peek() == U'<') {
            const auto state = _reader.save();
            if (tokenizeTag()) {
                co_yield std::move(_currentToken);
            } else {
                _reader.restore(state);
                tokenizeLiteralTagText();
                co_yield std::move(_currentToken);
            }
        } else {
            tokenizeText();
            co_yield std::move(_currentToken);
        }
    }
    co_return;
}

auto HtmlTokenizer::tokenizeTag() -> bool {
    if (!_reader.advanceIf(U'<')) {
        return false;
    }
    if (_reader.advanceIf(U'!')) {
        return tokenizeDeclaration();
    }
    if (_reader.advanceIf(U'/')) {
        return tokenizeClosingTag();
    }
    return tokenizeOpeningTag();
}

auto HtmlTokenizer::tokenizeDeclaration() -> bool {
    const auto savedState = _reader.save();
    if (tokenizeComment()) {
        return true;
    }
    _reader.restore(savedState);
    return tokenizeDocType();
}

auto HtmlTokenizer::tokenizeClosingTag() -> bool {
    const auto tagName = parseName();
    if (!tagName.has_value()) {
        return false;
    }
    skipWhitespace();
    if (!_reader.advanceIf(U'>')) {
        return false;
    }
    _currentToken = HtmlToken{HtmlTokenType::TagClose, *tagName};
    return true;
}

auto HtmlTokenizer::tokenizeOpeningTag() -> bool {
    const auto tagName = parseName();
    if (!tagName.has_value()) {
        return false;
    }
    _currentToken = HtmlToken{HtmlTokenType::TagOpen, *tagName};
    while (!_reader.isAtEnd()) {
        skipWhitespace();
        if (_reader.advanceIf(U'>')) {
            return true;
        }
        if (_reader.advanceIf(U'/')) {
            if (!_reader.advanceIf(U'>')) {
                return false;
            }
            _currentToken.selfClosing = true;
            return true;
        }

        const auto attributeName = parseName();
        if (!attributeName.has_value()) {
            return false;
        }
        auto attribute = HtmlAttribute{*attributeName};

        skipWhitespace();
        if (_reader.advanceIf(U'=')) {
            skipWhitespace();

            auto attributeValue = String{};
            if (!parseAttributeValue(attributeValue)) {
                return false;
            }
            attribute.setValue(std::move(attributeValue));
        }
        _currentToken.attributes.emplace_back(std::move(attribute));
    }
    return false;
}

void HtmlTokenizer::tokenizeText() {
    _reader.clearBuffer();
    while (!_reader.isAtEnd() && _reader.peek() != U'<') {
        auto decodedCharacter = Char{};
        if (_reader.peek() == U'&' && decodeEntity(decodedCharacter)) {
            _reader.appendToBuffer(decodedCharacter);
            continue;
        }
        _reader.readToBuffer();
    }
    _currentToken = HtmlToken{HtmlTokenType::Text, takeBufferString()};
}

void HtmlTokenizer::tokenizeLiteralTagText() {
    _reader.clearBuffer();
    if (_reader.peek() == U'<') {
        _reader.readToBuffer();
    }
    while (!_reader.isAtEnd() && _reader.peek() != U'<') {
        auto decodedCharacter = Char{};
        if (_reader.peek() == U'&' && decodeEntity(decodedCharacter)) {
            _reader.appendToBuffer(decodedCharacter);
            continue;
        }
        _reader.readToBuffer();
    }
    _currentToken = HtmlToken{HtmlTokenType::Text, takeBufferString()};
}

auto HtmlTokenizer::tokenizeComment() -> bool {
    if (!_reader.advanceIf(U'-') || !_reader.advanceIf(U'-')) {
        return false;
    }

    _reader.clearBuffer();
    while (!_reader.isAtEnd()) {
        if (_reader.peek() == U'-' && peekNext() == U'-') {
            _reader.advance();
            _reader.advance();
            if (!_reader.advanceIf(U'>')) {
                return false;
            }
            _currentToken = HtmlToken{HtmlTokenType::Comment, takeBufferString()};
            return true;
        }
        _reader.readToBuffer();
    }
    return false;
}

auto HtmlTokenizer::tokenizeDocType() -> bool {
    constexpr auto cDocType = std::array{U'd', U'o', U'c', U't', U'y', U'p', U'e'};
    for (const auto expectedCharacter : cDocType) {
        if (_reader.peek().toAsciiLowercase() != expectedCharacter) {
            return false;
        }
        _reader.advance();
    }
    if (!_reader.isAtEnd() && !_reader.peek().isAsciiWhitespace() && _reader.peek() != U'>') {
        return false;
    }

    skipWhitespace();
    _reader.clearBuffer();
    while (!_reader.isAtEnd() && _reader.peek() != U'>') {
        _reader.readToBuffer();
    }
    if (!_reader.advanceIf(U'>')) {
        return false;
    }
    _currentToken = HtmlToken{HtmlTokenType::DocType, trimmedBufferString()};
    return true;
}

auto HtmlTokenizer::decodeEntity(Char &decodedCharacter) -> bool {
    const auto savedState = _reader.save();
    if (!_reader.advanceIf(U'&')) {
        return false;
    }

    if (_reader.advanceIf(U'#')) {
        auto base = 10U;
        if (_reader.peek() == U'x' || _reader.peek() == U'X') {
            base = 16U;
            _reader.advance();
        }

        auto hasDigits = false;
        auto value = uint32_t{0};
        while (!_reader.isAtEnd()) {
            const auto digitValue = _reader.peek().digitValue();
            if (!digitValue.has_value() || digitValue.value() >= base) {
                break;
            }
            const auto digit = digitValue.value();
            if (value > ((0x10FFFFU - digit) / base)) {
                _reader.restore(savedState);
                return false;
            }
            hasDigits = true;
            value = (value * base) + digit;
            _reader.advance();
        }
        decodedCharacter = Char{static_cast<char32_t>(value)};
        if (!hasDigits || !_reader.advanceIf(U';') || !decodedCharacter.isValidUnicode()) {
            _reader.restore(savedState);
            return false;
        }
        return true;
    }

    auto name = std::u32string{};
    while (
        !_reader.isAtEnd() && _reader.peek() != U';' && !_reader.peek().isAsciiWhitespace() && _reader.peek() != U'<' &&
        _reader.peek() != U'&') {
        name.push_back(_reader.peek().toRawValue());
        _reader.advance();
    }
    if (!_reader.advanceIf(U';')) {
        _reader.restore(savedState);
        return false;
    }

    if (name == U"amp") {
        decodedCharacter = U'&';
        return true;
    }
    if (name == U"lt") {
        decodedCharacter = U'<';
        return true;
    }
    if (name == U"gt") {
        decodedCharacter = U'>';
        return true;
    }
    if (name == U"quot") {
        decodedCharacter = U'"';
        return true;
    }
    if (name == U"apos") {
        decodedCharacter = U'\'';
        return true;
    }

    _reader.restore(savedState);
    return false;
}

auto HtmlTokenizer::parseAttributeValue(String &value) -> bool {
    if (_reader.isAtEnd()) {
        return false;
    }

    if (_reader.peek() == U'"' || _reader.peek() == U'\'') {
        const auto quoteCharacter = _reader.peek();
        _reader.advance();
        _reader.clearBuffer();
        while (!_reader.isAtEnd() && _reader.peek() != quoteCharacter) {
            auto decodedCharacter = Char{};
            if (_reader.peek() == U'&' && decodeEntity(decodedCharacter)) {
                _reader.appendToBuffer(decodedCharacter);
                continue;
            }
            _reader.readToBuffer();
        }
        if (!_reader.advanceIf(quoteCharacter)) {
            return false;
        }
        value = takeBufferString();
        return true;
    }

    if (isAttributeValueTerminator(_reader.peek())) {
        return false;
    }
    _reader.clearBuffer();
    while (!_reader.isAtEnd() && !isAttributeValueTerminator(_reader.peek())) {
        auto decodedCharacter = Char{};
        if (_reader.peek() == U'&' && decodeEntity(decodedCharacter)) {
            _reader.appendToBuffer(decodedCharacter);
            continue;
        }
        _reader.readToBuffer();
    }
    value = takeBufferString();
    return !value.isEmpty();
}

auto HtmlTokenizer::parseName() -> std::optional<String> {
    _reader.clearBuffer();
    while (!_reader.isAtEnd() && !_reader.peek().isAsciiWhitespace() && !isNameTerminator(_reader.peek())) {
        _reader.readToBuffer();
    }
    auto result = takeBufferString();
    if (result.isEmpty()) {
        return std::nullopt;
    }
    return result;
}

void HtmlTokenizer::skipWhitespace() noexcept {
    while (!_reader.isAtEnd() && _reader.peek().isAsciiWhitespace()) {
        _reader.advance();
    }
}

auto HtmlTokenizer::takeBufferString() -> String {
    return _reader.takeBuffer().toU8String();
}

auto HtmlTokenizer::trimmedBufferString() const -> String {
    return _reader.bufferView().toU8String().trimmed();
}

auto HtmlTokenizer::peekNext() noexcept -> Char {
    const auto state = _reader.save();
    _reader.advance();
    const auto result = _reader.peek();
    _reader.restore(state);
    return result;
}

auto HtmlTokenizer::isNameTerminator(const Char character) noexcept -> bool {
    return character == U'>' || character == U'/' || character == U'=';
}

auto HtmlTokenizer::isAttributeValueTerminator(const Char character) noexcept -> bool {
    return character.isAsciiWhitespace() || character == U'>' || character == U'/';
}

}
