// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SourceIndex.hpp"

#include <erbsland/AsciiCategory.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/cpp/CppTokenizer.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringTransformations.hpp>
#include <erbsland/TruncateMode.hpp>
#include <erbsland/U8StringConstIterator.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace tool::anti_patterns {

using namespace el::text::literals;

SourceIndex::SourceIndex(el::Path relativePath, el::String text) :
    _path{std::move(relativePath)}, _text{std::move(text)} {
    buildLines();
    buildTokens();
    buildScopes();
}

void SourceIndex::buildLines() {
    _lines.append(el::ByteIndex::zero());
    auto position = el::ByteIndex::zero();
    while (true) {
        const auto next = _text.find("\n"_el, position);
        if (next.isNoIndex()) {
            break;
        }
        _lines.append(next.incremented());
        position = next.incremented();
    }
    for (auto number = el::LineIndex::zero(); number.isWithin(std::min(el::LineCount{10}, lineCount())); ++number) {
        if (line(number).contains("this is a generated file"_el, el::Char::compareAsciiFolded)) {
            _generated = true;
            break;
        }
    }
}

void SourceIndex::appendMask(el::StringEditor &output, const el::String &text) {
    auto start = el::ByteIndex::zero();
    while (start.isWithin(text.length())) {
        const auto newline = text.find("\n"_el, start);
        const auto end = newline.isNoIndex() ? el::ByteIndex::end(text.length()) : newline;
        // Each ASCII space replaces one source byte, including bytes of multibyte characters.
        output.append(U' ', el::CpCount::fromSizeT(start.absoluteDistanceTo(end).toSizeT()));
        if (newline.isNoIndex()) {
            break;
        }
        output.append(U'\n');
        start = end.incremented();
    }
}

void SourceIndex::recordDirective(const el::cpp::CppToken &token, el::List<std::pair<el::ByteIndex, bool>> &stack) {
    auto directive = token.text().trimmed();
    if (!directive.startsWith("#"_el)) {
        return;
    }
    directive = directive.slice(el::StringSide::Back, el::ByteIndex{1}).trimmed();
    if (directive.startsWith("ifdef "_el) || directive.startsWith("ifndef "_el) || directive.startsWith("if "_el) ||
        directive.startsWith("if("_el)) {
        stack.append({token.position(), directive.contains("ERBSLAND_OS"_el)});
    } else if (directive.startsWith("endif"_el) && !stack.isEmpty()) {
        const auto [start, isOs] = stack.takeLast();
        if (isOs) {
            _osRanges.append(el::ByteRange{start, token.range().endIndex()});
        }
    }
}

void SourceIndex::buildTokens() {
    auto output = el::StringEditor{};
    auto previous = el::ByteIndex::zero();
    auto conditionals = el::List<std::pair<el::ByteIndex, bool>>{};
    auto tokenizer = el::cpp::CppTokenizer{_text};
    while (true) {
        auto token = tokenizer.next();
        if (token.isEnd()) {
            break;
        }
        const auto start = token.position();
        const auto end = token.range().endIndex();
        output.append(slice(el::ByteRange{previous, start}));
        if (token.kind() == el::cpp::CppToken::Kind::Comment || token.kind() == el::cpp::CppToken::Kind::Literal ||
            token.kind() == el::cpp::CppToken::Kind::Directive) {
            appendMask(output, token.text());
        } else {
            output.append(token.text());
        }
        const auto index = el::ItemIndex::end(_tokens.count());
        if (token.kind() == el::cpp::CppToken::Kind::Comment) {
            _comments.append(index);
        } else if (token.kind() == el::cpp::CppToken::Kind::Literal) {
            _literals.append(index);
        } else if (token.kind() == el::cpp::CppToken::Kind::Directive) {
            recordDirective(token, conditionals);
        } else {
            _code.append(index);
        }
        _tokens.append(std::move(token));
        previous = end;
    }
    output.append(slice(el::ByteRange{previous, el::ByteIndex::end(_text.length())}));
    _masked = el::String{output};
    for (const auto &[start, isOs] : conditionals) {
        if (isOs) {
            _osRanges.append(el::ByteRange{start, el::ByteIndex::end(_text.length())});
        }
    }
    _osRanges.sort();
}

void SourceIndex::classify(SourceBlock &block) {
    const auto header = slice(el::ByteRange{block.headerStart, block.openingBrace()}, true).trimmed();
    static const auto cNamespace =
        el::RegEx::compile(R"((?s)\bnamespace(?:\s+([A-Za-z_]\w*(?:\s*::\s*\w+)*))?\s*$)"_el);
    static const auto cEnum = el::RegEx::compile(R"(\benum\s+class\s+([A-Za-z_]\w*)[^;{}]*$)"_el);
    static const auto cType = el::RegEx::compile(R"(\b(class|struct)\s+([A-Za-z_]\w*)[^;{}]*$)"_el);
    if (const auto match = cEnum->findFirst(header)) {
        block.kind = SourceBlock::Kind::EnumClass;
        block.name = match->content(1);
    } else if (const auto match = cType->findFirst(header)) {
        block.kind = match->content(1) == "class"_el ? SourceBlock::Kind::Class : SourceBlock::Kind::Struct;
        block.name = match->content(2);
    } else if (const auto match = cNamespace->findFirst(header)) {
        block.kind = SourceBlock::Kind::Namespace;
        static const auto cWhitespace = el::CharSet::from(el::AsciiCategory::Whitespace);
        block.name = match->content(1).replacedAll(cWhitespace, el::String{});
    }
}

void SourceIndex::buildScopes() {
    auto stack = el::List<el::ItemIndex>{};
    auto previousDelimiter = el::ByteIndex::zero();
    for (const auto &token : _tokens) {
        if (token.kind() != el::cpp::CppToken::Kind::Punctuation) {
            continue;
        }
        const auto &value = token.text();
        const auto position = token.position();
        if (value == "{"_el) {
            auto block = SourceBlock{};
            block.braces = el::ByteRange{position, el::ByteIndex::end(_text.length()).incremented()};
            block.headerStart = previousDelimiter;
            block.parent = stack.isEmpty() ? el::ItemIndex::noIndex() : stack.last();
            block.kind = SourceBlock::Kind::Other;
            classify(block);
            stack.append(el::ItemIndex::end(_blocks.count()));
            _blocks.append(std::move(block));
            previousDelimiter = position.incremented();
        } else if (value == "}"_el) {
            if (!stack.isEmpty()) {
                const auto index = stack.takeLast();
                auto block = _blocks.get(index);
                block.braces = el::ByteRange{block.openingBrace(), position.incremented()};
                _blocks.set(index, std::move(block));
            }
            previousDelimiter = position.incremented();
        } else if (value == ";"_el) {
            previousDelimiter = position.incremented();
        }
    }
}

auto SourceIndex::lineCount() const noexcept -> el::LineCount {
    // Each stored item represents a line start; a trailing newline adds an EOF sentinel.
    auto count = el::LineCount::fromSizeT(_lines.count().toSizeT());
    if (!_text.isEmpty() && _text.endsWith("\n"_el)) {
        --count;
    }
    return count;
}

auto SourceIndex::lineIndex(const el::ByteIndex index) const -> el::LineIndex {
    const auto after = std::upper_bound(_lines.begin(), _lines.end(), index);
    // Convert the native iterator distance at the binary-search boundary only.
    return el::LineIndex::fromSizeT(static_cast<std::size_t>(after - _lines.begin() - 1));
}

auto SourceIndex::lineStart(const el::LineIndex index) const -> el::ByteIndex {
    // The line-start list stores exactly one item per physical line index.
    return _lines.get(el::ItemIndex::fromInteger(index.toRawValue()), el::ByteIndex::end(_text.length()));
}

auto SourceIndex::lineEnd(const el::LineIndex index) const -> el::ByteIndex {
    const auto next = el::ItemIndex::fromInteger(index.incremented().toRawValue());
    if (!next.isWithin(_lines.count())) {
        return el::ByteIndex::end(_text.length());
    }
    return _lines.get(next).decremented();
}

auto SourceIndex::line(const el::LineIndex index) const -> el::String {
    return slice(el::ByteRange{lineStart(index), lineEnd(index)});
}

auto SourceIndex::slice(const el::ByteRange range, const bool masked) const -> el::String {
    const auto &source = masked ? _masked : _text;
    return source.slice(range.clampedTo(source.length()));
}

auto SourceIndex::statementEnd(const el::ByteIndex start) const -> el::ByteIndex {
    const auto semicolon = _masked.find(";"_el, start);
    const auto opening = _masked.find("{"_el, start);
    auto end = el::ByteIndex::end(_masked.length());
    if (semicolon.isNoIndex() && opening.isNoIndex()) {
        return lineEnd(lineIndex(start));
    }
    if (!semicolon.isNoIndex()) {
        end = semicolon.incremented();
    }
    if (!opening.isNoIndex()) {
        end = std::min(end, opening.incremented());
    }
    if (lineIndex(end) > lineIndex(start) + el::LineCount{80}) {
        return lineEnd(lineIndex(start));
    }
    return end;
}

auto SourceIndex::hasUnclosedDelimiterBefore(
    const el::ByteIndex offset, const el::String opening, const el::String closing) const -> bool {
    auto depth = el::ItemCount{};
    for (const auto tokenIndex : _code) {
        const auto &token = _tokens.getRef(tokenIndex);
        if (token.position() >= offset) {
            break;
        }
        if (token.text() == opening) {
            ++depth;
        } else if (token.text() == closing && !depth.isZero()) {
            --depth;
        }
    }
    return !depth.isZero();
}

auto SourceIndex::enclosingBlock(const el::ByteIndex offset) const -> el::ItemIndex {
    auto best = el::ItemIndex::noIndex();
    _blocks.forEach([&](const SourceBlock &block, const el::ItemIndex index) -> void {
        if (block.openingBrace() < offset && offset < block.closingBrace() &&
            (best.isNoIndex() || block.openingBrace() > _blocks.getRef(best).openingBrace())) {
            best = index;
        }
    });
    return best;
}

auto SourceIndex::parent(const SourceBlock &block) const -> const SourceBlock * {
    return block.parent.isNoIndex() ? nullptr : &_blocks.getRef(block.parent);
}

auto SourceIndex::namespaceScope(const el::ByteIndex offset) const -> bool {
    auto current = enclosingBlock(offset);
    while (current.isValid()) {
        const auto &block = _blocks.getRef(current);
        if (block.kind != SourceBlock::Kind::Namespace) {
            return false;
        }
        current = block.parent;
    }
    return true;
}

auto SourceIndex::insideOsConditional(const el::ByteIndex offset) const -> bool {
    return std::any_of(
        _osRanges.begin(), _osRanges.end(), [&](const auto &range) -> bool { return range.contains(offset); });
}

auto SourceIndex::typeStart(const SourceBlock &block) const -> el::ByteIndex {
    auto keyword = "struct"_el;
    if (block.kind == SourceBlock::Kind::Class) {
        keyword = "class"_el;
    } else if (block.kind == SourceBlock::Kind::EnumClass) {
        keyword = "enum"_el;
    }
    auto start = block.headerStart;
    for (const auto tokenIndex : _code) {
        const auto &token = _tokens.getRef(tokenIndex);
        const auto position = token.position();
        if (position >= block.openingBrace()) {
            break;
        }
        if (position >= block.headerStart && token.text() == keyword) {
            start = position;
        }
    }
    return start;
}

auto SourceIndex::inlineReason(const el::String &rule, const el::ByteRange range) const -> el::String {
    const auto start = range.beginIndex();
    const auto end = range.endIndex();
    const auto parse = [&](const el::cpp::CppToken &token) -> el::String {
        if (!token.text().startsWith("//"_el)) {
            return {};
        }
        const auto comment = token.text().slice(el::StringSide::Back, el::ByteIndex{2}).trimmed();
        if (!comment.startsWith("anti-pattern: allow "_el)) {
            return {};
        }
        const auto remainder = comment.slice(el::StringSide::Back, el::ByteIndex{20});
        const auto separator = remainder.find(" -- "_el);
        if (separator.isNoIndex() || remainder.slice(el::StringSide::Front, separator) != rule) {
            return {};
        }
        return remainder.slice(el::StringSide::Back, separator + el::ByteCount{4}).trimmed();
    };
    auto previous = lineIndex(start);
    while (!previous.isZero()) {
        --previous;
        const auto lineBegin = lineStart(previous);
        const auto lineFinish = lineEnd(previous);
        const el::cpp::CppToken *comment = nullptr;
        for (const auto index : _comments) {
            const auto &candidate = _tokens.getRef(index);
            if (candidate.position() >= lineBegin && candidate.position() <= lineFinish) {
                comment = &candidate;
                break;
            }
        }
        if (comment == nullptr || !slice(el::ByteRange{lineBegin, comment->position()}).trimmed().isEmpty()) {
            break;
        }
        if (auto reason = parse(*comment); !reason.isEmpty()) {
            return reason;
        }
    }
    const auto endLine = lineIndex(std::max(start, end.decremented()));
    for (const auto index : _comments) {
        const auto &comment = _tokens.getRef(index);
        if (lineIndex(comment.position()) == endLine && comment.position() >= end) {
            if (auto reason = parse(comment); !reason.isEmpty()) {
                return reason;
            }
        }
    }
    return {};
}

auto SourceIndex::snippet(const el::ByteRange range) const -> el::String {
    return el::toAsciiWhitespaceSimplified(slice(range)).truncated(el::CpCount{96}, el::TruncateMode::End, "…"_el);
}

}
