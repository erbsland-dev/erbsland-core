// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProgramWriter.hpp"
#include "Tokenizer.hpp"
#include "ValueTest.hpp"

#include "../Value_fwd.hpp"

#include "../../../text/StringList.hpp"
#include "../../../text/StringMap.hpp"

namespace erbsland::text::render::impl {

/// Compile one tokenized layout expression using recursive-descent precedence.
/// @tested{RenderExpressionTest RenderStatementTest RenderFilterTest RenderLanguageCompletionTest}
class ExpressionCompiler final {
public:
    /// Create a compiler for the expression at the tokenizer's current position.
    ExpressionCompiler(
        String layout,
        String origin,
        Tokenizer &tokenizer,
        StringList &constants,
        ProgramWriter &writer,
        const StringMap<FilterFn> &filters,
        bool allowSuper,
        bool allowOutputModifiers = false,
        EscapeFormat outputFormat = EscapeFormat::None);

public:
    /// Compile through, but without consuming, the active tag-end token.
    void compile();
    /// Get the escape format selected for a direct output expression.
    [[nodiscard]] auto outputFormat() const noexcept -> EscapeFormat;

private: // grammar
    /// Parse the `or` precedence level.
    void parseOr();
    /// Parse the `and` precedence level.
    void parseAnd();
    /// Parse the unary `not` precedence level.
    void parseNot();
    /// Parse one optional comparison.
    void parseComparison();
    /// Parse additive arithmetic and string concatenation.
    void parseAdditive();
    /// Parse multiplicative arithmetic.
    void parseMultiplicative();
    /// Parse unary numeric operators.
    void parseUnary();
    /// Parse one value followed by filter pipes.
    void parseFiltered();
    /// Parse one literal, path, or grouped expression.
    void parsePrimary();
    /// Parse a regular name path or a supported super call.
    void parseNameOrSuper();
    /// Parse a list collection literal.
    void parseList();
    /// Parse a string-keyed map collection literal.
    void parseMap();
    /// Parse zero, one, or two ordinary filter arguments.
    [[nodiscard]] auto parseFilterArguments() -> std::size_t;
    /// Parse a reserved output modifier.
    void parseOutputModifier(const String &name, unit::CodeLocation location);

private: // token tools
    /// Get the current lookahead token.
    [[nodiscard]] auto token() const noexcept -> const Token & { return _tokenizer.current(); }
    /// Advance to the next token.
    void nextToken();
    /// Reject an unsupported lexical character at the current token.
    void requireSupportedToken() const;
    /// Test if a token is a supported comparison operator.
    [[nodiscard]] static auto isComparison(TokenKind kind) noexcept -> bool;
    /// Convert a comparison token into its bytecode opcode.
    [[nodiscard]] auto comparisonOpcode(TokenKind kind) const -> Opcode;
    /// Resolve a built-in value-test name.
    [[nodiscard]] static auto valueTest(const String &name) noexcept -> std::optional<ValueTest>;

private: // tools
    /// Append one shared program constant and return its zero-based index.
    [[nodiscard]] auto addConstant(String value) -> std::size_t;
    /// Throw a source-aware expression syntax error.
    [[noreturn]] void throwSyntax(String description, unit::CodeLocation location) const;

private:
    String _layout;                                 ///< Logical layout name.
    String _origin;                                 ///< Diagnostic source origin.
    Tokenizer &_tokenizer;                          ///< Shared complete-layout token stream.
    StringList &_constants;                         ///< Shared program constant pool.
    ProgramWriter &_writer;                         ///< Destination bytecode writer.
    const StringMap<FilterFn> &_applicationFilters; ///< Setup-time application filter registry.
    bool _allowSuper{false};                        ///< Whether the active program is a block body.
    bool _allowOutputModifiers{false};              ///< Whether reserved output modifiers are accepted.
    bool _containsSuper{false};                     ///< Whether the expression contains a super request.
    bool _superTransformed{false};                  ///< Whether an ordinary filter transformed captured super output.
    EscapeFormat _outputFormat;                     ///< Selected direct-output escaping.
};

}
