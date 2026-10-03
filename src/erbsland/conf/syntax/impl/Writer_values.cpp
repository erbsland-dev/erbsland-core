// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Writer.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/StringList.hpp"

#include <algorithm>

namespace erbsland::conf::syntax::impl {

using namespace text;
using namespace unit;

using namespace text::literals;

auto Writer::renderLine(
    const LayoutLine &line, const bool first, const util::List<CpCount> &columns, const bool preserveSpacing)
    -> String {
    auto out = StringEditor{};
    auto payload = false;
    auto valueArea = false;
    auto column = ItemIndex{};
    TokenPtr previousValue;
    for (const auto &token : line.tokens) {
        const auto kind = token->kind();
        if (kind == TokenKind::Spacing) {
            if (payload || (preserveSpacing && valueArea)) {
                out.append(token->text());
            }
            continue;
        }
        if (kind == TokenKind::Indentation) {
            out.append(indentation());
            continue;
        }
        if (kind == TokenKind::NameValueSeparator) {
            pad(out, _style.separatorPositionMode(), _style.separatorPosition(), CpCount{1});
            out.append(_style.separator());
            continue;
        }
        if (kind == TokenKind::ValueListSeparator) {
            out.append(","_el);
            if (preserveSpacing) {
                continue;
            }
            auto spaces = CpCount{1};
            if (column.isWithin(columns.count()) && previousValue) {
                spaces += columns.getRef(column).subtracted(tokenText(*previousValue).characterLength());
            }
            out.append(Char{U' '}, spaces);
            ++column;
            continue;
        }
        if (kind == TokenKind::MultiLineValueListSeparator) {
            out.append("*"_el).append(Char{U' '}, _style.spacesAfterStar());
            continue;
        }
        if (kind == TokenKind::Comment) {
            if (!out.isEmpty() && !(preserveSpacing && String{out}.endsWith(" "_el))) {
                out.append(" "_el);
            }
            out.append(token->text());
            continue;
        }
        if (first && !out.isEmpty()) {
            // The first literal begins immediately after the assignment separator.
            const auto current = String{out};
            if (current.endsWith(":"_el) || current.endsWith("="_el)) {
                pad(out, _style.valuePositionMode(), _style.valuePosition());
            }
        }
        out.append(tokenText(*token));
        payload = payload || kind.isPayload();
        if (kind.isValue()) {
            previousValue = token;
            valueArea = true;
        }
    }
    out.append(line.newline);
    return out;
}

auto Writer::matrixColumns() -> util::List<CpCount> {
    auto widths = util::List<CpCount>{};
    auto matrix = false;
    for (const auto &line : _lines) {
        auto bullet = false;
        auto column = ItemIndex{};
        for (const auto &token : line.tokens) {
            if (token->kind() == TokenKind::MultiLineValueListSeparator) {
                bullet = true;
            }
            if (bullet && token->kind() == TokenKind::ValueListSeparator) {
                matrix = true;
            }
            if (bullet && token->kind().isValue()) {
                if (column == ItemIndex::end(widths.count())) {
                    widths.append(CpCount{});
                }
                widths.set(column, std::max(widths.getRef(column), tokenText(*token).characterLength()));
                ++column;
            }
        }
    }
    if (!matrix) {
        return {};
    }
    for (auto i = ItemIndex{}; i.isWithin(_lines.count()); ++i) {
        const auto rendered = renderLine(_lines.getRef(i), i.isZero(), widths);
        if (rendered.characterLength().subtracted(_lines.getRef(i).newline.characterLength()) >
            _style.maximumLineLength()) {
            return {};
        }
    }
    return widths;
}

auto Writer::assignment(const Statement &statement) -> String {
    if (auto vector = reflowVector(statement)) {
        return *vector;
    }
    if (const auto value = reflowValue()) {
        return *value;
    }
    auto widths = util::List<CpCount>{};
    if (_style.matrixStyle() == MatrixStyle::Align) {
        widths = matrixColumns();
    }
    auto preserveSpacing = false;
    if (_style.matrixStyle() == MatrixStyle::Keep) {
        for (const auto &line : _lines) {
            auto bullet = false;
            for (const auto &token : line.tokens) {
                if (token->kind() == TokenKind::MultiLineValueListSeparator) {
                    bullet = true;
                }
                if (bullet && token->kind() == TokenKind::ValueListSeparator) {
                    preserveSpacing = true;
                }
            }
        }
    }
    auto result = StringEditor{};
    for (auto i = ItemIndex{}; i.isWithin(_lines.count()); ++i) {
        result.append(renderLine(_lines.getRef(i), i.isZero(), widths, preserveSpacing));
    }
    return result;
}

}
