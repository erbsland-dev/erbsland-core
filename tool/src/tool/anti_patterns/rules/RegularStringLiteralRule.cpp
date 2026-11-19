// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegularStringLiteralRule.hpp"

#include "../SourceIndex.hpp"

#include <erbsland/U8StringConstIterator.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto RegularStringLiteralRule::create() -> RulePtr {
    return std::make_unique<RegularStringLiteralRule>();
}

auto RegularStringLiteralRule::identifier() const -> el::String {
    return "regular_string_literal"_el;
}

auto RegularStringLiteralRule::title() const -> el::String {
    return "Core string literals"_el;
}

auto RegularStringLiteralRule::description() const -> el::String {
    return "Detect ordinary string literals where a Core string literal is expected."_el;
}

auto RegularStringLiteralRule::severity() const -> Severity {
    return Severity::Medium;
}

auto RegularStringLiteralRule::suffix(const SourceIndex &source, const el::cpp::CppToken &token) const -> el::String {
    const auto end = token.range().endIndex();
    auto next = end;
    while (next.isWithin(source.text().length()) && source.text().charAt(next).isAsciiWord()) {
        ++next;
    }
    return source.slice(el::ByteRange{end, next});
}

auto RegularStringLiteralRule::literalEnd(const SourceIndex &source, const el::cpp::CppToken &token) const
    -> el::ByteIndex {
    return token.range().endIndex() + suffix(source, token).length();
}

auto RegularStringLiteralRule::isParameterNameArgument(const SourceIndex &source, el::ItemIndex tokenIndex) const
    -> bool {
    const auto &tokens = source.tokens();
    auto closing = el::StringList{};
    auto foundComma = false;
    while (!tokenIndex.isZero()) {
        --tokenIndex;
        const auto &token = tokens.getRef(tokenIndex);
        if (token.kind() == el::cpp::CppToken::Kind::Comment) {
            continue;
        }
        if (token.kind() == el::cpp::CppToken::Kind::Directive) {
            return false;
        }
        const auto value = token.text();
        if (!foundComma) {
            if (value != ","_el) {
                return false;
            }
            foundComma = true;
            continue;
        }
        if (value == ")"_el || value == "]"_el || value == "}"_el) {
            closing.append(value);
        } else if (value == "("_el || value == "["_el || value == "{"_el) {
            if (!closing.isEmpty()) {
                const auto end = closing.takeLast();
                if ((value == "("_el && end != ")"_el) || (value == "["_el && end != "]"_el) ||
                    (value == "{"_el && end != "}"_el)) {
                    return false;
                }
                continue;
            }
            if (value == "["_el) {
                return false;
            }
            while (!tokenIndex.isZero()) {
                --tokenIndex;
                const auto &name = tokens.getRef(tokenIndex);
                if (name.kind() == el::cpp::CppToken::Kind::Comment) {
                    continue;
                }
                return name.text() == "throwParameterFailure"_el || name.text() == "ParameterFailure"_el;
            }
            return false;
        } else if (closing.isEmpty() && (value == ","_el || value == ";"_el)) {
            return false;
        }
    }
    return false;
}

auto RegularStringLiteralRule::adjacentGroupEnd(const SourceIndex &source, const el::ItemIndex index) const
    -> el::ItemIndex {
    auto last = index;
    const auto &literals = source.literals();
    while (last.incremented().isWithin(literals.count())) {
        const auto &next = source.tokens().getRef(literals.getRef(last.incremented()));
        const auto between = source.slice(
            el::ByteRange{literalEnd(source, source.tokens().getRef(literals.getRef(last))), next.position()});
        if (!between.trimmed().isEmpty()) {
            break;
        }
        ++last;
    }
    return last;
}

auto RegularStringLiteralRule::isExplicitContext(const el::String &before) -> bool {
    if (before.endsWith("{"_el) || before.endsWith("("_el)) {
        const auto bracket = el::ByteIndex::last(before.length());
        auto nameEnd = bracket;
        while (!nameEnd.isZero() && before.charAt(nameEnd.decremented()).isAsciiWhitespace()) {
            --nameEnd;
        }
        if (!nameEnd.isZero() && before.charAt(nameEnd.decremented()) == U'>') {
            auto depth = el::ItemCount{};
            for (auto position = nameEnd; !position.isZero(); --position) {
                const auto character = before.charAt(position.decremented());
                if (character == U'>') {
                    ++depth;
                } else if (character == U'<') {
                    --depth;
                    if (depth.isZero()) {
                        nameEnd = position.decremented();
                        break;
                    }
                }
            }
        }
        auto nameStart = nameEnd;
        while (!nameStart.isZero()) {
            const auto character = before.charAt(nameStart.decremented());
            if (!character.isAsciiWord() && character != U':') {
                break;
            }
            --nameStart;
        }
        const auto name = before.slice(el::ByteRange{nameStart, nameEnd});
        if (before.endsWith("{"_el) && !name.isEmpty()) {
            return true;
        }
        auto uppercase = false;
        for (const auto character : name) {
            uppercase |= character.isAsciiUppercaseLetter();
        }
        if (before.endsWith("("_el) && uppercase) {
            return true;
        }
    }
    return false;
}

void RegularStringLiteralRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    const auto &literals = source.literals();
    for (auto index = el::ItemIndex::zero(); index.isWithin(literals.count());) {
        const auto firstIndex = index;
        const auto &first = source.tokens().getRef(literals.getRef(index));
        const auto start = first.position();
        if (!first.text().startsWith("\""_el) && !first.text().startsWith("R\""_el) &&
            !first.text().startsWith("u"_el) && !first.text().startsWith("L"_el)) {
            ++index;
            continue;
        }
        const auto last = adjacentGroupEnd(source, index);
        index = last.incremented();
        const auto &final = source.tokens().getRef(literals.getRef(last));
        const auto end = literalEnd(source, final);
        auto prefixed = false;
        for (auto group = firstIndex; group <= last; ++group) {
            const auto &literal = source.tokens().getRef(literals.getRef(group)).text();
            prefixed |= literal.startsWith("u"_el) || literal.startsWith("U"_el) || literal.startsWith("L"_el);
        }
        if (prefixed || final.text().endsWith("_el"_el) || suffix(source, final) == "_el"_el) {
            continue;
        }
        if (isParameterNameArgument(source, literals.getRef(firstIndex))) {
            continue;
        }
        const auto before = source.slice(el::ByteRange{start.retreated(el::ByteCount{256}), start}, true).trimmed();
        if (isExplicitContext(before)) {
            continue;
        }
        if (before.contains("static_assert("_el) || before.contains("assert("_el) ||
            before.contains("deprecated("_el)) {
            continue;
        }
        auto next = end;
        while (
            next.isWithin(source.masked().length()) &&
            source.slice(el::ByteRange{next, next.incremented()}, true).trimmed().isEmpty()) {
            ++next;
        }
        if (!next.isWithin(source.masked().length())) {
            continue;
        }
        const auto character = source.slice(el::ByteRange{next, next.incremented()}, true);
        if (character == ";"_el || character == ")"_el || character == "}"_el) {
            candidates.append(el::ByteRange{start, source.statementEnd(next)});
        }
    }
}

}
