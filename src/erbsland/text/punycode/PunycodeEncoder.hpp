// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PunycodeOptions.hpp"

#include "../String.hpp"

#include <optional>

namespace erbsland::text::punycode {

/// Encode Unicode text using RFC 3492 Punycode or strict IDNA2008.
/// @seedoc{/reference/text/encoding}
/// @tested{PunycodeTest IdnaTest}
class PunycodeEncoder final {
public:
    /// Create an encoder.
    /// @param text The Unicode input text.
    /// @param options The processing options.
    explicit PunycodeEncoder(String text, PunycodeOptions options = {});

public: // conversion
    /// Encode the text, returning no value for invalid input.
    [[nodiscard]] auto encode() const -> std::optional<String>;
    /// Encode the text or throw a detailed parse error.
    /// @return The ASCII Punycode payload, label, or domain.
    /// @throws err::ParseError If the input violates Punycode or selected IDNA2008 rules.
    [[nodiscard]] auto encodeOrThrow() const -> String;

public: // accessors
    /// Get the processing options.
    [[nodiscard]] auto options() const noexcept -> const PunycodeOptions & { return _options; }

private:
    String _text;             ///< The input text.
    PunycodeOptions _options; ///< The processing options.
};

}
