// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CppSpacing.hpp"

#include "CleanupText.hpp"

#include <erbsland/cpp/CppTokenizer.hpp>

#include <algorithm>

namespace tool::cleanup {

using namespace el::text::literals;

CppSpacing::CppSpacing(const el::String &source, const bool functions) :
    _lines{text::lines(source)}, _functions{functions} {
}

auto CppSpacing::apply(const el::String &source, const bool functions) -> el::String {
    auto scanner = CppSpacing{source, functions};
    if (!scanner.needsScan()) {
        return source;
    }
    scanner.scan();
    return scanner.render();
}

auto CppSpacing::needsScan() const -> bool {
    // Broad, cheap candidates avoid tokenizing already correctly spaced files.
    for (auto line = el::ItemIndex::zero(); line.isWithin(_lines.count()); ++line) {
        const auto &value = _lines.getRef(line);
        const auto isNamespace = value.startsWith("namespace "_el) || value.startsWith("inline namespace "_el);
        const auto isClose = value == "}"_el || value.startsWith("} //"_el);
        if (!isNamespace && !isClose) {
            continue;
        }
        const auto missingBefore = !line.isZero() && !_lines.getRef(line.decremented()).trimmed().isEmpty();
        const auto missingAfter =
            line.incremented().isWithin(_lines.count()) && !_lines.getRef(line.incremented()).trimmed().isEmpty();
        if (missingBefore || missingAfter) {
            return true;
        }
    }
    return false;
}

void CppSpacing::scan() {
    _before.resize(_lines.count(), false);
    _after.resize(_lines.count(), false);
    auto line = el::ItemIndex::zero();
    auto lineEnd = el::ByteIndex::end(_lines.first().length()).incremented();
    auto tokenizer = el::cpp::CppTokenizer{text::join(_lines)};
    while (true) {
        const auto token = tokenizer.next();
        if (token.isEnd()) {
            break;
        }
        if (token.kind() == el::cpp::CppToken::Kind::Comment) {
            const auto value = token.text().trimmed();
            if (value == "// clang-format off"_el) {
                _formatDisabled = true;
            } else if (value == "// clang-format on"_el) {
                _formatDisabled = false;
            }
            continue;
        }
        if (token.kind() == el::cpp::CppToken::Kind::Literal || token.kind() == el::cpp::CppToken::Kind::Directive) {
            continue;
        }
        // Token positions advance monotonically, including across multiline comments and literals.
        while (token.position() >= lineEnd && line.incremented().isWithin(_lines.count())) {
            ++line;
            lineEnd += _lines.getRef(line).length() + el::ByteCount::one();
        }
        consume(token.text(), line);
    }
}

void CppSpacing::declaration(const el::String &value, const el::ItemIndex line) {
    auto &state = _declaration;
    if (value == "namespace"_el) {
        state.hasNamespace = true;
        state.namespaceLine = line;
    }
    if (value == "<"_el && (_previous == "template"_el || state.templateDepth > 0)) {
        ++state.templateDepth;
    } else if (value == ">"_el && state.templateDepth > 0) {
        --state.templateDepth;
    } else if (value == ">>"_el && state.templateDepth > 0) {
        state.templateDepth = std::max(0, state.templateDepth - 2);
    }
    if (state.parentheses == 0 && state.templateDepth == 0) {
        state.hasType = state.hasType || value == "class"_el || value == "struct"_el || value == "enum"_el;
        state.hasEquals = state.hasEquals || (value == "="_el && _previous != "operator"_el);
    }
    state.hasParenthesis = state.hasParenthesis || value == ")"_el;
    state.hasArrow = state.hasArrow || value == "->"_el;
    if (value == "("_el) {
        ++state.parentheses;
    } else if (value == ")"_el && state.parentheses > 0) {
        --state.parentheses;
    }
}

void CppSpacing::consume(const el::String &value, const el::ItemIndex line) {
    const auto namespaceScope = _scopes.isEmpty() || _scopes.last().role == Role::Namespace;
    if (namespaceScope) {
        declaration(value, line);
    }
    if (value == "{"_el) {
        openScope(line, namespaceScope);
    } else if (value == "}"_el && !_scopes.isEmpty()) {
        closeScope(line);
    } else if (namespaceScope && value == ";"_el) {
        _declaration = {};
    }
    _previous = value;
}

void CppSpacing::openScope(const el::ItemIndex line, const bool namespaceScope) {
    auto role = Role::Other;
    const auto &state = _declaration;
    if (namespaceScope && state.hasNamespace && !state.hasEquals) {
        role = Role::Namespace;
        auto documentationLine = state.namespaceLine;
        while (
            !documentationLine.isZero() &&
            _lines.getRef(documentationLine.decremented()).trimmed().startsWith("///"_el)) {
            --documentationLine;
        }
        _before.set(documentationLine, !_formatDisabled);
        _after.set(line, !_formatDisabled);
    } else if (
        namespaceScope && _functions && state.hasParenthesis && !state.hasType && !state.hasEquals &&
        state.parentheses == 0) {
        // Braced member initializers end in a member name, unlike a function body.
        if (_previous == ")"_el || _previous == "}"_el || _previous == "const"_el || _previous == "noexcept"_el ||
            _previous == "&"_el || _previous == "&&"_el || state.hasArrow) {
            role = Role::Function;
        }
    }
    _scopes.append({role, line});
    if (role != Role::Other || !namespaceScope) {
        _declaration = {};
    }
}

void CppSpacing::closeScope(const el::ItemIndex line) {
    const auto [role, openLine] = _scopes.last();
    _scopes.removeLast();
    if (role == Role::Namespace) {
        if (line != openLine) {
            _before.set(line, !_formatDisabled);
            _after.set(line, !_formatDisabled);
        } else {
            // Preserve compact empty namespace declarations.
            _before.set(openLine, false);
            _after.set(openLine, false);
        }
    } else if (role == Role::Function) {
        _after.set(line, !_formatDisabled);
    }
    if (role != Role::Other) {
        _declaration = {};
    }
}

auto CppSpacing::render() const -> el::String {
    auto result = el::StringList{};
    result.reserve(_lines.count());

    for (auto line = el::ItemIndex::zero(); line.isWithin(_lines.count()); ++line) {
        if (_before.get(line) && !result.isEmpty() && !result.last().trimmed().isEmpty()) {
            result.append(el::String{});
        }
        result.append(_lines.getRef(line));
        if (_after.get(line) && line.incremented().isWithin(_lines.count()) &&
            !_lines.getRef(line.incremented()).trimmed().isEmpty()) {
            result.append(el::String{});
        }
    }
    return text::join(result);
}

}
