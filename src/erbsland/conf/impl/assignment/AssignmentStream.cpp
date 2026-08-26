// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AssignmentStream.hpp"

#include "../char/NamedChars.hpp"

#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../re/RegEx.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../text/StringList.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

auto AssignmentStream::create(LexerPtr lexer) -> AssignmentStreamPtr {
    return std::make_shared<AssignmentStream>(std::move(lexer), PrivateTag{});
}

AssignmentStream::AssignmentStream(LexerPtr lexer, PrivateTag) : _lexer(std::move(lexer)) {
    assert(_lexer != nullptr);
}

auto AssignmentStream::assignments() -> AssignmentGenerator {
    initialize();
    // Read tokens until the end of the document is reached.
    while (token().type() != TokenType::EndOfData) {
        switch (token().type().raw()) {
        case TokenType::LineBreak:
        case TokenType::Indentation:
        case TokenType::Spacing:
        case TokenType::Comment:
            next(); // Consume empty lines, comments, indentation and spacing.
            break;
        case TokenType::MetaName:
            co_yield handleMetaValue();
            break;
        case TokenType::RegularName:
        case TokenType::TextName:
            co_yield handleValue();
            break;
        case TokenType::SectionMapOpen:
        case TokenType::SectionListOpen:
            co_yield handleSection();
            break;
        default:
            // Coverage: The lexer catches most errors, probably never used.
            throwSyntaxError("Expected a section or named value, but got something else."_el);
        }
    }
    // Resume past the terminal token so the lexer can finalize its digest and release the decoder.
    if (_lexerGenerator.next().has_value()) {
        throw ConfError{ConfErrorCategory::Internal, "The lexer yielded another token after the end-of-data token."_el};
    }
    co_yield Assignment{}; // signal the end of the document.
    co_return;
}

void AssignmentStream::initialize() {
    _documentArea = DocumentArea::Root; // make it explicit.
    _currentSectionPath = {};
    _lexerGenerator = _lexer->tokens();
    // Start with the first token from the token stream.
    if (auto nextToken = _lexerGenerator.next(); nextToken.has_value()) {
        _token = std::move(*nextToken);
    } else {
        // If the stream is unexpectedly empty, make sure the initial token is the end-of-data token.
        _token = LexerToken{TokenType::EndOfData};
    }
}

void AssignmentStream::next() {
    while (auto nextToken = _lexerGenerator.next()) {
        _token = std::move(*nextToken);

        if (_token.type() != TokenType::Spacing && _token.type() != TokenType::Comment) {
            return; // Found a meaningful token.
        }
    }

    // If we reached the end, set to EndOfData.
    _token = LexerToken{TokenType::EndOfData};
}

void AssignmentStream::expectNext() {
    next();
    if (token().type() == TokenType::EndOfData) {
        throwUnexpectedEndError("Unexpected end of the document."_el);
    }
    if (token().type() == TokenType::LineBreak) {
        throwUnexpectedEndError("Unexpected end of the the line."_el);
    }
}

void AssignmentStream::nextAndVerify(const TokenType expectedTokenType, text::String errorMessage) {
    next();
    if (token().type() == TokenType::EndOfData) {
        throwUnexpectedEndError("Unexpected end of the document."_el);
    }
    if (expectedTokenType != TokenType::LineBreak && token().type() == TokenType::LineBreak) {
        throwSyntaxError("Unexpected end of the the line."_el);
    }
    if (token().type() != expectedTokenType) {
        throwSyntaxError(std::move(errorMessage));
    }
}

void AssignmentStream::verifyAndConsumeEndOfLine() {
    if (token().type() == TokenType::EndOfData) {
        return; // accept the end of the token stream.
    }
    if (token().type() == TokenType::LineBreak) {
        next(); // Consume the line-break token.
        return;
    }
    throwSyntaxError("Expected the end of the line, or the end of the document."_el);
}

auto AssignmentStream::token() const -> const LexerToken & {
    return _token;
}

void AssignmentStream::throwSyntaxError(text::String message) const {
    throw ConfError(ConfErrorCategory::Syntax, std::move(message), currentLocation());
}

void AssignmentStream::throwSyntaxError(text::String message, const NamePath &namePath) const {
    throw ConfError(ConfErrorCategory::Syntax, std::move(message), currentLocation(), namePath);
}

void AssignmentStream::throwUnsupportedError(text::String message) const {
    throw ConfError(ConfErrorCategory::Unsupported, std::move(message), currentLocation());
}

void AssignmentStream::throwUnexpectedEndError(text::String message) const {
    throw ConfError(ConfErrorCategory::UnexpectedEnd, std::move(message), currentLocation());
}

void AssignmentStream::throwLimitExceededError(text::String message) const {
    throw ConfError(ConfErrorCategory::LimitExceeded, std::move(message), currentLocation());
}

auto AssignmentStream::currentLocation() const noexcept -> Location {
    return Location{_lexer->sourceIdentifier(), _token.begin()};
}

auto AssignmentStream::handleMetaValue() -> Assignment {
    auto name = Name::createRegular(std::get<text::String>(token().content()));
    const auto nameLocation = currentLocation();
    if (std::ranges::find(Name::allMetaNames(), name) == Name::allMetaNames().end()) {
        throwSyntaxError("Unknown meta value name."_el, name);
    }
    if (name == Name::metaSignature() && !token().begin().line().isNoIndex() && !token().begin().line().isZero()) {
        throwSyntaxError("Signature must be defined in the first line of the document."_el);
    }
    if ((name == Name::metaVersion() || name == Name::metaFeatures()) && _documentArea != DocumentArea::Root) {
        throwSyntaxError("The version and features must be defined before the first section."_el);
    }
    expectNext(TokenType::NameValueSeparator);
    nextAndVerify(TokenType::Text, "Only single-line text is supported for a meta value or command."_el);
    auto valueText = std::get<text::String>(token().content());
    next();
    if (token().type() == TokenType::ValueListSeparator) {
        throwSyntaxError("Only single text value is supported for a meta value or command."_el);
    }
    verifyAndConsumeEndOfLine();
    if (name == Name::metaVersion()) {
        if (_readMetaVersion) {
            throwSyntaxError("The '@version' meta-value must be defined only once."_el);
        }
        if (valueText != text::String{defaults::languageVersion}) {
            throwUnsupportedError("This parser only supports version 1.0 of the configuration language."_el);
        }
        _readMetaVersion = true;
    } else if (name == Name::metaFeatures()) {
        if (_readMetaFeatures) {
            throwSyntaxError("The '@features' meta-value must be defined only once."_el);
        }
        verifyFeatures(valueText);
        _readMetaFeatures = true;
    } else if (name == Name::metaInclude()) { // include
        // After each @include, reset the section path.
        _lastAbsolutePath = {};
        _currentSectionPath = {};
    }
    return Assignment{
        AssignmentType::MetaValue, NamePath(std::move(name)), nameLocation, Value::createText(std::move(valueText))};
}

auto AssignmentStream::handleValue() -> Assignment {
    const bool isTextName = token().type() == TokenType::TextName;
    auto name = isTextName ? Name::createText(std::get<text::String>(token().content()))
                           : Name::createRegular(std::get<text::String>(token().content()));
    auto nameLocation = currentLocation();
    const auto createAssignment = [&](ValuePtr &&value) noexcept -> Assignment {
        value->setLocation(nameLocation); // copy
        auto namePath = _currentSectionPath;
        namePath.append(std::move(name));
        return Assignment{AssignmentType::Value, std::move(namePath), std::move(nameLocation), value};
    };

    expectNext(TokenType::NameValueSeparator);
    next(); // get either a line-break or the start of a value.
    if (token().type() == TokenType::LineBreak) {
        // If we got a line-break, the value must be indented on the next line.
        expectNext(TokenType::Indentation);
        expectNext();
    }
    switch (token().type().raw()) {
    case TokenType::Integer:
    case TokenType::Float:
    case TokenType::Boolean:
    case TokenType::Text:
    case TokenType::Code:
    case TokenType::RegEx:
    case TokenType::Date:
    case TokenType::DateTime:
    case TokenType::Time:
    case TokenType::TimeDelta:
    case TokenType::Bytes: {
        auto valueList = handleValueOrValueList();
        if (valueList.size() == 1) {
            return createAssignment(std::move(valueList.front()));
        }
        auto value = Value::createValueList(std::move(valueList));
        return createAssignment(std::move(value));
    }
    case TokenType::MultiLineValueListSeparator: {
        auto valueList = handleMultiLineValueList();
        if (valueList.size() == 1) {
            return createAssignment(std::move(valueList.front()));
        }
        auto value = Value::createValueList(std::move(valueList));
        return createAssignment(std::move(value));
    }
    case TokenType::MultiLineTextOpen:
    case TokenType::MultiLineCodeOpen: {
        auto text = handleMultiLineText();
        auto value = Value::createText(std::move(text));
        return createAssignment(std::move(value));
    }
    case TokenType::MultiLineRegexOpen: {
        auto text = handleMultiLineRegEx();
        auto value = Value::createRegEx(createRegEx(text, true));
        return createAssignment(std::move(value));
    }
    case TokenType::MultiLineBytesOpen: {
        auto data = handleMultiLineBytes();
        auto value = Value::createBytes(std::move(data));
        return createAssignment(std::move(value));
    }
    default:
        throw ConfError(ConfErrorCategory::Internal, "Unexpected token for value."_el);
    }
}

auto AssignmentStream::handleValueOrValueList() -> std::vector<ValuePtr> {
    std::vector<ValuePtr> valueList;
    ValuePtr value;
    while (!(token().type() == TokenType::LineBreak || token().type() == TokenType::EndOfData)) {
        switch (token().type().raw()) {
        case TokenType::Integer:
            value = Value::createInteger(std::get<Integer>(token().content()));
            break;
        case TokenType::Float:
            value = Value::createFloat(std::get<Float>(token().content()));
            break;
        case TokenType::Boolean:
            value = Value::createBoolean(std::get<bool>(token().content()));
            break;
        case TokenType::Text:
        case TokenType::Code:
            value = Value::createText(std::get<text::String>(token().content()));
            break;
        case TokenType::RegEx:
            value = Value::createRegEx(createRegEx(std::get<text::String>(token().content()), false));
            break;
        case TokenType::Date:
            value = Value::createDate(std::get<time::Date>(token().content()));
            break;
        case TokenType::DateTime:
            value = Value::createDateTime(std::get<time::DateTime>(token().content()));
            break;
        case TokenType::Time:
            if (std::holds_alternative<time::Time>(token().content())) {
                value = Value::createTime(std::get<time::Time>(token().content()));
            } else {
                value = Value::createTimeWithZone(std::get<time::TimeWithZone>(token().content()));
            }
            break;
        case TokenType::TimeDelta:
            value = Value::createCalendarDelta(std::get<time::CalendarDelta>(token().content()));
            break;
        case TokenType::Bytes:
            value = Value::createBytes(std::get<mem::ByteBlock>(token().content()));
            break;
        default:
            throw ConfError(ConfErrorCategory::Internal, "Unexpected token type for value."_el);
        }
        value->setLocation(currentLocation());
        valueList.emplace_back(std::move(value));
        next();     // Consume the value
        if (token().type() == TokenType::ValueListSeparator) {
            next(); // Consume the seperator
        }
    }
    verifyAndConsumeEndOfLine();
    return valueList;
}

}
