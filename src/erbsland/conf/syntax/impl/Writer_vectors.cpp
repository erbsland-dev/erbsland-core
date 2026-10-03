// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Writer.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/StringJoins.hpp"

namespace erbsland::conf::syntax::impl {

using namespace text;
using namespace unit;
using namespace text::literals;

auto Writer::vectorValues(LayoutLine &prefix) const -> StringList {
    auto values = StringList{};
    auto seenSeparator = false;
    for (const auto &line : _lines) {
        auto bullet = false;
        for (const auto &token : line.tokens) {
            const auto kind = token->kind();
            if (kind == TokenKind::Comment || isMultilineToken(kind)) {
                return {};
            }
            if (!seenSeparator) {
                prefix.tokens.append(token);
            }
            if (kind == TokenKind::NameValueSeparator) {
                seenSeparator = true;
            }
            if (kind == TokenKind::MultiLineValueListSeparator) {
                bullet = true;
            }
            if (bullet && kind == TokenKind::ValueListSeparator) {
                return {};
            }
            if (kind.isValue()) {
                values.append(tokenText(*token));
            }
        }
    }
    return values;
}

auto Writer::compactVector(const String &prefix, const StringList &values) const -> std::optional<String> {
    if (_style.vectorStyle() != VectorStyle::Compact || values.count() < ItemCount{2}) {
        return {};
    }
    const auto value = values.join(", "_el);
    auto sameLine = StringEditor{prefix};
    pad(sameLine, _style.valuePositionMode(), _style.valuePosition());
    const auto nextLineWidth = indentation().characterLength() + value.characterLength();
    const auto wasNextLine = _lines.count() > ItemCount{1};
    const auto fitsContinuation = nextLineWidth <= _style.maximumLineLength() &&
        valueOnNextLine(sameLine.characterLength() + value.characterLength(), nextLineWidth, wasNextLine);
    if (sameLine.characterLength() + value.characterLength() > _style.maximumLineLength() && !fitsContinuation) {
        return {};
    }
    return placeValue(prefix, value, wasNextLine, _lines.getRef(_lines.lastIndex()).newline);
}

auto Writer::multilineVector(const String &prefix, const StringList &values) const -> String {
    auto bullet = StringEditor{indentation()};
    bullet.append("*"_el).append(Char{U' '}, _style.spacesAfterStar());
    const auto separator = text::join(newline(), bullet);
    return text::join(prefix, separator, values.join(separator), _lines.getRef(_lines.lastIndex()).newline);
}

auto Writer::reflowVector(const Statement &statement) -> std::optional<String> {
    if (_style.vectorStyle() == VectorStyle::Keep || statement.valueType() != ValueType::ValueList) {
        return {};
    }
    auto prefix = LayoutLine{};
    const auto values = vectorValues(prefix);
    if (values.isEmpty()) {
        return {};
    }
    const auto assignment = renderLine(prefix, true);
    if (const auto compact = compactVector(assignment, values)) {
        return compact;
    }
    return multilineVector(assignment, values);
}

}
