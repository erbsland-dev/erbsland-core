// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Writer.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../text/BooleanFormat.hpp"
#include "../../../text/CharSet.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringJoins.hpp"
#include "../../../text/StringList.hpp"
#include "../../../text/StringSide.hpp"
#include "../../../text/StringTransformations.hpp"
#include "../../ConfError.hpp"

#include <utility>

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
    auto prefix = String{};
    auto name = text;
    if (name.startsWith("@"_el)) {
        prefix = "@"_el;
        name = name.slice(StringSide::Back, ByteIndex{1});
    }
    auto words = StringList::fromSplit(name, CharSet{U' ', U'_'});
    words.map([this, wordCase](const String &word) -> String {
        if (wordCase == WordCase::Title) {
            const auto upper = toAsciiUppercase(word);
            if (_style.abbreviations().contains(upper)) {
                return upper;
            }
            return toAsciiCapitalized(word);
        } else if (wordCase == WordCase::Upper) {
            return toAsciiUppercase(word);
        } else if (wordCase == WordCase::Lower) {
            return toAsciiLowercase(word);
        }
        return word;
    });
    if (separator == WordSeparator::Space) {
        return text::join(prefix, words.join(" "_el));
    }
    return text::join(prefix, words.join("_"_el));
}

auto Writer::booleanText(const String &source) const -> String {
    const auto lower = toAsciiLowercase(source);
    auto original = BooleanForm::TrueFalse;
    if (lower == "yes"_el || lower == "no"_el) {
        original = BooleanForm::YesNo;
    } else if (lower == "on"_el || lower == "off"_el) {
        original = BooleanForm::OnOff;
    } else if (lower == "enabled"_el || lower == "disabled"_el) {
        original = BooleanForm::EnabledDisabled;
    }
    auto form = _style.booleanForm();
    if (form == BooleanForm::Keep) {
        form = original;
    }
    if (_style.booleanCase() == WordCase::Keep && form == original) {
        return source;
    }
    const auto value = lower == "true"_el || lower == "yes"_el || lower == "on"_el || lower == "enabled"_el;
    auto format = BooleanFormat{};
    switch (form) {
    case BooleanForm::YesNo:
        format.setStyle(BooleanFormat::Style::YesNo);
        break;
    case BooleanForm::OnOff:
        format.setStyle(BooleanFormat::Style::OnOff);
        break;
    case BooleanForm::EnabledDisabled:
        format.setStyle(BooleanFormat::Style::EnabledDisabled);
        break;
    default:
        break;
    }
    if (_style.booleanCase() == WordCase::Title) {
        format.setCapitalization(Capitalization::Capitalized);
    } else if (_style.booleanCase() == WordCase::Upper) {
        format.setCapitalization(Capitalization::Uppercase);
    }
    return String{format.text(value)};
}

auto Writer::tokenText(const Token &token) const -> String {
    switch (token.kind().raw()) {
    case TokenKind::Boolean:
        return booleanText(token.text());
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
