// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Writer.hpp"

#include "../../../text/StringJoins.hpp"

namespace erbsland::conf::syntax::impl {

using namespace text;
using namespace unit;

auto Writer::isMultilineToken(const TokenKind kind) noexcept -> bool {
    return kind.isPayload() || kind == TokenKind::MultiLineTextOpen || kind == TokenKind::MultiLineCodeOpen ||
        kind == TokenKind::MultiLineRegexOpen || kind == TokenKind::MultiLineBytesOpen;
}

auto Writer::valueOnNextLine(const CpCount sameLineWidth, const CpCount nextLineWidth, const bool wasNextLine) const
    -> bool {
    switch (_style.valueWrapping()) {
    case ValueWrapping::Keep:
        return wasNextLine;
    case ValueWrapping::SameLine:
        return false;
    case ValueWrapping::NextLine:
        return true;
    case ValueWrapping::NextLineIfFits:
        return sameLineWidth > _style.maximumLineLength() && nextLineWidth <= _style.maximumLineLength();
    case ValueWrapping::WhenTooLong:
        return sameLineWidth > _style.maximumLineLength();
    }
    return false;
}

auto Writer::placeValue(
    const String &prefix, const String &value, const bool wasNextLine, const String &finalNewline) const -> String {
    auto sameLine = StringEditor{prefix};
    pad(sameLine, _style.valuePositionMode(), _style.valuePosition());
    const auto indent = indentation();
    if (valueOnNextLine(
            sameLine.characterLength() + value.characterLength(),
            indent.characterLength() + value.characterLength(),
            wasNextLine)) {
        return text::join(prefix, newline(), indent, value, finalNewline);
    }
    sameLine.append(value).append(finalNewline);
    return sameLine;
}

auto Writer::reflowValue() -> std::optional<String> {
    if (_lines.isEmpty() || _lines.count() > ItemCount{2}) {
        return {};
    }
    auto prefix = LayoutLine{};
    auto value = LayoutLine{};
    auto separatorSeen = false;
    auto valueSeen = false;
    auto wasNextLine = false;
    for (auto index = ItemIndex{}; index.isWithin(_lines.count()); ++index) {
        const auto &line = _lines.getRef(index);
        for (const auto &token : line.tokens) {
            const auto kind = token->kind();
            if (isMultilineToken(kind) || kind == TokenKind::MultiLineValueListSeparator) {
                return {};
            }
            if (!separatorSeen) {
                prefix.tokens.append(token);
                separatorSeen = kind == TokenKind::NameValueSeparator;
            } else {
                if (!valueSeen) {
                    if (kind == TokenKind::Comment) {
                        return {};
                    }
                    if (kind == TokenKind::Spacing || kind == TokenKind::Indentation) {
                        continue;
                    }
                    if (!kind.isValue()) {
                        return {};
                    }
                    valueSeen = true;
                    wasNextLine = !index.isZero();
                }
                value.tokens.append(token);
            }
        }
        // A second payload line or a standalone comment must retain its source structure.
        if (valueSeen && index != _lines.lastIndex()) {
            return {};
        }
    }
    if (!separatorSeen || !valueSeen) {
        return {};
    }
    return placeValue(
        renderLine(prefix, true), renderLine(value, false), wasNextLine, _lines.getRef(_lines.lastIndex()).newline);
}

}
