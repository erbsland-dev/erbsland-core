// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ItemIndex.hpp>
#include <erbsland/List.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>

namespace tool::cleanup {

/// Fill the project's namespace and free-function spacing gaps in clang-format.
class CppSpacing final {
public: // cleanup
    /// Insert missing blank lines around namespace blocks and between source function definitions.
    /// Comments, literals, preprocessor continuations, and nested class bodies are lexically distinguished.
    /// @param text Complete C++ source.
    /// @param functions Whether namespace-scope function spacing is enabled (.cpp files).
    /// @return Source with missing structural blank lines inserted.
    [[nodiscard]] static auto apply(const el::String &text, bool functions) -> el::String;

private:
    /// Syntactic role of a brace scope.
    enum class Role { Namespace, Function, Other };
    /// Open scope tracked by its source line.
    struct Scope {
        Role role;              ///< Syntactic scope role.
        el::ItemIndex openLine; ///< Physical line containing the opening brace.
    };
    /// Summary of the declaration currently being scanned.
    struct Declaration {
        bool hasNamespace{};   ///< A namespace keyword was seen.
        bool hasType{};        ///< A class, struct or enum keyword was seen outside template parameters.
        bool hasEquals{};      ///< A namespace alias or initializer was seen.
        bool hasParenthesis{}; ///< A parameter list may have ended.
        bool hasArrow{};       ///< A trailing return type was seen.
        int parentheses{};     ///< Parenthesis nesting outside other scopes.
        int templateDepth{};   ///< Template parameter nesting.
        el::ItemIndex namespaceLine{el::ItemIndex::zero()}; ///< Line containing the namespace keyword.
    };

    /// Bind one source and allocate scan state only after the cheap candidate pass.
    CppSpacing(const el::String &text, bool functions);
    /// Test column-zero candidates before paying for tokenization.
    [[nodiscard]] auto needsScan() const -> bool;
    /// Tokenize once and record the structural spacing edits.
    void scan();
    /// Update a namespace-level declaration using a significant token.
    void declaration(const el::String &value, el::ItemIndex line);
    /// Process one significant token with its physical source line.
    void consume(const el::String &value, el::ItemIndex line);
    /// Record a new namespace, function or other brace scope.
    void openScope(el::ItemIndex line, bool namespaceScope);
    /// Close one scope and mark its required spacing.
    void closeScope(el::ItemIndex line);
    /// Build the output by inserting only the recorded missing blank lines.
    [[nodiscard]] auto render() const -> el::String;

private:
    el::StringList _lines;        ///< Shared physical source lines.
    bool _functions;              ///< Enable spacing between namespace-scope functions.
    el::List<Scope> _scopes;      ///< Open scopes, innermost last.
    el::List<bool> _before;       ///< Lines that require a blank before them.
    el::List<bool> _after;        ///< Lines that require a blank after them.
    Declaration _declaration;     ///< Current namespace-level declaration.
    el::String _previous;         ///< Previous significant token.
    bool _formatDisabled = false; ///< Inside a clang-format-off region.
};

}
