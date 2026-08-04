// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EscapeFormatter_fwd.hpp"

#include "../AnyStringBuilder.hpp"
#include "../Char.hpp"
#include "../EscapeAmount.hpp"
#include "../EscapeFormat.hpp"

#include <cstddef>

namespace erbsland::text::impl {

/// Defines a strategy for escaping one string format.
class EscapeFormatter {
public:
    // defaults
    virtual ~EscapeFormatter() = default;

public:
    /// Test if a character requires escaping.
    [[nodiscard]] virtual auto needsEscape(Char character, EscapeAmount) const noexcept -> bool = 0;
    /// Append an escaped character to a string builder.
    virtual void escape(Char character, AnyStringBuilder &builder) const = 0;
    /// Get the output size of an escaped character.
    [[nodiscard]] virtual auto escapeSize(Char character, StringKind stringKind) const noexcept -> std::size_t = 0;

protected:
    /// Apply common escape policy based on the escape amount.
    [[nodiscard]] static auto needsEscapeByAmount(
        Char character, EscapeAmount amount, bool required, bool balanced) noexcept -> bool;

public:
    /// Get the formatter for an escape format.
    [[nodiscard]] static auto forFormat(EscapeFormat format) noexcept -> const EscapeFormatterPtr &;
};

}
