// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HeaderScanner.hpp"

#include "../../cleanup/CleanupText.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/cpp/CppTokenizer.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/U8StringConstIterator.hpp>

#include <vector>

namespace tool::doc::impl {

using namespace el::text::literals;

auto HeaderScanner::signature(const el::String &text) -> el::String {
    static const auto reOpenParenthesisSpace = el::RegEx::compile(R"re(\(\s+)re"_el);
    static const auto reCloseParenthesisSpace = el::RegEx::compile(R"re(\s+\))re"_el);

    auto result = text.slice(el::StringSide::Front, text.findFirstOf(el::CharSet{U';', U'{'})).trimmed();
    result = reOpenParenthesisSpace->replaceAll(result, "("_el);
    return reCloseParenthesisSpace->replaceAll(result, ")"_el);
}

auto HeaderScanner::braceDeltas(const el::String &source, const el::ItemCount lineCount) -> std::vector<int> {
    // Tokenization counts braces outside comments, quoted literals, and preprocessing directives.
    auto braces = std::vector<int>(lineCount.toSizeT(), 0);
    auto offset = el::ByteIndex{0};
    auto lineIndex = std::size_t{0};
    for (const auto &token : el::cpp::CppTokenizer::tokens(source)) {
        lineIndex +=
            source.slice(el::ByteRange{offset, offset.absoluteDistanceTo(token.position())}).count("\n"_el).toSizeT();
        if (lineIndex < braces.size() && token.kind() == el::cpp::CppToken::Kind::Punctuation) {
            if (token.text() == "{"_el) {
                ++braces[lineIndex];
            } else if (token.text() == "}"_el) {
                --braces[lineIndex];
            }
        }
        lineIndex += token.text().count("\n"_el).toSizeT();
        offset = token.range().endIndex();
    }
    return braces;
}

void HeaderScanner::scanDocumented(ScanState &state, const el::String &current, const el::StringList &scope) {
    static const auto reDeclarationPreamble = el::RegEx::compile(R"re(^\s+|^\[\[[^\]]+\]\]\s*$)re"_el);
    static const auto reReturnPreamble = el::RegEx::compile(R"re((?x)
        ^ (?: \[\[[^\]]+\]\] \s* )*               # Attributes
        (?:(?:inline|constexpr)\s+)*              # Function qualifiers
        (?:auto|void) \s* $                       # Standalone return-type line
    )re"_el);
    static const auto reClassDeclaration = el::RegEx::compile(R"re((?x)
        ^ (?<kind>class|struct) \s+               # Supported declaration kind
        (?<name>[A-Za-z_]\w*) \b                  # Declared identifier
    )re"_el);
    static const auto reEnumDeclaration = el::RegEx::compile(R"re(^enum\s+class\s+(?<name>[A-Za-z_]\w*)\b)re"_el);
    static const auto reAliasDeclaration = el::RegEx::compile(R"re(^using\s+(?<name>[A-Za-z_]\w*)\s*=)re"_el);
    static const auto reConceptDeclaration = el::RegEx::compile(R"re(^concept\s+(?<name>[A-Za-z_]\w*)\b)re"_el);
    static const auto reFunctionDeclaration = el::RegEx::compile(R"re((?x)
        ^ (?: \[\[[^\]]+\]\] \s* )*                      # Attributes
        (?:(?:inline|constexpr)\s+)*                     # Function qualifiers
        (?:auto|void) \s+                                # Supported return type
        (?<name>[A-Za-z_]\w*|operator""_[A-Za-z_]\w*)    # Function or literal operator
        \s* (?<signature>\(.*) $                         # Remaining parameter list and declaration
    )re"_el);
    static const auto reFunctionContinuation = el::RegEx::compile(R"re((?x)
        ^ (?<name>[A-Za-z_]\w*|operator""_[A-Za-z_]\w*)  # Function following a separate return line
        \s* (?<signature>\(.*) $                         # Parameter list and declaration suffix
    )re"_el);

    const auto prefix = el::join(scope.join("::"_el), "::"_el);
    const auto hasTerminator = current.contains(";"_el) || current.contains("{"_el);
    if (current.startsWith("template "_el) || current.startsWith("requires "_el) ||
        reDeclarationPreamble->findFirst(current)) {
        // Keep documentation pending through template constraints and attribute preambles.
    } else if (reReturnPreamble->match(current)) {
        state.functionReturn = true;
    } else {
        el::re::MatchPtr declaration;
        auto kind = el::String{};
        if ((declaration = reClassDeclaration->match(current))) {
            kind = declaration->content("kind"_el);
        } else if ((declaration = reEnumDeclaration->match(current))) {
            kind = "enum"_el;
        } else if ((declaration = reAliasDeclaration->match(current))) {
            kind = "typedef"_el;
        } else if ((declaration = reConceptDeclaration->match(current))) {
            kind = "concept"_el;
        }
        if (!kind.isEmpty() && !scope.isEmpty() && scope.first() != "std"_el) {
            auto directive =
                el::join(".. doxygen"_el, kind, ":: "_el, prefix, declaration->content("name"_el), "\n"_el);
            if (kind == "class"_el || kind == "struct"_el) {
                directive = el::join(directive, "    :members:\n"_el);
            }
            state.result.append(directive);
        } else if (!scope.isEmpty() && scope.first() != "std"_el) {
            auto function = reFunctionDeclaration->match(current);
            if (!function && state.functionReturn) {
                function = reFunctionContinuation->match(current);
            }
            if (function) {
                state.pendingFunction =
                    el::join(prefix, function->content("name"_el), signature(function->content("signature"_el)));
                if (hasTerminator && state.pendingFunction.count("("_el) <= state.pendingFunction.count(")"_el)) {
                    state.result.append(el::join(".. doxygenfunction:: "_el, state.pendingFunction, "\n"_el));
                    state.pendingFunction = {};
                }
            }
        }
        state.documented = false;
        state.functionReturn = false;
    }
}

void HeaderScanner::scanLine(ScanState &state, const el::String &current) {
    static const auto reNamespaceDeclaration = el::RegEx::compile(R"re((?x)
        ^ namespace \s+                           # Namespace declaration
        (?<name>[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)  # Possibly nested namespace name
        \s* \{ \s*                                # Opening body
        (?://.*)? $                               # Optional trailing line comment
    )re"_el);
    if (const auto declared = reNamespaceDeclaration->match(current)) {
        state.namespaces.emplace_back(state.depth + 1, declared->content("name"_el));
    }
    auto scope = el::StringList{};
    for (const auto &[level, name] : state.namespaces) {
        (void)level;
        scope.append(name);
    }
    const auto hasTerminator = current.contains(";"_el) || current.contains("{"_el);
    if (!state.pendingFunction.isEmpty()) {
        state.pendingFunction = signature(el::join(state.pendingFunction, " "_el, current.trimmed()));
        if (hasTerminator && state.pendingFunction.count("("_el) <= state.pendingFunction.count(")"_el)) {
            state.result.append(el::join(".. doxygenfunction:: "_el, state.pendingFunction, "\n"_el));
            state.pendingFunction = {};
        }
    } else if (state.documented) {
        scanDocumented(state, current, scope);
    }
}

auto HeaderScanner::scan(const el::String &source) const -> el::StringList {
    const auto lines = cleanup::text::lines(source);
    const auto braces = braceDeltas(source, lines.count());
    auto state = ScanState{};
    for (auto index = el::ItemIndex{0}; index.isWithin(lines.count()); ++index) {
        const auto &current = lines.getRef(index);
        if (current.startsWith("///"_el)) {
            state.documented = true;
            continue;
        }
        scanLine(state, current);
        state.depth += braces[index.toSizeT()];
        if (state.depth < 0) {
            throwError("Unmatched source brace while scanning API documentation."_el);
        }
        while (!state.namespaces.empty() && state.namespaces.back().first > state.depth) {
            state.namespaces.pop_back();
        }
    }
    if (!state.namespaces.empty()) {
        throwError("Unclosed namespace in API documentation source."_el);
    }
    return state.result;
}

void HeaderScanner::throwError(const el::String &description, const el::String &path) {
    throw el::ApplicationError{"API Header Scan Failed"_el, description}.withSourcePath(path);
}

}
