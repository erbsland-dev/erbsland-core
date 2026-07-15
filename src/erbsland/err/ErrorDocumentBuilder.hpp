// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../i18n/DisplayTextMap_fwd.hpp"
#include "../text/StringView.hpp"
#include "../text/TextDocument.hpp"
#include "../text/TextNode_fwd.hpp"
#include "../unit/CodeLocation.hpp"

namespace erbsland::err {

/// Builds consistently styled error documents.
/// @tested{ErrorDocumentBuilderTest}
class ErrorDocumentBuilder final {
public:
    /// Create a builder and add the initial title and optional description.
    explicit ErrorDocumentBuilder(
        text::StringView title,
        text::StringView description = {},
        const i18n::DisplayTextMapConstPtr &displayText = {});

    // defaults/deletions
    ~ErrorDocumentBuilder() = default;
    ErrorDocumentBuilder(const ErrorDocumentBuilder &) = delete;
    ErrorDocumentBuilder(ErrorDocumentBuilder &&) noexcept = default;
    auto operator=(const ErrorDocumentBuilder &) -> ErrorDocumentBuilder & = delete;
    auto operator=(ErrorDocumentBuilder &&) noexcept -> ErrorDocumentBuilder & = default;

public:
    /// Add a consistently styled diagnostic section heading.
    auto addSection(text::StringView title) -> text::TextNodePtr;
    /// Add available source name, path, and location fields.
    void addSource(const text::StringView &sourceName, const text::StringView &sourcePath, unit::CodeLocation location);
    /// Transfer the completed document out of this builder.
    [[nodiscard]] auto takeDocument() -> text::TextDocument;

public: // accessors
    /// Access the document root for domain-specific additions.
    [[nodiscard]] auto root() const noexcept -> text::TextNodePtr { return _document.root(); }
    /// Access the resolved display-text map.
    [[nodiscard]] auto displayText() const noexcept -> const i18n::DisplayTextMapConstPtr & { return _displayText; }

private:
    void addSourceField(const text::StringView &label, text::StringView value, const text::StringView &style);

private:
    text::TextDocument _document;
    i18n::DisplayTextMapConstPtr _displayText;
};

}
