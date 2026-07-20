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
public:
    /// Create a new name lexer using the given text reference.
    /// @param text A reference to the text to parse.
    explicit NameLexer(text::String text) noexcept : _decoder{std::move(text)} {}

    // defaults
    ~NameLexer() = default;
    NameLexer(const NameLexer &) = delete;
    auto operator=(const NameLexer &) -> NameLexer & = delete;

public:
    void initialize();

    [[nodiscard]] auto hasNext() const noexcept -> bool { return !_decoder.character().isEndOfData(); }

    [[nodiscard]] auto next() -> Name;

private:
    void expectNameSeparatorOrEnd();

    void expectNameSeparatorIndexOrEnd();

    [[nodiscard]] auto expectGenericIndex() -> std::size_t;

    [[nodiscard]] auto expectRegularName() -> Name;

    [[nodiscard]] auto expectTextNameOrIndex() -> Name;

    [[nodiscard]] auto expectIndex() -> Name;

    void skipSpacing();

private:
    bool _afterFirstElement = false;
    FastNameDecoder _decoder;
};

}
