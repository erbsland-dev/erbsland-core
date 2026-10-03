// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NamePathPattern.hpp"

#include "../../../err/ParseError.hpp"
#include "../../../text/impl/ComponentGlobMatcher.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringTransformations.hpp"

namespace erbsland::conf::syntax::impl {

using namespace text;
using namespace err;
using namespace unit;

using namespace text::literals;
NamePathPattern::NamePathPattern(const String &pattern) {
    auto part = StringEditor{};
    bool quoted = false;
    bool escaped = false;
    bool characterSet = false;
    auto index = ByteIndex{};
    while (index.isWithin(pattern.length())) {
        const auto ch = pattern.readCharAndAdvance(index);
        if (escaped) {
            part.append(ch);
            escaped = false;
            continue;
        }
        if (ch == U'\\') {
            part.append(ch);
            escaped = true;
            continue;
        }
        if (!quoted && ch == U'[') {
            characterSet = true;
        }
        if (!quoted && ch == U']') {
            characterSet = false;
        }
        if (!characterSet && ch == U'"') {
            quoted = !quoted;
        }
        if (ch == U'.' && !quoted && !characterSet) {
            append(part);
            part.clear();
        } else {
            part.append(ch);
        }
    }
    if (quoted || escaped) {
        throw ParseError{"Unterminated name-path pattern."_el};
    }
    append(part);
}

void NamePathPattern::append(String part) {
    part = part.trimmed();
    if (part.isEmpty()) {
        throw ParseError{"Empty name-path pattern component."_el};
    }
    auto component = Component{};
    if (part == "**"_el) {
        component.recursive = true;
        _specificity[0] = 1;
        --_specificity[3];
    } else {
        bool literal = !part.contains("*"_el) && !part.contains("?"_el) && !part.contains("["_el);
        if (part.startsWith("\""_el)) {
            auto name = NamePath::fromText(part);
            if (name.size() != 1 || name.front().type() != NameType::Text) {
                throw ParseError{"Invalid quoted pattern name."_el};
            }
            // Quoted components are exact names, including literal glob characters.
            part = name.front()
                       .asText()
                       .replacedAll("\\"_el, "\\\\"_el)
                       .replacedAll("*"_el, "\\*"_el)
                       .replacedAll("?"_el, "\\?"_el)
                       .replacedAll("["_el, "\\["_el)
                       .replacedAll("]"_el, "\\]"_el);
            component.type = NameType::Text;
            literal = true;
        } else {
            if (part.contains("**"_el)) {
                throw ParseError{"Recursive wildcard must occupy an entire component."_el};
            }
            part = text::toIdentifierNormalized(part);
            if (literal) {
                component.type = NameType::Regular;
            }
        }
        if (literal) {
            ++_specificity[1];
        } else {
            _specificity[0] = 1;
        }
        auto index = ByteIndex{};
        while (index.isWithin(part.length())) {
            const auto ch = part.readCharAndAdvance(index);
            if (ch == U'*' || ch == U'?') {
                --_specificity[4];
            } else {
                ++_specificity[2];
            }
        }
        component.pattern = StringPattern::fromGlob(part);
    }
    _components.push_back(std::move(component));
}

auto NamePathPattern::Component::matches(const Name &name) const -> bool {
    return (!type || *type == name.type()) && pattern && pattern->matches(name.asText());
}

auto NamePathPattern::matches(const NamePath &path) const -> bool {
    auto matcher = text::impl::ComponentGlobMatcher{};
    matcher.start(_components);
    for (const auto &name : path) {
        matcher.consume(_components, name);
    }
    return matcher.accepts();
}

}
