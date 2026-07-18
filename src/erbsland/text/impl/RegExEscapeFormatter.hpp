// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeFormatter.hpp"

namespace erbsland::text::impl {

class RegExEscapeFormatter final : public EscapeFormatter {
public:
    [[nodiscard]] auto needsEscape(Char character, EscapeAmount amount) const noexcept -> bool override;
    void escape(Char character, AnyStringBuilder &builder) const override;
    [[nodiscard]] auto escapeSize(Char character, StringKind stringKind) const noexcept -> std::size_t override;

public:
    [[nodiscard]] static auto instance() noexcept -> const EscapeFormatterPtr &;
};

}
