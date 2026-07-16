// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlainTextRenderer_fwd.hpp"
#include "StringBuilder.hpp"
#include "TextDocument_fwd.hpp"

#include "impl/PlainTextRenderer_fwd.hpp"

#include <memory>

namespace erbsland::text {

/// Render a text document as plain UTF-8 text.
/// @tested{TextDocumentTest}
class PlainTextRenderer final {
public:
    /// Create a plain-text renderer for a document.
    /// @param document The document to render.
    explicit PlainTextRenderer(const TextDocument &document);

    // defaults
    ~PlainTextRenderer();
    PlainTextRenderer(const PlainTextRenderer &) = delete;
    PlainTextRenderer(PlainTextRenderer &&) noexcept = default;
    auto operator=(const PlainTextRenderer &) -> PlainTextRenderer & = delete;
    auto operator=(PlainTextRenderer &&) noexcept -> PlainTextRenderer & = default;

public:
    /// Build a new UTF-8 string from the document.
    /// @return The rendered plain text string.
    [[nodiscard]] auto build() -> String;
    /// Append the rendered plain text to an existing builder.
    /// @param builder The builder to append to.
    /// @return The same builder.
    auto appendTo(StringBuilder &builder) -> StringBuilder &;

private:
    std::unique_ptr<impl::PlainTextRenderer> _impl; ///< The renderer implementation.
};

}
