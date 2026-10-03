// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RstCleaner.hpp"

#include "../../cleanup/CleanupText.hpp"

#include <erbsland/AsciiCategory.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/UnicodeCategory.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

RstCleaner::RstCleaner(const DoxygenIndex &index, const int lineWidth) : _index{index}, _lineWidth{lineWidth} {
}

auto RstCleaner::adornment(const el::String &text) -> bool {
    if (text.characterLength() < el::CpCount{3}) {
        return false;
    }
    const auto first = text.charAt(el::StringSide::Front);
    static const auto cAdornmentCharacters = el::CharSet{U'*', U'=', U'~', U'^', U'-'};
    return cAdornmentCharacters.contains(first) && text.containsOnly(el::CharSet{first});
}

auto RstCleaner::paragraph(const el::StringList &lines, const el::ItemIndex index) -> bool {
    static const auto reParagraphMarker = el::RegEx::compile(R"re((?x)
        ^ [\s|+:]                   # Indentation, table, or field marker
        | ^ (?: \.\. | ::           # Directives and literal blocks
              | [-*+]\s             # Bullet item
              | [0-9]+\.\s          # Numbered item
              | \#\.\s )            # Automatically numbered item
    )re"_el);

    const auto &text = lines.getRef(index);
    if (text.isEmpty() || reParagraphMarker->findFirst(text) || adornment(text)) {
        return false;
    }
    if (!index.isZero() && adornment(lines.getRef(index - el::ItemCount{1}))) {
        return false;
    }
    if ((index + el::ItemCount{1}).isWithin(lines.count()) && adornment(lines.getRef(index + el::ItemCount{1}))) {
        return false;
    }
    return true;
}

auto RstCleaner::wrap(const el::String &text) const -> el::StringList {
    static const auto cWordDelimiters = el::CharSet{U'"', U'\'', U'(', U')', U'[', U']', U'{', U'}'};
    static const auto reSentenceAbbreviation = el::RegEx::compile(R"re((?x)
        (?i) ^ (?: ca|cf|ch|chap|e\.g|etc|fig|figs|i\.e            # Common abbreviations
                  | max|min|no|nos|ref|refs|sec|secs|std|vs ) \.$  # More abbreviations
        | ^ (?:[A-Za-z]\.)+ $                                      # Initials
        | [0-9] \.$                                                # Numeric suffix
    )re"_el);

    static const auto reInlineToken = el::RegEx::compile(
        R"re((?x)
        (?:
            :[A-Za-z][-A-Za-z0-9_]*        # Initial interpreted-text role
            (?::[A-Za-z][-A-Za-z0-9_]*)*   # Optional domain qualification
            :`[^`]+`                       # Role contents
          | ``[^`]*``                      # Inline literal contents
          | \S+                            # Other non-whitespace token
        )
        [,.;:]*                            # Attached punctuation
    )re"_el);
    auto result = el::StringList{};
    auto current = el::StringEditor{};
    for (const auto &found : reInlineToken->findAll(text)) {
        const auto token = found->content();
        if (token.characterLength().toSizeT() > static_cast<std::size_t>(_lineWidth)) {
            return el::StringList{text};
        }
        if (!current.isEmpty() &&
            el::String{current}.characterLength() + token.characterLength() + el::CpCount{1} >
                el::CpCount::fromInteger(_lineWidth)) {
            result.append(current);
            current.clear();
        }
        if (!current.isEmpty()) {
            current.append(U' ');
        }
        current.append(token);
        const auto word = token.trimmed(cWordDelimiters);
        if (word.endsWith("."_el) && !reSentenceAbbreviation->findFirst(word)) {
            result.append(current);
            current.clear();
        }
    }
    if (!current.isEmpty()) {
        result.append(current);
    }
    return result;
}

auto RstCleaner::appendHeading(const el::StringList &lines, el::ItemIndex &index, el::StringList &result) -> bool {
    const auto &current = lines.getRef(index);
    if (adornment(current) && (index + el::ItemCount{2}).isWithin(lines.count()) &&
        !lines.getRef(index + el::ItemCount{1}).isEmpty() && adornment(lines.getRef(index + el::ItemCount{2}))) {
        const auto symbol = current.charAt(el::StringSide::Front);
        const auto rule =
            el::String::fromCharacter(symbol, lines.getRef(index + el::ItemCount{1}).trimmed().characterLength());
        result.append(rule).append(lines.getRef(index + el::ItemCount{1})).append(rule);
        index += el::ItemCount{3};
        return true;
    }
    if ((index + el::ItemCount{1}).isWithin(lines.count()) && !current.isEmpty() &&
        !(current.charAt(el::StringSide::Front) == U'\t' ||
            current.charAt(el::StringSide::Front).category() == el::UnicodeCategory::SpaceSeparator) &&
        adornment(lines.getRef(index + el::ItemCount{1}))) {
        result.append(current).append(
            el::String::fromCharacter(
                lines.getRef(index + el::ItemCount{1}).charAt(el::StringSide::Front),
                current.trimmed().characterLength()));
        index += el::ItemCount{2};
        return true;
    }
    return false;
}

auto RstCleaner::preserveLine(Protection &state, const el::String &current) -> bool {
    static const auto reCodeDirective = el::RegEx::compile(R"re((?x)
        ^ \s* \.\. \s+                                # Indented RST directive
        (?:code|code-block|sourcecode|erbsland-ansi)  # Protected source/output directives
        ::                                            # End of directive name
    )re"_el);

    static const auto blanks = el::CharSet::from(el::AsciiCategory::Blank);
    if (current.trimmed().startsWith(".. erbsland-demo::"_el)) {
        state.demo = true;
    }
    if (state.demo) {
        if (current.trimmed() == ".. erbsland-demo-end::"_el) {
            state.demo = false;
        }
        return true;
    }
    const auto firstText = current.findFirstNotOf(blanks);
    auto indentation = current.length();
    if (!firstText.isNoIndex()) {
        indentation = el::ByteIndex{0}.absoluteDistanceTo(firstText);
    }
    if (state.code && (firstText.isNoIndex() || indentation > state.indent)) {
        return true;
    }
    state.code = false;
    if (reCodeDirective->match(current) || (current.endsWith("::"_el) && !current.trimmed().startsWith(".."_el))) {
        state.code = true;
        state.indent = indentation;
        return true;
    }
    return false;
}

auto RstCleaner::process(const el::String &text, el::StringList &warnings) const -> el::String {
    auto lines = cleanup::text::lines(text);
    auto result = el::StringList{};
    auto protection = Protection{};
    for (auto index = el::ItemIndex{0}; index.isWithin(lines.count());) {
        const auto current = lines.getRef(index);
        if (preserveLine(protection, current)) {
            result.append(current);
            ++index;
            continue;
        }
        if (appendHeading(lines, index, result)) {
            continue;
        }
        if (paragraph(lines, index)) {
            auto parts = el::StringList{};
            while (index.isWithin(lines.count()) && paragraph(lines, index)) {
                parts.append(_index.link(lines.getRef(index), warnings).trimmed());
                ++index;
            }
            for (const auto &wrapped : wrap(parts.join(" "_el))) {
                result.append(wrapped);
            }
        } else {
            result.append(_index.link(current, warnings));
            ++index;
        }
    }
    auto output = result.join("\n"_el);
    if (text.isEmpty() || text.endsWith("\n"_el)) {
        output = el::join(output, "\n"_el);
    }
    return output;
}

}
