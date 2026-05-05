// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Char.hpp"
#include "../EscapeAmount.hpp"
#include "../EscapeFormat.hpp"
#include "../StringBuilder.hpp"

#include <cstddef>
#include <memory>

namespace erbsland::text::impl {

class EscapeFormatter;
using EscapeFormatterPtr = std::shared_ptr<EscapeFormatter>;

class EscapeFormatter {
public:
    virtual ~EscapeFormatter() = default;

public:
    [[nodiscard]] virtual auto needsEscape(Char character, EscapeAmount) const noexcept -> bool = 0;
    virtual void escape(Char character, StringBuilder &builder) const = 0;
    [[nodiscard]] virtual auto escapeSize(Char character, StringKind stringKind) const noexcept -> std::size_t = 0;

protected:
    [[nodiscard]] static auto needsEscapeByAmount(
        Char character, EscapeAmount amount, bool required, bool balanced) noexcept -> bool;

public:
    [[nodiscard]] static auto forFormat(EscapeFormat format) noexcept -> const EscapeFormatterPtr &;
};

}
