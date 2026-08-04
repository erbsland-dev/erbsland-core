// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/HtmlParser_fwd.hpp"

#include "../AnyString.hpp"
#include "../TextDocument.hpp"

#include <memory>

namespace erbsland::text::html {

/// A tolerant HTML parser that converts HTML fragments into text documents.
/// @tested{HtmlParserTest}
class HtmlParser final {
public:
    /// Create a parser for the given HTML text.
    /// @param html The HTML fragment or document to parse.
    explicit HtmlParser(AnyString html);

    /// Move parser state into this instance.
    auto operator=(HtmlParser &&) noexcept -> HtmlParser &;

    // defaults/deletions
    HtmlParser(const HtmlParser &) = delete;
    HtmlParser(HtmlParser &&) noexcept;
    ~HtmlParser();

    // defaults/deletions
    auto operator=(const HtmlParser &) -> HtmlParser & = delete;

public:
    /// Parse the HTML text into a document.
    /// @return The parsed text document, or a best-effort document if a parser error is recovered.
    [[nodiscard]] auto parse() noexcept -> TextDocument;
    /// Parse the HTML text into a document.
    /// @return The parsed text document.
    /// @throws err::ParseError If a future unrecoverable parser condition is detected.
    [[nodiscard]] auto parseOrThrow() -> TextDocument;

private:
    std::unique_ptr<impl::HtmlParser> _impl; ///< The parser implementation.
};

}
