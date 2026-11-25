// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DoxygenIndex.hpp"

#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/xml/XmlDocument.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

auto DoxygenIndex::childText(const el::data::xml::XmlNodePtr &node, const el::String &name) -> el::String {
    for (const auto &child : node->children()) {
        if (child->name() == name) {
            return child->textContentOrThrow();
        }
    }
    return {};
}

void DoxygenIndex::add(DoxygenSymbol symbol) {
    if (!symbol.name.startsWith("erbsland::"_el) || symbol.name.contains("::impl::"_el)) {
        return;
    }
    auto key = symbol.name;
    while (!key.isEmpty()) {
        auto candidates = _symbols.get(key, {});
        if (symbol.role == "class"_el || symbol.role == "struct"_el) {
            candidates.removeIf([&](const DoxygenSymbol &entry) -> bool {
                return entry.name == symbol.name && entry.role == "func"_el;
            });
        }
        if (symbol.role == "func"_el &&
            !candidates
                .findFirstIf([&](const DoxygenSymbol &entry) -> bool {
                    return entry.name == symbol.name && (entry.role == "class"_el || entry.role == "struct"_el);
                })
                .isNoIndex()) {
            return;
        }
        const auto existing = candidates.findFirstIf(
            [&](const DoxygenSymbol &entry) -> bool { return entry.role == symbol.role && entry.name == symbol.name; });
        if (existing.isNoIndex()) {
            candidates.append(symbol);
        }
        _symbols.set(key, candidates);
        const auto separator = key.splitAtFirst("::"_el);
        if (!separator || separator->first.isEmpty() || separator->second.isEmpty()) {
            break;
        }
        key = separator->second;
    }
}

void DoxygenIndex::read(const frame::ProjectFiles &files, const el::Path &path) {
    _symbols.clear();
    const auto document = el::data::xml::XmlDocument::fromStringOrThrow(files.readText(path));
    for (const auto &compound : document.root()->children()) {
        if (compound->name() != "compound"_el) {
            continue;
        }
        const auto name = childText(compound, "name"_el);
        const auto kind = compound->getAttribute("kind"_el).value_or(el::String{});
        if (kind == "class"_el || kind == "struct"_el || kind == "concept"_el) {
            add({kind, name});
        }
        for (const auto &member : compound->children()) {
            if (member->name() != "member"_el) {
                continue;
            }
            auto role = member->getAttribute("kind"_el).value_or(el::String{});
            if (role == "function"_el) {
                role = "func"_el;
            } else if (role == "typedef"_el) {
                role = "type"_el;
            } else if (role != "enum"_el) {
                continue;
            }
            const auto memberName = childText(member, "name"_el);
            if (memberName.isEmpty() || memberName.startsWith("~"_el)) {
                continue;
            }
            auto shortName = name;
            if (const auto separator = name.splitAtLast("::"_el)) {
                shortName = separator->second;
            }
            if ((kind == "class"_el || kind == "struct"_el) && memberName == shortName) {
                continue;
            }
            add({role, el::join(name, "::"_el, memberName)});
        }
    }
}

auto DoxygenIndex::link(const el::String &text, el::StringList &warnings) const -> el::String {
    static const auto reExplicitTitle = el::RegEx::compile(R"re(^(?<display>.+?)\s+<(?<target>[^<>]+)>$)re"_el);
    static const auto reQualifiedName = el::RegEx::compile(R"re((?x)
        ^ [A-Za-z_]\w*                                   # Initial C++ identifier
        (?: :: [A-Za-z0-9_+*/%<>=!&|\^~\[\](),"\-]+ )*   # Qualified names, including operators
        $                                                # Complete reference target
    )re"_el);

    static const auto reCppReference = el::RegEx::compile(
        R"re((?x)
        (?<before>^|[^`])                                # Preserve preceding text; exclude double backticks
        (?<prefix> :cpp:                                 # Optional explicit role
            (?:any|type|class|struct|enum|func|concept)
            :
        )?
        ` (?<body>[^`\n]+) `                             # Single-backtick reference contents
    )re"_el);
    return reCppReference->replaceAll(text, [&](const el::re::MatchPtr &found) -> el::String {
        const auto before = found->content("before"_el);
        if (before == ":"_el && found->content("prefix"_el).isEmpty()) {
            return found->content();
        }
        auto display = found->content("body"_el);
        auto query = display;
        if (const auto explicitTitle = reExplicitTitle->match(display)) {
            display = explicitTitle->content("display"_el);
            query = explicitTitle->content("target"_el);
        }
        if (query.startsWith("::"_el)) {
            query = query.slice(el::StringSide::Back, el::ByteIndex{2});
        } else if (query.startsWith("~"_el)) {
            query = query.slice(el::StringSide::Back, el::ByteIndex{1});
        }
        if (query.endsWith("()"_el)) {
            query = query.slice(el::StringSide::Front, el::ByteIndex::end(query.length() - el::ByteCount{2}));
        }
        if (query.startsWith("el::"_el)) {
            query = el::join("erbsland::"_el, query.slice(el::StringSide::Back, el::ByteIndex{4}));
        }
        if (!reQualifiedName->match(query)) {
            return found->content();
        }
        const auto candidates = _symbols.get(query, {});
        if (candidates.count() == el::ItemCount{1}) {
            const auto &symbol = candidates.first();
            return el::join(before, ":cpp:"_el, symbol.role, ":`"_el, display, " <"_el, symbol.name, ">`"_el);
        }
        warnings.append(el::join("Unresolved or ambiguous C++ reference: "_el, query));
        return el::join(before, "``"_el, display, "``"_el);
    });
}

}
