// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AssemblerToken.hpp"
#include "DataSection.hpp"
#include "OperationData.hpp"

#include "../error/InternalError.hpp"
#include "../Limits.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../unit/CodeLocation.hpp"
#include "../../../unit/ColumnCount.hpp"

#include <set>
#include <utility>

namespace erbsland::re::impl {

using namespace text::literals;

/// A simple tokenizer to safely read assembly code.
class AssemblerTokenizer {
public:
    /// The maximum length of keywords, identifiers, and labels.
    constexpr static std::size_t maxIdentifierLength = 16U;

public:
    /// Create a tokenizer for one assembly `line`.
    explicit AssemblerTokenizer(const text::String &line) noexcept : _reader{line} {}

    // defaults/deletions
    ~AssemblerTokenizer() = default;
    AssemblerTokenizer(const AssemblerTokenizer &) = delete;
    AssemblerTokenizer(AssemblerTokenizer &&) = delete;
    auto operator=(const AssemblerTokenizer &) -> AssemblerTokenizer & = delete;
    auto operator=(AssemblerTokenizer &&) -> AssemblerTokenizer & = delete;

public:
    /// Return all tokens for the given line.
    [[nodiscard]] auto tokens() -> AssemblerTokens {
        ERBSLAND_CORE_RE_REQUIRE_DEBUG(_reader.position().isZero(), "Don't call 'tokens()' twice"_el);
        if (_reader.isAtEnd()) {
            return {};
        }
        readNextSkipSpacing();
        while (!_character.isEndOfData()) {
            auto token = nextToken();
            if (token.type() != AssemblerToken::Comment) {
                _tokens.emplace_back(std::move(token));
            }
        }
        return std::move(_tokens);
    }

private: // char-level
    /// Advance to the next input character.
    void readNext() {
        if (!_reader.position().isWithin(limits::maximumAssemblerLineLength)) {
            throwError("Line too long"_el);
        }
        _character = _reader.read();
    }

    /// Consume following ASCII whitespace.
    void skipSpacing() {
        while (_character.isAsciiBlank()) {
            readNext();
        }
    }

    /// Advance once and consume following ASCII whitespace.
    void readNextSkipSpacing() {
        readNext();
        skipSpacing();
    }

    /// Test whether the current character ends a token.
    [[nodiscard]] auto isValidTokenEnd() const noexcept -> bool {
        return _character.isEndOfData() || _character.isAsciiBlank() || _character == U';' || _character == U',' ||
            _character == U'-';
    }

    /// Require the current character to end a token.
    void requireValidTokenEnd() {
        if (!isValidTokenEnd()) {
            throwError("Unexpected character"_el);
        }
    }

    /// Get the current input column.
    [[nodiscard]] auto currentColumn() const noexcept -> unit::ColumnIndex {
        return unit::ColumnIndex::fromSizeT(_reader.position().toSizeT()) - unit::ColumnCount::one();
    }

    /// Throw an assembler error at the current column.
    [[noreturn]] void throwError(text::String description) const {
        throw RegExError{
            ErrorCategory::Assembler,
            "Failed to assemble regular expression"_el,
            std::move(description),
            unit::CodeLocation{}.setColumn(currentColumn())};
    }

private: // token-level
    /// Create a token beginning at the current token start.
    [[nodiscard]] auto createToken(const AssemblerToken::Type type, AssemblerToken::Value value) const
        -> AssemblerToken {
        return AssemblerToken{type, std::move(value), _tokenStartColumn};
    }

    /// Read the next token from the input.
    [[nodiscard]] auto nextToken() -> AssemblerToken {
        _tokenStartColumn = currentColumn();
        if (_character.isDigitValue(text::IntegerBase::Decimal)) {
            return readInteger();
        }
        if (_character == U'"') {
            return readText();
        }
        if (_character == U'\'') {
            return readCharacter();
        }
        if (_character.isAsciiLetter()) {
            return readKeywordOrLabel();
        }
        if (_character == U'%') {
            return readLabel();
        }
        if (_character == U'$') {
            return readOffset();
        }
        if (_character == U'&') {
            return readIdentifier();
        }
        if (_character == U'.') {
            return readCommand();
        }
        if (_character == U',') {
            readNextSkipSpacing();
            return createToken(AssemblerToken::Comma, 0U);
        }
        if (_character == U'-') {
            readNextSkipSpacing();
            return createToken(AssemblerToken::Minus, 0U);
        }
        if (_character == U';') {
            // Skip the rest of the line while still enforcing the code-point line limit.
            while (!_character.isEndOfData()) {
                readNext();
            }
            return createToken(AssemblerToken::Comment, {});
        }
        throwError("Unexpected character"_el);
    }

    /// Read an unsigned decimal integer token.
    [[nodiscard]] auto readInteger() -> AssemblerToken {
        text::StringEditor integerStr;
        while (_character.isDigitValue(text::IntegerBase::Decimal)) {
            if (integerStr.length().toSizeT() >= std::numeric_limits<uint32_t>::digits10) {
                throwError("Integer too large"_el);
            }
            integerStr.append(_character);
            readNext();
        }
        requireValidTokenEnd();
        skipSpacing(); // skip any spacing to the next token.
        const auto value = integerStr.toInteger<std::uint32_t>();
        return createToken(AssemblerToken::Integer, value);
    }

    /// Read a quoted text token.
    [[nodiscard]] auto readText() -> AssemblerToken {
        text::StringEditor result;
        readNext(); // consume `"`
        while (_character != U'"') {
            if (result.length() > unit::ByteLength{2000U}) {
                throwError("Text literal too long"_el);
            }
            if (_character.isEndOfData()) {
                throwError("Unterminated text literal"_el);
            }
            if (!_character.isSafeUnicode()) {
                throwError("Invalid character in text literal"_el);
            }
            if (_character == U'\\') {
                readNext();
                if (_character.isEndOfData()) {
                    throwError("Unterminated text literal"_el);
                }
                switch (_character.toRawValue()) {
                case U'n':
                    result.append(U'\n');
                    break;
                case U'r':
                    result.append(U'\r');
                    break;
                case U't':
                    result.append(U'\t');
                    break;
                case U'\\':
                    result.append(U'\\');
                    break;
                case U'\'':
                    result.append(U'\'');
                    break;
                case U'"':
                    result.append(U'"');
                    break;
                default:
                    throwError("Invalid escape sequence"_el);
                }
                readNext(); // consume the escape character.
                continue;
            }
            result.append(_character);
            readNext();
        }
        readNext();    // consume the ending `"`.
        requireValidTokenEnd();
        skipSpacing(); // skip any spacing to the next token.
        return createToken(AssemblerToken::Text, std::move(result));
    }

    /// Read a quoted character token.
    [[nodiscard]] auto readCharacter() -> AssemblerToken {
        readNext(); // consume `'`
        auto character = text::Char::noCodePoint();
        while (_character != U'\'') {
            if (!character.isNoCodePoint()) {
                throwError("Character literal must have a single code-point"_el);
            }
            if (_character.isEndOfData()) {
                throwError("Unterminated character literal"_el);
            }
            if (!_character.isSafeUnicode()) {
                throwError("Invalid character in character literal"_el);
            }
            if (_character == U'\\') {
                readNext();
                if (_character.isEndOfData()) {
                    throwError("Unterminated character literal"_el);
                }
                switch (_character.toRawValue()) {
                case U'n':
                    character = U'\n';
                    break;
                case U'r':
                    character = U'\r';
                    break;
                case U't':
                    character = U'\t';
                    break;
                case U'\\':
                    character = U'\\';
                    break;
                case U'\'':
                    character = U'\'';
                    break;
                case U'"':
                    character = U'"';
                    break;
                default:
                    throwError("Invalid escape sequence"_el);
                }
                readNext(); // consume the escape character.
                continue;
            }
            character = _character;
            readNext();
        }
        readNext(); // consume `'`
        requireValidTokenEnd();
        skipSpacing();
        if (character.isNoCodePoint()) {
            throwError("Character literal must contain one code-point"_el);
        }
        return createToken(AssemblerToken::Char, static_cast<uint32_t>(character.toRawValue()));
    }

    /// Read a keyword, operation, modifier, or line label.
    [[nodiscard]] auto readKeywordOrLabel() -> AssemblerToken {
        text::StringEditor keyword;
        while (_character.isAsciiWord()) {
            if (keyword.length().toSizeT() >= maxIdentifierLength) {
                throwError("Keyword or label too long"_el);
            }
            keyword.append(_character.toAsciiLowercase());
            readNext();
        }
        if (_tokens.empty()) { // if we are at the start of the line, test for a label.
            const auto save = std::make_pair(_reader.save(), _character);
            if (_character.isAsciiBlank()) {
                readNextSkipSpacing();
            }
            if (_character == U':') { // This is a label.
                readNext();           // consume that.
                skipSpacing();
                return createToken(AssemblerToken::Label, std::move(keyword));
            }
            _reader.restore(save.first);
            _character = save.second;
        }
        requireValidTokenEnd();
        skipSpacing(); // skip any spacing to the next token.
        using namespace text::literals;
        if (keyword == "true"_el) {
            return createToken(AssemblerToken::Boolean, true);
        }
        if (keyword == "false"_el) {
            return createToken(AssemblerToken::Boolean, false);
        }
        if (keyword == "not"_el) {
            return createToken(AssemblerToken::Modifier, OperationModifier::Negated);
        }
        if (keyword == "assert"_el) {
            return createToken(AssemblerToken::Modifier, OperationModifier::Assert);
        }
        if (keyword == "ci"_el) {
            return createToken(AssemblerToken::Modifier, OperationModifier::CaseInsensitive);
        }
        if (keyword == "skip"_el) {
            return createToken(AssemblerToken::Modifier, OperationModifier::Skip);
        }
        if (keyword == "add"_el) {
            return createToken(AssemblerToken::Modifier, OperationModifier::Add);
        }
        if (keyword == "start"_el) {
            return createToken(AssemblerToken::Modifier, OperationModifier::Start);
        }
        if (keyword == "stop"_el) {
            return createToken(AssemblerToken::Modifier, OperationModifier::Stop);
        }
        try {
            const auto operation = baseOperationForString(keyword);
            return createToken(AssemblerToken::Operation, operation);
        } catch (const err::ParameterError &) {
            throwError(text::StringFormat{"Unknown keyword '{}'"}.build(keyword));
        }
    }

    /// Read a percent-prefixed label token.
    [[nodiscard]] auto readLabel() -> AssemblerToken {
        readNext(); // consume `%`
        text::StringEditor label;
        if (_character.isEndOfData() || _character.isAsciiBlank() || _character == U',') {
            throwError("Expected a label after '%'"_el);
        }
        if (!_character.isAsciiLetter()) {
            throwError("Unexpected character after '%'. Expected a letter"_el);
        }
        while (_character.isAsciiWord()) {
            if (label.length().toSizeT() >= maxIdentifierLength) {
                throwError("label too long"_el);
            }
            label.append(_character.toAsciiLowercase());
            readNext();
        }
        requireValidTokenEnd();
        skipSpacing(); // skip any spacing to the next token.
        return createToken(AssemblerToken::Label, std::move(label));
    }

    /// Read a dollar-prefixed hexadecimal offset token.
    [[nodiscard]] auto readOffset() -> AssemblerToken {
        readNext(); // consume `$`
        text::StringEditor offset;
        while (_character.isDigitValue(text::IntegerBase::Hexadecimal)) {
            if (offset.length() >= unit::ByteLength{8U}) {
                throwError("Offset has too many digits"_el);
            }
            offset.append(_character);
            readNext();
        }
        if (offset.isEmpty()) {
            throwError("Missing offset after '$'"_el);
        }
        requireValidTokenEnd();
        skipSpacing(); // skip any spacing to the next token.
        auto parseOptions = text::IntegerParseOptions::stringDefault();
        parseOptions.setFixedBase(text::IntegerBase::Hexadecimal);
        const auto value = offset.toInteger<std::uint32_t>(0U, parseOptions);
        return createToken(AssemblerToken::Offset, value);
    }

    /// Read an ampersand-prefixed identifier token.
    [[nodiscard]] auto readIdentifier() -> AssemblerToken {
        readNext(); // consume `&`
        text::StringEditor identifier;
        while (_character.isAsciiWord()) {
            if (identifier.length().toSizeT() >= maxIdentifierLength) {
                throwError("identifier too long"_el);
            }
            identifier.append(_character);
            readNext();
        }
        if (identifier.isEmpty()) {
            throwError("Missing identifier after '&'"_el);
        }
        requireValidTokenEnd();
        skipSpacing(); // skip any spacing to the next token.
        return createToken(AssemblerToken::Identifier, std::move(identifier));
    }

    /// Read a dot-prefixed assembler command token.
    [[nodiscard]] auto readCommand() -> AssemblerToken {
        readNext(); // consume `.`
        text::StringEditor command;
        while (_character.isAsciiLetter()) {
            if (command.length().toSizeT() >= maxIdentifierLength) {
                throwError("command too long"_el);
            }
            command.append(_character);
            readNext();
        }
        if (command.isEmpty()) {
            throwError("Missing command after '.'"_el);
        }
        requireValidTokenEnd();
        skipSpacing();
        return createToken(AssemblerToken::Command, std::move(command));
    }

private:
    text::StringCharReader _reader;
    unit::ColumnIndex _tokenStartColumn;
    text::Char _character{text::Char::noCodePoint()};
    AssemblerTokens _tokens;
};

}
