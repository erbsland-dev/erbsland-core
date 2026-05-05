// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeFormatter.hpp"

#include <cstdint>

namespace erbsland::text::impl {

class JsonEscapeFormatter final : public EscapeFormatter {
public:
    [[nodiscard]] auto needsEscape(Char character, EscapeAmount amount) const noexcept -> bool override;
    void escape(Char character, StringBuilder &builder) const override;
    [[nodiscard]] auto escapeSize(Char character, StringKind stringKind) const noexcept -> std::size_t override;

public:
    [[nodiscard]] static auto instance() noexcept -> const EscapeFormatterPtr &;

private:
    [[nodiscard]] static auto unicodeEscapeFormat(std::size_t width) noexcept -> IntegerFormat;
    static void appendUnicodeEscape(StringBuilder &builder, uint32_t codePoint);
};

}
