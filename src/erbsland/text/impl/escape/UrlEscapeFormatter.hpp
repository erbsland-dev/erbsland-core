// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeFormatter.hpp"

#include "../../CharSet.hpp"

namespace erbsland::text::impl {

/// Percent-encode decoded URL data as UTF-8 octets according to RFC 3986.
/// @seedoc{/reference/text/formatting_and_parsing}
/// @tested{StringEscapingTest UrlTest}
class UrlEscapeFormatter final : public EscapeFormatter {
public:
    /// Select the URL component's allowed ASCII characters.
    /// @param allowedCharacters Characters that may remain unescaped.
    explicit UrlEscapeFormatter(const CharSet &allowedCharacters) noexcept : _allowedCharacters{allowedCharacters} {}

public: // implement EscapeFormatter
    [[nodiscard]] auto needsEscape(Char character, EscapeAmount amount) const noexcept -> bool override;
    void escape(Char character, AnyStringBuilder &builder) const override;
    [[nodiscard]] auto escapeSize(Char character, StringKind stringKind) const noexcept -> std::size_t override;

public: // shared instances
    /// Get the formatter for data within a URL component.
    /// @return The shared URL data formatter.
    [[nodiscard]] static auto instance() noexcept -> const EscapeFormatterPtr &;
    /// Get the formatter for a complete decoded URL path.
    /// @return The shared URL-path formatter.
    [[nodiscard]] static auto pathInstance() noexcept -> const EscapeFormatterPtr &;
    /// Get the formatter for a complete decoded URL query or fragment.
    /// @return The shared URL-query formatter.
    [[nodiscard]] static auto queryInstance() noexcept -> const EscapeFormatterPtr &;
    /// Get the formatter for a decoded username or password field.
    /// @return The shared credential-field formatter.
    [[nodiscard]] static auto userInfoInstance() noexcept -> const EscapeFormatterPtr &;

private:
    CharSet _allowedCharacters; ///< The component's allowed ASCII characters.
};

}
