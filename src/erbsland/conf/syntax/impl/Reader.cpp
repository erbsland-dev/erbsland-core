// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Reader.hpp"

#include "SnapshotSource.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/StringSplitter.hpp"
#include "../../impl/constants/Limits.hpp"
#include "../../impl/decoder/TokenDecoder.hpp"
#include "../../impl/lexer/DocumentPart.hpp"

namespace erbsland::conf::syntax::impl {

using namespace text;
using namespace unit;

using namespace text::literals;
using conf::impl::CharClass;
using conf::impl::TokenType;
namespace lex = conf::impl::lexer;

auto Reader::read(String text, SourceIdentifierPtr identifier) -> DocumentPtr {
    initialize(std::move(text), std::move(identifier));
    while (!_decoder->character().isEndOfData()) {
        readPart();
    }
    indexLines();
    return std::move(_document);
}

void Reader::initialize(String text, SourceIdentifierPtr identifier) {
    _statement.reset();
    _position = CodeLocation{LineIndex{}, ColumnIndex{}, CpIndex{}};
    _offset = {};
    _absolute.clear();
    _section.reset();
    _document = std::make_shared<Document>();
    _document->_text = std::move(text);
    if (!identifier) {
        identifier = SourceIdentifier::createForText();
    }
    _document->_identifier = identifier;
    auto lines = StringSplitter{_document->_text, Char{U'\n'}, StringSplitMode::KeepSeparator};
    auto offset = ByteIndex{};
    while (!lines.isAtEnd()) {
        const auto line = lines.next();
        if (line.isEmpty()) {
            break;
        }
        _document->_lines.append(ByteRange{offset, line.length()});
        offset += line.length();
    }
    auto source = std::make_shared<SnapshotSource>(_document->_text, std::move(identifier));
    source->open();
    _decoder = conf::impl::TokenDecoder::create(
        conf::impl::CharStream::create(source), {}, conf::impl::TokenDecoderMode::Syntax);
    _decoder->initialize();
}

void Reader::readPart() {
    if (_decoder->isAtStatementStart()) {
        _statement.emplace();
        _statement->_firstToken = ItemIndex::end(_document->_tokens.count());
        _statement->_range = ByteRange{_offset, ByteCount{}};
        if (_decoder->character() == CharClass::SectionStart) {
            _statement->_kind = Statement::Kind::Section;
        }
    }
    for (const auto &token : lex::readDocumentPart(*_decoder)) {
        append(token);
    }
    for (const auto &context : _decoder->takeDiagnostics()) {
        diagnose(context);
    }
    finishStatement();
}

void Reader::indexLines() {
    auto tokenIndex = ItemIndex{};
    auto lineIndex = LineIndex{};
    for (const auto &line : _document->_lines) {
        auto tokens = util::List<TokenPtr>{};
        while (tokenIndex.isWithin(_document->_tokens.count())) {
            const auto &token = _document->_tokens.getRef(tokenIndex);
            if (token->range().beginIndex() >= line.endIndex()) {
                break;
            }
            tokens.append(token);
            ++tokenIndex;
        }
        _document->_lineTokens.append(std::move(tokens));
        if (line.length().toSizeT() > conf::impl::limits::maxLineLength) {
            _document->_diagnostics.append(
                Diagnostic{
                    ConfErrorCategory::LimitExceeded,
                    "Source line exceeds the ELCL byte limit."_el,
                    line,
                    CodeLocation{lineIndex, ColumnIndex{}}});
            for (auto i = ItemIndex{}; i.isWithin(_document->_statements.count()); ++i) {
                auto statement = _document->_statements.get(i);
                if (statement._range.beginIndex() < line.endIndex() &&
                    statement._range.endIndex() > line.beginIndex()) {
                    statement._hasErrors = true;
                    _document->_statements.set(i, std::move(statement));
                }
            }
        }
        ++lineIndex;
    }
}

void Reader::append(const conf::impl::LexerToken &source) {
    const auto kind = source.type();
    if (kind == TokenType::EndOfData || source.rawText().isEmpty()) {
        return;
    }
    auto token = std::make_shared<Token>();

    token->_kind = kind;
    token->_text = source.rawText();
    token->_range = ByteRange{_offset, token->_text.length()};
    token->_location = _position;
    auto characterIndex = ByteIndex{};
    while (characterIndex.isWithin(token->_text.length())) {
        if (token->_text.readCharAndAdvance(characterIndex) == U'\n') {
            _position.nextLine();
        } else {
            _position.nextColumn();
        }
    }
    if (const auto value = std::get_if<String>(&source.content())) {
        token->_decodedText = *value;
    }
    if (_statement && _statement->_kind == Statement::Kind::Section) {
        if (kind == TokenType::RegularName) {
            token->_kind = TokenKind::SectionName;
        }
        if (kind == TokenType::TextName) {
            token->_kind = TokenKind::SectionTextName;
        }
    }
    _offset += token->_text.length();
    _document->_tokens.append(std::move(token));
}

void Reader::diagnose(const ConfErrorContext &context) {
    auto location = context.location().value_or(CodeLocation{});
    location.setPosition(CpIndex::noIndex());
    auto range = ByteRange{_offset, ByteCount{}};
    if (!location.line().isNoIndex() && location.line().toSizeT() < _document->_lines.count().toSizeT()) {
        const auto lineRange = _document->_lines.getRef(ItemIndex::fromSizeT(location.line().toSizeT()));
        const auto line = _document->_text.slice(lineRange);
        auto index = ByteIndex{};
        auto column = ColumnIndex{};
        while (!location.column().isNoIndex() && column < location.column() && index.isWithin(line.length())) {
            [[maybe_unused]] const auto skipped = line.readCharAndAdvance(index);
            ++column;
        }
        auto end = index;
        if (end.isWithin(line.length())) {
            [[maybe_unused]] const auto character = line.readCharAndAdvance(end);
        }
        range =
            ByteRange{lineRange.beginIndex() + ByteCount::fromSizeT(index.toSizeT()), ByteRange{index, end}.length()};
    }
    _document->_diagnostics.append(Diagnostic{context.category(), context.description(), range, location});
    if (_statement) {
        _statement->_hasErrors = true;
    }
}

void Reader::finishStatement() {
    if (!_statement) {
        return;
    }
    _statement->_endToken = ItemIndex::end(_document->_tokens.count());
    _statement->_range = ByteRange{_statement->_range.beginIndex(), _offset};
    try {
        resolve(*_statement);
    } catch (const ConfError &error) {
        if (!conf::impl::TokenDecoder::isRecoverable(error.category())) {
            throw;
        }
        diagnose(error.context());
        if (_statement->_kind == Statement::Kind::Section) {
            _section.reset();
        }
    }
    _document->_statements.append(std::move(*_statement));
    _statement.reset();
}

void Reader::resolve(Statement &statement) {
    auto path = NamePath{};
    bool separator = false;
    bool hasValue = false;
    bool needsName = true;
    for (auto i = statement._firstToken; i < statement._endToken; ++i) {
        const auto &token = _document->_tokens.getRef(i);
        const auto kind = token->kind();
        if (kind == TokenKind::SectionListOpen) {
            statement._listSection = true;
        }
        if (kind == TokenKind::NamePathSeparator) {
            needsName = true;
        }
        if (kind == TokenKind::NamePathSeparator && path.empty()) {
            statement._relative = true;
        }
        if (kind == TokenKind::NameValueSeparator) {
            separator = true;
        }
        if (kind == TokenKind::MetaName) {
            statement._meta = true;
        }
        if (!separator &&
            (kind == TokenKind::RegularName || kind == TokenKind::MetaName || kind == TokenKind::SectionName)) {
            path.append(Name::createRegular(token->decodedText()));
            needsName = false;
        } else if (!separator && (kind == TokenKind::TextName || kind == TokenKind::SectionTextName)) {
            path.append(Name::createText(token->decodedText()));
            needsName = false;
        }
        const auto type = kind.toValueType();
        if (!type.isUndefined()) {
            if (!hasValue) {
                statement._valueType = type;
                hasValue = true;
            }
        }
        if (kind == TokenKind::ValueListSeparator || kind == TokenKind::MultiLineValueListSeparator) {
            statement._valueType = ValueType::ValueList;
            hasValue = true;
        }
    }
    statement._nameCount = path.size();
    if (statement._kind == Statement::Kind::Section) {
        if (statement._hasErrors) {
            _section.reset();
            return;
        }
        const auto location =
            Location{_document->_identifier, _document->_tokens.getRef(statement._firstToken)->location()};
        if (needsName) {
            throw ConfError{ConfErrorCategory::Syntax, "Expected a section name."_el, location};
        }
        if (path.size() > conf::impl::limits::maxNamePathLength) {
            throw ConfError{ConfErrorCategory::LimitExceeded, "Too many section name components."_el, location};
        }
        if (statement._relative) {
            if (_absolute.empty()) {
                throw ConfError{
                    ConfErrorCategory::Syntax, "Relative section has no preceding absolute section."_el, location};
            }
            path.prepend(_absolute);
        } else {
            _absolute = path;
        }
        _section = path;
        statement._path = path;
    } else if (!path.empty()) {
        if (statement._meta) {
            statement._path = path;
            if (path.front() == Name::metaInclude()) {
                _section.reset();
                _absolute.clear();
            }
        } else if (_section) {
            path.prepend(*_section);
            statement._path = path;
        }
    }
}

}
