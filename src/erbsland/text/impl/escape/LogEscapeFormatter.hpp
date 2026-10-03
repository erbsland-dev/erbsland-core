// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeFormatter.hpp"

namespace erbsland::text::impl {

/// Escape unsafe characters for logs while preserving line feeds.
/// @tested{StringEscapingTest}
class LogEscapeFormatter final : public EscapeFormatter {
public: // implements EscapeFormatter
    [[nodiscard]] auto needsEscape(Char character, EscapeAmount amount) const noexcept -> bool override;
    void escape(Char character, AnyStringBuilder &builder) const override;
    [[nodiscard]] auto escapeSize(Char character, StringKind stringKind) const noexcept -> std::size_t override;

public:
    /// Get the shared log-escape formatter instance.
    [[nodiscard]] static auto instance() noexcept -> const EscapeFormatterPtr &;
};

}
