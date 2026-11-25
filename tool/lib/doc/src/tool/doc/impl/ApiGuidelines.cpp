// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApiGuidelines.hpp"

#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/U8StringConstIterator.hpp>
#include <tool/cleanup/CleanupText.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

auto ApiGuidelines::returnType(el::String value) const -> bool {
    static const auto reQualifiedType = el::RegEx::compile(R"re((?x)
        ^ (?:[A-Za-z_❮❯][A-Za-z0-9_❮❯]*::)*  # Optional namespace or placeholder qualification
        [A-Za-z_❮❯][A-Za-z0-9_❮❯]*           # Final type or placeholder name
        $                                    # Complete type
    )re"_el);
    static const auto reTemplateType = el::RegEx::compile(R"re(^([A-Za-z_][A-Za-z0-9_:]*)<(.+)>$)re"_el);

    if (value.startsWith("["_el) && value.endsWith("]"_el) && !value.contains("\n"_el)) {
        value = value.slice(el::ByteRange{el::ByteIndex{1}, value.length() - el::ByteCount{2}});
    }
    if (value.startsWith("const "_el)) {
        value = value.slice(el::StringSide::Back, el::ByteIndex{6});
    }
    if (value.endsWith("&&"_el)) {
        value = value.slice(el::StringSide::Front, el::ByteIndex::end(value.length() - el::ByteCount{2}));
    } else if (value.endsWith("&"_el) || value.endsWith("*"_el)) {
        value = value.slice(el::StringSide::Front, el::ByteIndex::end(value.length() - el::ByteCount{1}));
    }
    if (reQualifiedType->match(value)) {
        return true;
    }
    const auto found = reTemplateType->match(value);
    if (!found) {
        return false;
    }
    auto depth = 0;
    auto item = el::StringEditor{};
    for (const auto character : found->content(el::re::CaptureGroupIndex{2})) {
        if (character == U'<') {
            ++depth;
        }
        if (character == U'>') {
            --depth;
        }
        if (depth < 0) {
            return false;
        }
        if (character == U',' && depth == 0) {
            if (!returnType(el::String{item}.trimmed())) {
                return false;
            }
            item.clear();
        } else {
            item.append(character);
        }
    }
    return depth == 0 && returnType(el::String{item}.trimmed());
}

auto ApiGuidelines::typeList(const el::String &value, const bool allowSlash) const -> bool {
    static const auto reQualifiedClass = el::RegEx::compile(R"re((?x)
        ^ (?:[A-Za-z_❮❯][A-Za-z0-9_❮❯]*::)*     # Optional qualification
        [A-Z❮][A-Za-z0-9_❮❯]*                   # Uppercase type or placeholder name
        $                                       # Complete type
    )re"_el);

    auto separators = el::CharSet{U','};
    if (allowSlash) {
        separators = el::CharSet{U',', U'/'};
    }
    for (const auto &part : el::StringList::fromSplit(value, separators, el::ItemCount::infinite(), true)) {
        if (!reQualifiedClass->match(part.trimmed())) {
            return false;
        }
    }
    return !value.isEmpty();
}

auto ApiGuidelines::callPattern(const el::String &value) const -> bool {
    static const auto reMemberExpression = el::RegEx::compile(R"re((?x)
        ^ [A-Za-z_❮][A-Za-z0-9_❮❯]*    # Member or placeholder name
        (?: <[^<>\n]+> )?              # Optional simple template arguments
        (?: \(.*\) )?                  # Optional call arguments
        $                              # Complete expression
    )re"_el);

    if (!value.endsWith(")"_el)) {
        return false;
    }
    auto depth = 0;
    auto brackets = 0;
    for (const auto character : value) {
        if (character == U'(') {
            ++depth;
        } else if (character == U')') {
            if (--depth < 0) {
                return false;
            }
        } else if (character == U'[') {
            ++brackets;
        } else if (character == U']') {
            if (--brackets < 0) {
                return false;
            }
        } else if (character.isAsciiWhitespace() && depth == 0) {
            return false;
        }
    }
    if (depth != 0 || brackets != 0) {
        return false;
    }
    if (value.startsWith("T("_el)) {
        return true;
    }
    auto remainder = value;
    if (remainder.startsWith("T::"_el)) {
        remainder = remainder.slice(el::StringSide::Back, el::ByteIndex{3});
    } else if (remainder.startsWith("o."_el)) {
        remainder = remainder.slice(el::StringSide::Back, el::ByteIndex{2});
    }
    auto alternatives = el::StringList{};
    auto part = el::StringEditor{};
    depth = 0;
    for (const auto character : remainder) {
        if (character == U'(') {
            ++depth;
        } else if (character == U')') {
            --depth;
        }
        if (character == U'/' && depth == 0) {
            alternatives.append(part);
            part.clear();
        } else {
            part.append(character);
        }
    }
    alternatives.append(part);
    for (const auto &alternative : alternatives) {
        if (!reMemberExpression->match(alternative)) {
            return false;
        }
    }
    return alternatives.last().contains("("_el);
}

auto ApiGuidelines::codeLine(const el::String &text, const el::String &kind) const -> el::String {
    static const auto reDocumentedExpression = el::RegEx::compile(R"re((?x)
        ^ [ ]{4} (?<expression>.+?)        # Four-space code indentation and expression
        [ ]+ // [ ]                        # Literal inline-comment separator
        (?<description>\S.*) $             # Nonempty explanation
    )re"_el);
    static const auto rePlaceholderDefinition = el::RegEx::compile(R"re((?x)
        ^ ( [A-Z][A-Za-z]?                 # First placeholder identifier
            (?: ,[ ] [A-Z][A-Za-z]? )* )   # Further comma-space-separated identifiers
        [ ] = [ ] (.+) $                   # Definition with literal surrounding spaces
    )re"_el);
    static const auto reReturnArrow = el::RegEx::compile(R"re(^(?<call>.*?) -> (?<return>.*)$)re"_el);

    if (text.characterLength() > el::CpCount{120}) {
        return "code-block lines must not exceed 120 characters."_el;
    }
    const auto found = reDocumentedExpression->match(text);
    if (!found || text.startsWith("     "_el)) {
        return "line requires four-space indentation, an expression, and ' // ' description."_el;
    }
    if (found->content("description"_el).endsWith("."_el)) {
        return "description must not end with a period."_el;
    }
    const auto expression = found->content("expression"_el);
    if (kind == "type"_el) {
        if (!typeList(expression, false)) {
            return "invalid comma-separated type list."_el;
        }
    } else if (kind == "definition"_el) {
        const auto definition = rePlaceholderDefinition->match(expression);
        if (!definition || !typeList(definition->content(el::re::CaptureGroupIndex{2}), true)) {
            return "invalid pattern shortcut definition."_el;
        }
    } else {
        const auto arrow = reReturnArrow->match(expression);
        if (arrow && !returnType(arrow->content("return"_el))) {
            return "invalid return type in pattern."_el;
        }
        auto call = expression;
        if (arrow) {
            call = arrow->content("call"_el);
        }
        if (!callPattern(call)) {
            return "invalid constructor, method, or free-function pattern."_el;
        }
    }
    return {};
}

auto ApiGuidelines::validateTitle(const el::StringList &lines, ApiGuidelineState &state) -> bool {
    const auto count = lines.count();
    if (count > el::ItemCount{500}) {
        state.issue(el::ItemIndex{0}, "page must not exceed 500 lines."_el);
    }
    if (count < el::ItemCount{4}) {
        state.issue(el::ItemIndex{0}, "page must start with an overlined title."_el);
        return false;
    }
    const auto titleRule = el::String::fromCharacter(U'*', lines.getRef(el::ItemIndex{1}).characterLength());
    if (lines.getRef(el::ItemIndex{0}) != titleRule || lines.getRef(el::ItemIndex{2}) != titleRule ||
        !lines.getRef(el::ItemIndex{3}).isEmpty()) {
        state.issue(el::ItemIndex{0}, "page title requires matching '*' adornments and a following blank line."_el);
    }
    return true;
}

void ApiGuidelines::openSection(ApiGuidelineState &state, const el::String &current, const el::ItemIndex index) {
    if (!state.sectionKind.isEmpty() && state.sectionKind != "core"_el && state.directives != 1) {
        state.issue(state.sectionStart, "section must contain exactly one '.. code-block:: text' directive."_el);
    }
    if (state.sectionKind == "core"_el && state.sectionStart.absoluteDistanceTo(index) > el::ItemCount{64}) {
        state.issue(state.sectionStart, "Core Semantics must not exceed 60 content lines."_el);
    }
    state.sectionStart = index;
    state.directives = 0;
    state.subsections = 0;
    if (current == "Core Semantics"_el) {
        if (state.phase != 0 || ++state.coreCount > 1) {
            state.issue(index, "Core Semantics must occur once before type sections."_el);
        }
        state.sectionKind = "core"_el;
        state.phase = 1;
    } else if (current.endsWith(" Types"_el)) {
        if (state.phase > 2) {
            state.issue(index, "type sections must precede patterns."_el);
        }
        if (state.typeCount++ == 0 && current != "Primary Types"_el) {
            state.issue(index, "the first type section must be Primary Types."_el);
        }
        state.sectionKind = "type"_el;
        state.phase = 2;
    } else if (current == "Pattern Definitions"_el) {
        if (state.phase != 2 || ++state.definitions > 1) {
            state.issue(index, "Pattern Definitions must follow types and occur at most once."_el);
        }
        state.sectionKind = "definition"_el;
        state.phase = 3;
    } else if (current.endsWith(" Patterns"_el)) {
        if (state.phase < 2) {
            state.issue(index, "pattern sections must follow types."_el);
        }
        state.sectionKind = "pattern"_el;
        state.phase = 4;
    } else {
        state.issue(index, "invalid top-level section."_el);
        state.sectionKind = ""_el;
    }
}

void ApiGuidelines::validateContent(
    ApiGuidelineState &state, const el::StringList &lines, const el::ItemIndex index) const {
    const auto count = lines.count();
    const auto &current = lines.getRef(index);
    if (state.sectionKind == "core"_el) {
        if ((index + el::ItemCount{1}).isWithin(count) && !current.isEmpty() &&
            lines.getRef(index + el::ItemCount{1}) == el::String::fromCharacter(U'-', current.characterLength())) {
            if (++state.subsections > 4) {
                state.issue(index, "Core Semantics allows at most four subsections."_el);
            }
        }
        return;
    }
    if (current.isEmpty()) {
        return;
    }
    if (current == ".. code-block:: text"_el) {
        ++state.directives;
        return;
    }
    if (state.sectionKind.isEmpty()) {
        state.issue(index, "content outside structural sections is not allowed."_el);
        return;
    }
    const auto error = codeLine(current, state.sectionKind);
    if (!error.isEmpty()) {
        state.issue(index, error);
    }
}

auto ApiGuidelines::validate(const el::String &text) const -> el::StringList {
    const auto lines = cleanup::text::lines(text);
    auto state = ApiGuidelineState{};
    if (!validateTitle(lines, state)) {
        return state.issues;
    }
    for (auto index = el::ItemIndex{4}; index.isWithin(lines.count()); ++index) {
        const auto &current = lines.getRef(index);
        const auto heading = (index + el::ItemCount{1}).isWithin(lines.count()) && !current.isEmpty() &&
            lines.getRef(index + el::ItemCount{1}) == el::String::fromCharacter(U'=', current.characterLength());
        if (heading) {
            openSection(state, current, index);
            ++index;
        } else {
            validateContent(state, lines, index);
        }
    }
    if (state.typeCount == 0) {
        state.issue(el::ItemIndex{0}, "at least one type section is required."_el);
    }
    if (!state.sectionKind.isEmpty() && state.sectionKind != "core"_el && state.directives != 1) {
        state.issue(state.sectionStart, "section must contain exactly one code block."_el);
    }
    return state.issues;
}

}
