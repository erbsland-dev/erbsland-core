// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"
#include "../text/TextDocument.hpp"
#include "../text/TextNode_fwd.hpp"
#include "../unit/CodeLocation.hpp"

namespace erbsland::err {

/// Builds consistently styled error documents.
/// @tested{ErrorDocumentBuilderTest}
class ErrorDocumentBuilder final {
public:
    /// Create a builder and add the initial title and optional description.
    explicit ErrorDocumentBuilder(text::String title, text::String description = {});

    // defaults/deletions
    ~ErrorDocumentBuilder() = default;
    ErrorDocumentBuilder(const ErrorDocumentBuilder &) = delete;
    ErrorDocumentBuilder(ErrorDocumentBuilder &&) noexcept = default;
    auto operator=(const ErrorDocumentBuilder &) -> ErrorDocumentBuilder & = delete;
    auto operator=(ErrorDocumentBuilder &&) noexcept -> ErrorDocumentBuilder & = default;

public:
    /// Add a consistently styled diagnostic section heading.
    auto addSection(text::String title) -> text::TextNodePtr;
    /// Add available source name, path, and location fields.
    void addSource(const text::String &sourceName, const text::String &sourcePath, unit::CodeLocation location);
    /// Transfer the completed document out of this builder.
    [[nodiscard]] auto takeDocument() -> text::TextDocument;

public: // accessors
    /// Access the document root for domain-specific additions.
    [[nodiscard]] auto root() const noexcept -> text::TextNodePtr { return _document.root(); }

private:
    /// Add a source-location field to the diagnostic document.
    void addSourceField(const text::String &label, text::String value, const text::String &style);

private:
    text::TextDocument _document;
};

}
