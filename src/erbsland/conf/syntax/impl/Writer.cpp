// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Writer.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringJoins.hpp"
#include "../../ConfError.hpp"

namespace erbsland::conf::syntax::impl {

using namespace text;
using namespace unit;
using namespace err;

using namespace text::literals;

void Writer::validate(const FormatStyle &style) {
    if (style.maximumLineLength().isInfinite() || style.maximumLineLength().isZero() ||
        style.spacesAfterStar().isZero() || style.spacesAfterStar().isInfinite() || style.indentationCount().isZero() ||
        style.indentationCount().isInfinite() || style.separatorPosition().isInfinite() ||
        style.valuePosition().isInfinite() || (style.separator() != U':' && style.separator() != U'=')) {
        throw ParameterFailure{"Invalid ELCL format counts or separator."_el, "style"_el};
    }
}

auto Writer::write(const DocumentPtr &document, const Format &format) -> FormatResult {
    if (!document) {
        throw ParameterFailure{"The syntax document must not be null."_el, "document"_el};
    }
    _document = document;
    validate(format.baseStyle());
    auto out = StringEditor{};
    auto offset = ByteIndex{};
    auto diagnostics = document->diagnostics();
    for (const auto &statement : document->statements()) {
        out.append(document->text().slice(ByteRange{offset, statement.range().beginIndex()}));
        _style = format.resolve(statement);
        validate(_style);
        if (statement.hasErrors()) {
            out.append(document->text().slice(statement.range()));
        } else {
            splitLines(statement);
            if (statement.kind() == Statement::Kind::Section) {
                out.append(section(statement));
            } else {
                out.append(assignment(statement));
            }
        }
        offset = statement.range().endIndex();
    }
    out.append(document->text().slice(ByteRange{offset, ByteIndex::end(document->text().length())}));
    String result = out;
    if (result != document->text()) {
        for (const auto &token : document->tokens()) {
            if (token->kind() == TokenKind::MetaName && token->decodedText() == "@signature"_el) {
                diagnostics.append(
                    {ConfErrorCategory::Signature,
                        "Formatting changed signed source; re-sign the document."_el,
                        token->range(),
                        token->location(),
                        true});
                break;
            }
        }
    }
    return {std::move(result), std::move(diagnostics)};
}

void Writer::splitLines(const Statement &statement) {
    _lines.clear();
    auto line = LayoutLine{};
    for (auto i = statement.firstToken(); i < statement.endToken(); ++i) {
        const auto &token = _document->tokens().getRef(i);
        if (token->kind() == TokenKind::LineBreak) {
            line.newline = token->text();
            _lines.append(std::move(line));
            line = {};
        } else {
            line.tokens.append(token);
        }
    }
    if (!line.tokens.isEmpty() || _lines.isEmpty()) {
        _lines.append(std::move(line));
    }
}

void Writer::pad(StringEditor &out, const PositionMode mode, const CpCount count, const CpCount minimum) {
    auto spaces = count;
    if (mode == PositionMode::Column) {
        spaces = minimum;
        if (count > out.characterLength()) {
            spaces = count - out.characterLength();
        }
    }
    out.append(Char{U' '}, spaces);
}

auto Writer::nameText(const String &text, const WordCase wordCase, const WordSeparator separator) const -> String {
    auto result = StringEditor{};
    // ASCII case changes and separator replacements preserve the UTF-8 byte length.
    result.reserve(text.length());
    auto first = true;
    auto index = ByteIndex{};
    while (index.isWithin(text.length())) {
        auto ch = text.readCharAndAdvance(index);
        if (ch == U' ' || ch == U'_') {
            if (separator == WordSeparator::Space) {
                result.append(Char{U' '});
            } else {
                result.append(Char{U'_'});
            }
            first = true;
        } else {
            if (wordCase == WordCase::Upper || (wordCase == WordCase::Title && first)) {
                ch = ch.toAsciiUppercase();
            } else if (wordCase != WordCase::Keep) {
                ch = ch.toAsciiLowercase();
            }
            result.append(ch);
            if (ch != U'@') {
                first = false;
            }
        }
    }
    return result;
}

auto Writer::tokenText(const Token &token) const -> String {
    switch (token.kind().raw()) {
    case TokenKind::SectionName:
        return nameText(token.text(), _style.sectionNameCase(), _style.sectionNameSeparator());
    case TokenKind::RegularName:
    case TokenKind::MetaName:
        return nameText(token.text(), _style.valueNameCase(), _style.valueNameSeparator());
    case TokenKind::Text:
    case TokenKind::TextName:
    case TokenKind::SectionTextName: {
        auto casing = _style.textNameCase();
        auto separator = _style.textNameSeparator();
        if (token.kind() == TokenKind::Text) {
            casing = _style.textIdentifierCase();
            separator = _style.textIdentifierSeparator();
        }
        if (casing == WordCase::Keep) {
            return token.text();
        }
        try {
            [[maybe_unused]] const auto identifier = Name::createRegular(token.decodedText());
        } catch (const ConfError &) {
            return token.text();
        }
        return text::join("\""_el, nameText(token.decodedText(), casing, separator), "\""_el);
    }
    default:
        return token.text();
    }
}

auto Writer::section(const Statement &statement) -> String {
    auto header = StringEditor{};
    if (statement.isListSection()) {
        header.append("*"_el);
    }
    header.append("["_el);
    if (_style.spaceAfterOpen()) {
        header.append(" "_el);
    }
    auto first = true;
    String comment;
    for (const auto &token : _lines.getRef(ItemIndex{}).tokens) {
        if (token->kind() == TokenKind::SectionName || token->kind() == TokenKind::SectionTextName) {
            header.append(tokenText(*token));
            first = false;
        } else if (token->kind() == TokenKind::NamePathSeparator) {
            if (_style.spaceAroundDot() && !first) {
                header.append(" "_el);
            }
            header.append("."_el);
            if (_style.spaceAroundDot()) {
                header.append(" "_el);
            }
        } else if (token->kind() == TokenKind::Comment) {
            comment = token->text();
        }
    }
    if (_style.spaceBeforeClose()) {
        header.append(" "_el);
    }
    header.append("]"_el);
    auto star = CpCount{};
    if (statement.isListSection() && _style.sectionStar()) {
        header.append("*"_el);
        star = CpCount{1};
    }
    auto width = _style.maximumLineLength();
    if (!comment.isEmpty()) {
        width = width.subtracted(comment.characterLength() + CpCount{1});
    }
    auto before = CpCount{};
    auto after = CpCount{};
    auto leadingStar = CpCount{};
    if (statement.isListSection()) {
        leadingStar = CpCount{1};
    }
    if (_style.sectionIndent().isInfinite() && _style.sectionSuffix().isInfinite()) {
        const auto remaining = width.subtracted(header.characterLength()).toSizeT();
        before = CpCount::fromSizeT(remaining / 2);
        after = CpCount::fromSizeT(remaining - remaining / 2);
    } else if (_style.sectionIndent().isInfinite()) {
        after = _style.sectionSuffix().subtracted(star);
        before = width.subtracted(header.characterLength() + after);
    } else {
        before = _style.sectionIndent().subtracted(leadingStar);
        if (_style.sectionSuffix().isInfinite()) {
            after = width.subtracted(before + header.characterLength());
        } else {
            after = _style.sectionSuffix().subtracted(star);
        }
    }
    auto result = StringEditor{};
    result.append(Char{U'-'}, before).append(header).append(Char{U'-'}, after);
    if (!comment.isEmpty()) {
        result.append(" "_el).append(comment);
    }
    result.append(_lines.getRef(ItemIndex{}).newline);
    return result;
}

auto Writer::indentation() const -> String {
    auto ch = Char{U' '};
    if (_style.indentationStyle() == IndentationStyle::Tabs) {
        ch = Char{U'\t'};
    }
    return StringEditor{}.append(ch, _style.indentationCount());
}

auto Writer::newline() const -> String {
    for (const auto &line : _lines) {
        if (!line.newline.isEmpty()) {
            return line.newline;
        }
    }
    for (const auto &token : _document->tokens()) {
        if (token->kind() == TokenKind::LineBreak) {
            return token->text();
        }
    }
    return "\n"_el;
}

}
