// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../char/NamedChars.hpp"
#include "../decoder/FastNameDecoder.hpp"

#include "../../Name.hpp"

#include <cstddef>

namespace erbsland::conf::impl {

/// A minimalistic decoder for decoding names and name paths.
class NameLexer {
    /// Restricts construction of names to this lexer.
    class PrivateTag {};

public:
    /// Create a new name lexer using the given text reference.
    /// @param text A reference to the text to parse.
    explicit NameLexer(text::String text) noexcept : _decoder{std::move(text)} {}

    // defaults
    ~NameLexer() = default;
    NameLexer(const NameLexer &) = delete;
    auto operator=(const NameLexer &) -> NameLexer & = delete;

public:
    /// Initialize parsing state for the next name component.
    void initialize();

    /// Test if another name component is available.
    [[nodiscard]] auto hasNext() const noexcept -> bool { return !_decoder.character().isEndOfData(); }

    /// Parse the next name component.
    [[nodiscard]] auto next() -> Name;

private:
    /// Require a name separator or the end of the input.
    void expectNameSeparatorOrEnd();

    /// Require a name-separator index or the end of the input.
    void expectNameSeparatorIndexOrEnd();

    /// Parse a generic name index.
    [[nodiscard]] auto expectGenericIndex() -> std::size_t;

    /// Parse a regular name component.
    [[nodiscard]] auto expectRegularName() -> Name;

    /// Parse a text-name or index component.
    [[nodiscard]] auto expectTextNameOrIndex() -> Name;

    /// Parse an index component.
    [[nodiscard]] auto expectIndex() -> Name;

    /// Skip spacing between name components.
    void skipSpacing();

private:
    bool _afterFirstElement = false;
    FastNameDecoder _decoder;
};

}
