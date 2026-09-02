// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PunycodeOptions.hpp"

#include "../String.hpp"

#include <optional>

namespace erbsland::text::punycode {

/// Decode RFC 3492 Punycode or strict IDNA2008 text into UTF-8.
/// @seedoc{/reference/text/encoding}
/// @tested{PunycodeTest IdnaTest}
class PunycodeDecoder final {
public:
    /// Create a decoder.
    /// @param text The ASCII input text.
    /// @param options The processing options.
    explicit PunycodeDecoder(String text, PunycodeOptions options = {});

public: // conversion
    /// Decode the text, returning no value for invalid input.
    [[nodiscard]] auto decode() const -> std::optional<String>;
    /// Decode the text or throw a detailed parse error.
    /// @return The decoded UTF-8 text.
    /// @throws err::ParseError If the input violates Punycode or selected IDNA2008 rules.
    [[nodiscard]] auto decodeOrThrow() const -> String;

public: // accessors
    /// Get the processing options.
    [[nodiscard]] auto options() const noexcept -> const PunycodeOptions & { return _options; }

private:
    String _text;             ///< The input text.
    PunycodeOptions _options; ///< The processing options.
};

}
