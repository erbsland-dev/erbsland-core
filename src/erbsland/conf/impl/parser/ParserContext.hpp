// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParserContext_fwd.hpp"

#include "../assignment/AssignmentStream.hpp"
#include "../decoder/TokenDecoder.hpp"
#include "../lexer/Lexer.hpp"

#include "../../Source.hpp"

#include <vector>

namespace erbsland::conf::impl {

/// Parsing context for a single document source.
/// @needtest{Covered indirectly by parser integration tests.}
class ParserContext final {
    class PrivateTag {};

public:
    /// Construct a new parsing context.
    /// @param includeLevel The include level for this source.
    /// @param source Source from which tokens are read.
    explicit ParserContext(std::size_t includeLevel, SourcePtr source, PrivateTag /*pt*/) noexcept;

    /// Create a new context instance.
    /// @param includeLevel The include level for this source.
    /// @param source Source from which tokens are read.
    /// @return Shared-pointer to the new context.
    [[nodiscard]] static auto create(std::size_t includeLevel, SourcePtr source) -> ParserContextPtr;

    // defaults
    ~ParserContext() = default;

public:
    /// Test if this context was initialized.
    [[nodiscard]] auto isInitialized() const noexcept -> bool { return _initialized; }

    /// Initialize this context.
    void initialize();

    /// Check if more assignments are available.
    [[nodiscard]] auto hasNext() const noexcept -> bool { return _hasNextAssignment; }

    /// Retrieve the next assignment.
    /// @throws ConfError For any problems while parsing a document.
    /// @throws err::LogicError If the assignment stream violates its terminal-marker contract.
    [[nodiscard]] auto nextAssignment() -> Assignment;

    /// Set the signature text for this context.
    void setSignatureText(text::String signatureText) { _signatureText = std::move(signatureText); }

    /// Get the signature text assigned to this context.
    [[nodiscard]] auto signatureText() const -> text::String { return _signatureText; }

    /// Get the document digest produced by the lexer.
    [[nodiscard]] auto digest() const -> mem::ByteBlock { return _lexer->digest(); }

    /// Get the include level of this source.
    [[nodiscard]] auto includeLevel() const noexcept -> uint8_t { return _includeLevel; }

    /// Identifier of the source currently processed.
    [[nodiscard]] auto sourceIdentifier() const -> SourceIdentifierPtr { return _source->identifier(); }

    /// Get a best-effort excerpt from this context's source.
    [[nodiscard]] auto codeSnippet(unit::CodeLocation location) -> std::optional<text::CodeSnippet> {
        return _source != nullptr ? _source->codeSnippet(location) : std::nullopt;
    }

    /// Set the include location for this context.
    void setIncludeLocation(Location includeLocation) { _includeLocation = std::move(includeLocation); }

    /// Access the include location of this context.
    [[nodiscard]] auto includeLocation() const -> const Location & { return _includeLocation; }

    /// Set the parent source identifier.
    void setParentSourceIdentifier(SourceIdentifierPtr parentSourceIdentifier) {
        _parentSourceIdentifier = std::move(parentSourceIdentifier);
    }

    /// Access the parent source identifier.
    [[nodiscard]] auto parentSourceIdentifier() const -> const SourceIdentifierPtr & { return _parentSourceIdentifier; }

    /// Close this context.
    /// Explicit call to avoid exceptions in destruction.
    void close();

private:
    bool _initialized = false; ///< Flag indicating if the context has been initialized.
    uint8_t _includeLevel;     ///< The include level for this context.
    SourcePtr _source;         ///< The source for this context, as reference to detect inclusion loops.
    SourceIdentifierPtr _parentSourceIdentifier; ///< The identifier of the parent source.
    Location _includeLocation;                   ///< The location of the include directive.
    LexerPtr _lexer;                             ///< The lexer instance.
    AssignmentStreamPtr _assignmentStream;       ///< The assignment stream.
    AssignmentGenerator _assignmentGenerator;    ///< The assignment generator.
    bool _hasNextAssignment{false};              ///< If another assignment can be read.
    text::String _signatureText;                 ///< The signature text, if any.
};

}
