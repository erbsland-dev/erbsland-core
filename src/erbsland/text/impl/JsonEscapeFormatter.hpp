// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeFormatter.hpp"

#include <cstdint>

namespace erbsland::text::impl {

/// Escape formatter for JSON string syntax.
class JsonEscapeFormatter final : public EscapeFormatter {
public:
    [[nodiscard]] auto needsEscape(Char character, EscapeAmount amount) const noexcept -> bool override;
    void escape(Char character, AnyStringBuilder &builder) const override;
    [[nodiscard]] auto escapeSize(Char character, StringKind stringKind) const noexcept -> std::size_t override;

public:
    /// Access the shared JSON escape formatter.
    [[nodiscard]] static auto instance() noexcept -> const EscapeFormatterPtr &;

private:
    /// Create the integer format used for a Unicode escape width.
    [[nodiscard]] static auto unicodeEscapeFormat(std::size_t width) noexcept -> IntegerFormat;
    /// Append a Unicode escape for one code point.
    static void appendUnicodeEscape(AnyStringBuilder &builder, uint32_t codePoint);
};

}
