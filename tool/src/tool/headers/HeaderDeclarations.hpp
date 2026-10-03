// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HeaderDeclaration.hpp"

#include <erbsland/cpp/CppTokenizer.hpp>
#include <erbsland/StringSet.hpp>

#include <vector>

namespace tool::headers {

/// Inspect namespace boundaries and simple type declarations in portable C++ headers.
class HeaderDeclarations final {
public: // scanning
    /// Scan a header, ignoring comments, literals, preprocessor lines, and nested type bodies.
    /// @param text Header contents.
    explicit HeaderDeclarations(const el::String &text);
    /// Get declared namespace names.
    [[nodiscard]] auto namespaces() const -> const el::StringSet & { return _namespaces; }
    /// Get namespace-scope type declarations.
    [[nodiscard]] auto declarations() const -> const std::vector<HeaderDeclaration> & { return _declarations; }

private:
    /// Advance to the next declaration-relevant token, skipping comments, literals and directives.
    void advance();
    /// Track namespace and type scopes while consuming the token stream.
    void scan();
    /// Read a namespace name while leaving its opening brace or declaration terminator pending.
    auto readNamespaceName(const el::String &sourceNamespace, bool &anonymous) -> el::String;
    /// Consume the template parameter list, including nested closing-angle tokens.
    void skipTemplateParameters();
    /// Consume a namespace-scope type declaration, retaining safe forward declarations.
    void record(const el::String &sourceNamespace, bool isTemplate);

private:
    el::cpp::CppTokenizer _tokenizer;             ///< Streaming lexical scanner over shared input storage.
    el::String _token;                            ///< Current significant token; empty at end of input.
    el::StringSet _namespaces;                    ///< Fully qualified namespace blocks found during scanning.
    std::vector<HeaderDeclaration> _declarations; ///< Namespace-scope type declarations in source order.
};

}
