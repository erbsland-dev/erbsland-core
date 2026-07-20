// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParserContext.hpp"
#include "ParserSettings.hpp"

#include "../value/DocumentBuilder.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

/// The parser implementation to hide details from the API.
/// This class is the main abstraction of the parser process with two main functions. First, it hides the
/// implementation from the `Parser` interface. Second, the context creates the token decoder stack used
/// to parse nested documents.
/// About the const reference to `ParserSettings`: an instance of this structure is created as a local variable
/// in `conf::Parser::parse()`. Therefore, the reference stored here is always valid for the lifetime of this object.
/// @tested{ParserBasicTest ParserIncludeTest}
class Parser {
public:
    Parser(SourcePtr documentSource, const ParserSettings &settings);

    ~Parser() = default;

    // prevent copy and assign.
    Parser(const Parser &) = delete;
    auto operator=(const Parser &) -> Parser & = delete;

public:
    /// Parse the document and return the resulting value tree.
    /// @return The root value of the parsed document.
    auto parse() -> DocumentPtr;

private:
    /// Test if there is more context for processing.
    [[nodiscard]] auto hasMoreContent() const -> bool;

    /// Access the current context.
    [[nodiscard]] auto currentContext() -> ParserContext &;

    /// Access the current context.
    [[nodiscard]] auto currentContext() const -> const ParserContext &;

    /// Initialize the current context if required.
    void initializeCurrentContext();

    /// Test if there is a next token.
    [[nodiscard]] auto hasNext() const -> bool;

    /// Get the next token.
    [[nodiscard]] auto nextAssignment() -> Assignment;

    /// Process an assignment
    void processAssignment(const Assignment &assignment);

    void processMetaValue(const Assignment &assignment);

    void addSourceContext(
        const std::size_t includeLevel,
        const SourcePtr &source,
        const SourceIdentifierPtr &parentSourceIdentifier,
        const Location &location);

    /// The source identifier for the current context.
    [[nodiscard]] auto sourceIdentifier() const -> SourceIdentifierPtr;

    /// The signature text for the current context.
    [[nodiscard]] auto signatureText() const -> text::String;

    /// The digest text for the current context.
    [[nodiscard]] auto digestText() const -> text::String;

    /// Process the signature before leaving the context.
    void preLeaveProcessing();

    /// Leave the current context.
    void leaveContext();

private:
    DocumentBuilder _builder;         ///< The document builder.
    ParserContextStack _contextStack; ///< The context stack.
    const ParserSettings &_settings;  ///< The parser settings.
};

}
