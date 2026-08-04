// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IdnaData.hpp"

#include "../PunycodeOptions.hpp"

#include "../../String.hpp"

#include <span>
#include <utility>
#include <vector>

namespace erbsland::text::punycode::impl {

/// Implement strict RFC 5890--5893 label and domain processing.
/// @notest{Tested through PunycodeEncoder and PunycodeDecoder.}
class IdnaProcessor final {
private:
    /// One decoded code point and its cached IDNA attributes.
    struct CodePoint final {
        char32_t value;            ///< The Unicode code point.
        IdnaAttributes attributes; ///< Its combined IDNA attributes.
    };
    /// One canonical Unicode label prepared for validation and encoding.
    struct PreparedLabel final {
        String text;                       ///< The canonical Unicode label.
        std::vector<CodePoint> codePoints; ///< Its code points and cached attributes.
    };

public:
    /// Create a processor for strict IDNA options.
    explicit IdnaProcessor(PunycodeOptions options) : _options{std::move(options)} {}

public:
    /// Convert one label or domain to canonical ASCII.
    [[nodiscard]] auto toAscii(const String &text) const -> String;
    /// Convert one ASCII label or domain to canonical Unicode.
    [[nodiscard]] auto toUnicode(const String &text) const -> String;

private:
    /// Fold ASCII uppercase and normalize to NFC, rejecting prohibited mapping input.
    [[nodiscard]] static auto prepareUnicode(const String &text) -> String;
    /// Split a label or domain according to the configured mode.
    [[nodiscard]] auto split(const String &text) const -> std::vector<String>;
    /// Prepare one Unicode or A-label input for canonical processing.
    [[nodiscard]] auto prepareLabel(const String &label) const -> PreparedLabel;
    /// Prepare one ASCII A-label or NR-LDH label for decoding.
    [[nodiscard]] auto prepareAsciiLabel(const String &label) const -> PreparedLabel;
    /// Decode and prepare one A-label.
    [[nodiscard]] auto decodeALabel(const String &label) const -> PreparedLabel;
    /// Read and validate one canonical Unicode label, excluding its domain-wide bidi rule.
    [[nodiscard]] auto validateUnicodeLabel(const String &label) const -> PreparedLabel;
    /// Encode one validated Unicode label.
    [[nodiscard]] auto encodeLabel(const PreparedLabel &label) const -> String;
    /// Test whether a collection of labels requires domain-wide bidi validation.
    [[nodiscard]] static auto requiresBidi(const std::vector<PreparedLabel> &labels) noexcept -> bool;
    /// Validate all RFC 5892 contextual rules.
    static void validateContext(std::span<const CodePoint> codePoints, std::size_t index, IdnaStatus status);
    /// Validate the complete RFC 5893 bidi rule.
    static void validateBidi(std::span<const CodePoint> codePoints, bool force);
    /// Validate DNS label and domain lengths.
    void validateAsciiLength(const String &text, bool label) const;
    /// Join processed labels using the ASCII full stop.
    [[nodiscard]] static auto join(const std::vector<String> &labels) -> String;
    /// Read a canonical label into code points and cache their generated attributes.
    [[nodiscard]] static auto codePoints(const String &label) -> std::vector<CodePoint>;

private:
    PunycodeOptions _options; ///< The strict IDNA processing options.
};

}
