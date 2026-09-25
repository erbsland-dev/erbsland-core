// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../decoder/TokenDecoder.hpp"

#include "../../../text/StringEditor.hpp"

namespace erbsland::conf::impl::placeholder {

/// Parses and expands one placeholder inside an ELCL text value.
/// @tested{ParserPlaceholderTest}
class PlaceholderTextParser final {
public:
    /// Create a parser positioned on the placeholder's dollar character.
    PlaceholderTextParser(TokenDecoder &decoder, text::StringEditor &target) noexcept;

public:
    /// Parse, resolve, and append one placeholder.
    void parse();

private:
    /// One parsed source or filter part.
    struct Part final {
        text::String name;      ///< Normalized source or filter name.
        text::String parameter; ///< Decoded parameter text.
    };

    /// Parse one source or filter part and leave on its literal separator or closing brace.
    [[nodiscard]] auto parsePart() -> Part;
    /// Parse content up to a literal separator or closing brace.
    [[nodiscard]] auto parseContent(bool isName) -> text::String;

private:
    TokenDecoder &_decoder;
    text::StringEditor &_target;
};

}
