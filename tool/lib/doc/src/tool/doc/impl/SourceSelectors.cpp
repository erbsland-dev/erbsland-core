// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SourceSelectors.hpp"

#include <erbsland/AnyString.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/AsciiCategory.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/cpp/CppTokenizer.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemOffset.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringCharReader.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSet.hpp>
#include <erbsland/U8StringConstIterator.hpp>
#include <tool/cleanup/CleanupText.hpp>

#include <algorithm>
#include <vector>

namespace tool::doc::impl {

using namespace el::text::literals;

SourceSelectors::SourceSelectors(const el::String &source) : _lines{cleanup::text::lines(source)} {
    auto masked = el::StringEditor{};
    auto offset = el::ByteIndex{0};
    for (const auto &token : el::cpp::CppTokenizer::tokens(source)) {
        masked.append(source.slice(el::ByteRange{offset, offset.absoluteDistanceTo(token.position())}));
        if (token.kind() == el::cpp::CppToken::Kind::Literal || token.kind() == el::cpp::CppToken::Kind::Comment) {
            for (const auto character : token.text()) {
                if (character == U'\n') {
                    masked.append(character);
                } else {
                    masked.append(U' ');
                }
            }
        } else {
            masked.append(token.text());
        }
        offset = token.range().endIndex();
    }
    masked.append(source.slice(el::ByteRange::fromBegin(offset)));
    _code = cleanup::text::lines(masked);
    for (const auto &lineText : _lines) {
        auto comment = el::String{};
        for (const auto &token : el::cpp::CppTokenizer::tokens(lineText)) {
            if (token.kind() == el::cpp::CppToken::Kind::Comment && token.text().startsWith("//"_el)) {
                comment = token.text().slice(el::ByteRange::fromBegin(el::ByteIndex{2}));
                break;
            }
        }
        _comments.append(comment);
    }
}

auto SourceSelectors::blockEnd(const el::ItemIndex start) const -> el::ItemIndex {
    auto depth = 0;
    auto opened = false;
    for (auto index = start; index.isWithin(_code.count()); ++index) {
        for (const auto character : _code.getRef(index)) {
            if (character == U'{') {
                ++depth;
                opened = true;
            } else if (character == U'}') {
                --depth;
            }
        }
        if (opened && depth == 0) {
            return index;
        }
    }
    throwError("Selected block has no closing brace."_el);
}

auto SourceSelectors::endpoint(const el::String &kind, el::String name) const -> el::ItemRange {
    static const auto reQuotedName = el::RegEx::compile(R"re(^["'](?<name>.*)["']$)re"_el);
    static const auto reFunctionName = el::RegEx::compile(R"re((?x)
        ^ \s* (?:auto|void) \s+    # Supported function declaration
        (?<name> [A-Za-z_]\w*           # Initial identifier
            (?:::[A-Za-z_]\w*)* )      # Optional qualification
        \s* \(                        # Start of parameter list
    )re"_el);
    static const auto reAssignment = el::RegEx::compile(R"re((?<name>[A-Za-z_]\w*)\s*=(?:[^=]|$))re"_el);

    name = name.trimmed();
    if (const auto quoted = reQuotedName->match(name)) {
        name = quoted->content("name"_el)
                   .replacedAll(R"(\')"_el, "'"_el)
                   .replacedAll(R"(\")"_el, R"(")"_el)
                   .replacedAll(R"(\\)"_el, R"(\)"_el);
    }
    auto starts = el::List<el::ItemIndex>{};
    for (auto index = el::ItemIndex{0}; index.isWithin(_lines.count()); ++index) {
        auto selected = false;
        if (kind == "c"_el) {
            selected = _comments.getRef(index).contains(name);
        } else if (kind == "f"_el) {
            const auto found = reFunctionName->match(_code.getRef(index));
            selected = found && found->content("name"_el) == name;
        } else {
            const auto found = reAssignment->findFirst(_code.getRef(index));
            selected = found && found->content("name"_el) == name;
        }
        if (selected) {
            starts.append(index);
        }
    }
    if (starts.count() != el::ItemCount{1}) {
        throwError(el::join("Missing or ambiguous selector: "_el, kind, ":"_el, name));
    }
    const auto first = starts.first();
    if (kind == "c"_el) {
        return {first, el::ItemCount{1}};
    }
    if (kind == "f"_el) {
        return {first, blockEnd(first) + el::ItemCount{1}};
    }
    for (auto index = first; index.isWithin(_code.count()); ++index) {
        if (_code.getRef(index).contains(";"_el)) {
            return {first, index + el::ItemCount{1}};
        }
    }
    throwError("Selected assignment has no semicolon."_el);
}

auto SourceSelectors::splitSelectors(const el::String &expression) -> el::StringList {
    auto parts = el::StringList{};
    auto reader = el::StringCharReader{expression};
    reader.startCapture();
    auto quote = el::Char{};
    auto depth = 0;
    while (!reader.isAtEnd()) {
        const auto character = reader.peek();
        if (quote != U'\0') {
            if (character == U'\\') {
                reader.advance();
            } else if (character == quote) {
                quote = el::Char{};
            }
        } else if (character == U'\'' || character == U'"') {
            quote = character;
        } else if (character == U'(') {
            ++depth;
        } else if (character == U')') {
            --depth;
        } else if (character == U',' && depth == 0) {
            parts.append(reader.takeCapture().toString());
            reader.advance();
            reader.startCapture();
            continue;
        }
        if (depth < 0) {
            throwError("Unexpected ')' in selector."_el);
        }
        reader.advance();
    }
    if (quote != U'\0' || depth != 0) {
        throwError("Unclosed quote or offset in selector."_el);
    }
    parts.append(reader.takeCapture().toString());
    return parts;
}

auto SourceSelectors::ranges(const el::String &expression) const -> el::List<el::ItemRange> {
    auto result = el::List<el::ItemRange>{};
    static const auto reSelector = el::RegEx::compile(
        R"re((?x)
        ^ \s* (?<kind>[fac]) :                    # First endpoint: function, assignment, or comment
        (?<name>
            "(?:\\.|[^"\\])*"                     # Double-quoted name with escapes
          | '(?:\\.|[^'\\])*'                     # Single-quoted name with escapes
          | [^,()]+?                              # Unquoted name, stopping before range or offsets
        )
        (?: \s* - \s* (?<lastKind>[fac]) :        # Optional inclusive range endpoint
            (?<lastName>
                "(?:\\.|[^"\\])*"                 # Double-quoted endpoint name
              | '(?:\\.|[^'\\])*'                 # Single-quoted endpoint name
              | [^,()]+?                          # Unquoted endpoint name
            )
        )?
        (?: \( \s* (?<front>[-+][0-9]+) \s*       # Signed start offset
            (?: , \s* (?<back>[-+][0-9]+) \s* )?  # Optional separate end offset
            \)
        )?
        \s* $                                     # No trailing selector text
    )re"_el);
    for (const auto &item : splitSelectors(expression)) {
        const auto found = reSelector->fullMatch(item);
        if (!found) {
            throwError(el::join("Invalid selector: "_el, item));
        }
        auto range = endpoint(found->content("kind"_el), found->content("name"_el));
        if (!found->content("lastKind"_el).isEmpty()) {
            const auto last = endpoint(found->content("lastKind"_el), found->content("lastName"_el));
            if (last.beginIndex() < range.beginIndex()) {
                throwError("Reversed selector range."_el);
            }
            range = {range.beginIndex(), last.endIndex()};
        }
        const auto front = found->content("front"_el).toInteger<int>(0);
        auto back = front;
        if (!found->content("back"_el).isEmpty()) {
            back = found->content("back"_el).toIntegerOrThrow<int>();
        }
        const auto frontOffset = el::ItemOffset::fromInteger(-static_cast<std::int64_t>(front));
        const auto backOffset = el::ItemOffset::fromInteger(back);
        if (!range.beginIndex().canMove(frontOffset, _lines.count()) ||
            !range.lastIndex().canMove(backOffset, _lines.count())) {
            throwError("Selector offset leaves the source or empties the selection."_el);
        }
        const auto first = range.beginIndex().moved(frontOffset);
        const auto last = range.lastIndex().moved(backOffset);
        if (last < first) {
            throwError("Selector offset leaves the source or empties the selection."_el);
        }
        result.append({first, last + el::ItemCount{1}});
    }
    return result;
}

auto SourceSelectors::defaultRange() const -> el::ItemRange {
    auto first = el::ItemIndex{0};
    while (first.isWithin(_lines.count()) && !_lines.getRef(first).trimmed().startsWith("///"_el)) {
        ++first;
    }
    if (first == el::ItemIndex::end(_lines.count())) {
        throwError("Demo source has no /// documentation comment."_el);
    }
    auto end = el::ItemIndex::end(_lines.count());
    for (auto index = el::ItemIndex{0}; index < first; ++index) {
        if (_lines.getRef(index).trimmed() == "namespace demo {"_el) {
            end = blockEnd(index);
        }
    }
    return {first, end};
}

auto SourceSelectors::select(const el::String &show, const el::String &hide) const -> el::StringList {
    static const auto indentationCharacters = el::CharSet::from(el::AsciiCategory::Whitespace);
    auto selected = el::List<el::ItemRange>{};
    if (show.isEmpty()) {
        selected.append(defaultRange());
    } else {
        selected = ranges(show);
    }
    auto excluded = std::vector<bool>(_lines.count().toSizeT(), false);
    if (!hide.isEmpty()) {
        for (const auto range : ranges(hide)) {
            for (auto index = range.beginIndex(); index < range.endIndex(); ++index) {
                excluded[index.toSizeT()] = true;
            }
        }
    }
    auto seen = std::vector<bool>(_lines.count().toSizeT(), false);
    auto result = el::StringList{};
    for (const auto range : selected) {
        auto indent = el::ByteCount::infinite();
        for (auto index = range.beginIndex(); index < range.endIndex(); ++index) {
            const auto prefixEnd = _lines.getRef(index).findFirstNotOf(indentationCharacters);
            if (prefixEnd.isNoIndex()) {
                continue;
            }
            const auto prefix = el::ByteIndex{0}.absoluteDistanceTo(prefixEnd);
            indent = std::min(indent, prefix);
        }
        auto gap = !result.isEmpty();
        for (auto index = range.beginIndex(); index < range.endIndex(); ++index) {
            if (seen[index.toSizeT()] || excluded[index.toSizeT()]) {
                gap = !result.isEmpty();
                seen[index.toSizeT()] = true;
                continue;
            }
            seen[index.toSizeT()] = true;
            const auto value = _lines.getRef(index).slice(el::ByteRange::fromBegin(el::ByteIndex::end(indent)));
            if (value.trimmed().isEmpty() && (result.isEmpty() || gap)) {
                continue;
            }
            if (gap && !result.isEmpty() && !result.last().isEmpty()) {
                result.append(""_el);
            }
            result.append(value);
            gap = false;
        }
    }
    while (!result.isEmpty() && result.last().trimmed().isEmpty()) {
        result = result.prefix(result.count() - el::ItemCount{1});
    }
    return result;
}

void SourceSelectors::throwError(const el::String &description, const el::String &path) {
    throw el::ApplicationError{"Invalid Demo Source Selector"_el, description}.withSourcePath(path);
}

}
