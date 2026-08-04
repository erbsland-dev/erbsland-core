// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../EscapeAmount.hpp"

#include "../../unit/CpLength.hpp"

namespace erbsland::text::json {

/// Options for serializing a JSON value.
/// @tested{JsonParserTest}
class JsonFormatOptions final {
public: // accessors
    /// Get the spaces used for each indentation level; zero selects compact output.
    [[nodiscard]] constexpr auto indentation() const noexcept -> unit::CpLength { return _indentation; }
    /// Set the spaces used for each indentation level.
    constexpr auto setIndentation(const unit::CpLength value) noexcept -> JsonFormatOptions & {
        _indentation = value;
        return *this;
    }
    /// Get the JSON string escape amount.
    [[nodiscard]] constexpr auto escapeAmount() const noexcept -> EscapeAmount { return _escapeAmount; }
    /// Set the JSON string escape amount.
    constexpr auto setEscapeAmount(const EscapeAmount value) noexcept -> JsonFormatOptions & {
        _escapeAmount = value;
        return *this;
    }

public: // factories
    /// Create compact formatting options.
    [[nodiscard]] static constexpr auto compact() noexcept -> JsonFormatOptions { return {}; }
    /// Create pretty formatting options using two spaces.
    [[nodiscard]] static constexpr auto pretty() noexcept -> JsonFormatOptions {
        return JsonFormatOptions{}.setIndentation(unit::CpLength{2U});
    }

private:
    unit::CpLength _indentation;                        ///< Spaces per nesting level.
    EscapeAmount _escapeAmount{EscapeAmount::Required}; ///< String escape policy.
};

}
