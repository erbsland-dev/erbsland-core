// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Capitalization.hpp"
#include "StringLiteral.hpp"

#include <cstdint>

namespace erbsland::text {

/// Options for boolean text formatting.
/// @tested{ToStringTest}
class BooleanFormat final {
public:
    /// The word pair used for boolean values.
    enum class Style : uint8_t {
        TrueFalse = 0,       ///< Write `true` and `false`.
        YesNo = 1,           ///< Write `yes` and `no`.
        OnOff = 2,           ///< Write `on` and `off`.
        EnabledDisabled = 3, ///< Write `enabled` and `disabled`.
    };

public:
    /// Create the default `true`/`false` lowercase format.
    constexpr BooleanFormat() noexcept = default;
    /// Create a boolean format for the given style.
    constexpr BooleanFormat(const Style style) noexcept : _style{style} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~BooleanFormat() = default;
    BooleanFormat(const BooleanFormat &) = default;
    BooleanFormat(BooleanFormat &&) = default;
    auto operator=(const BooleanFormat &) -> BooleanFormat & = default;
    auto operator=(BooleanFormat &&) -> BooleanFormat & = default;

public: // accessors
    /// Get the word pair style.
    [[nodiscard]] constexpr auto style() const noexcept -> Style { return _style; }
    /// Set the word pair style.
    constexpr auto setStyle(Style style) noexcept -> BooleanFormat & {
        _style = style;
        return *this;
    }
    /// Get the word capitalization.
    [[nodiscard]] constexpr auto capitalization() const noexcept -> Capitalization { return _capitalization; }
    /// Set the word capitalization.
    constexpr auto setCapitalization(Capitalization capitalization) noexcept -> BooleanFormat & {
        _capitalization = capitalization;
        return *this;
    }
    /// Get the text for the given boolean value.
    [[nodiscard]] auto text(bool value) const noexcept -> StringLiteral;

public: // factories
    /// Create the default format.
    [[nodiscard]] constexpr static auto defaultFormat() noexcept -> BooleanFormat { return {}; }
    /// Create a `true`/`false` format.
    [[nodiscard]] constexpr static auto trueFalse() noexcept -> BooleanFormat {
        return BooleanFormat{Style::TrueFalse};
    }
    /// Create a `yes`/`no` format.
    [[nodiscard]] constexpr static auto yesNo() noexcept -> BooleanFormat { return BooleanFormat{Style::YesNo}; }
    /// Create an `on`/`off` format.
    [[nodiscard]] constexpr static auto onOff() noexcept -> BooleanFormat { return BooleanFormat{Style::OnOff}; }
    /// Create an `enabled`/`disabled` format.
    [[nodiscard]] constexpr static auto enabledDisabled() noexcept -> BooleanFormat {
        return BooleanFormat{Style::EnabledDisabled};
    }

private:
    Style _style{Style::TrueFalse};                            ///< The word pair style.
    Capitalization _capitalization{Capitalization::Lowercase}; ///< The capitalization for the words.
};

}
