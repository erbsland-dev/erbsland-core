// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FloatFormat_fwd.hpp"
#include "LetterCase.hpp"

#include "../unit/ElementCount.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::text {

/// Options for floating point text formatting.
/// @tested{FloatConversionTest}
class FloatFormat final {
public:
    /// The floating point presentation style.
    enum class Style : uint8_t {
        Default = 0,     ///< Use the default `std::format` floating point presentation.
        Fixed = 1,       ///< Use fixed-point notation.
        Scientific = 2,  ///< Use scientific notation.
        General = 3,     ///< Use general notation.
        Hexadecimal = 4, ///< Use hexadecimal floating point notation.
    };

public:
    /// Create the default floating point format.
    constexpr FloatFormat() noexcept = default;
    /// Create a floating point format for the given style.
    constexpr FloatFormat(const Style style) noexcept : _style{style} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~FloatFormat() = default;
    FloatFormat(const FloatFormat &) = default;
    FloatFormat(FloatFormat &&) = default;
    auto operator=(const FloatFormat &) -> FloatFormat & = default;
    auto operator=(FloatFormat &&) -> FloatFormat & = default;

public: // accessors
    /// Get the presentation style.
    [[nodiscard]] constexpr auto style() const noexcept -> Style { return _style; }
    /// Set the presentation style.
    constexpr auto setStyle(Style style) noexcept -> FloatFormat & {
        _style = style;
        return *this;
    }
    /// Test if a precision is configured.
    [[nodiscard]] constexpr auto hasPrecision() const noexcept -> bool { return _precision.has_value(); }
    /// Get the configured precision, or zero if no precision is configured.
    [[nodiscard]] constexpr auto precision() const noexcept -> unit::ElementCount {
        return _precision.value_or(unit::ElementCount{});
    }
    /// Set the precision.
    constexpr auto setPrecision(unit::ElementCount precision) noexcept -> FloatFormat & {
        _precision = precision;
        return *this;
    }
    /// Clear the precision.
    constexpr auto clearPrecision() noexcept -> FloatFormat & {
        _precision.reset();
        return *this;
    }
    /// Get the letter case used by styles that emit letters.
    [[nodiscard]] constexpr auto letterCase() const noexcept -> LetterCase { return _letterCase; }
    /// Set the letter case used by styles that emit letters.
    constexpr auto setLetterCase(LetterCase letterCase) noexcept -> FloatFormat & {
        _letterCase = letterCase;
        return *this;
    }

public: // factories
    /// Create the default format.
    [[nodiscard]] constexpr static auto defaultFormat() noexcept -> FloatFormat { return {}; }
    /// Create a fixed-point format.
    [[nodiscard]] constexpr static auto fixed() noexcept -> FloatFormat { return FloatFormat{Style::Fixed}; }
    /// Create a scientific format.
    [[nodiscard]] constexpr static auto scientific() noexcept -> FloatFormat { return FloatFormat{Style::Scientific}; }
    /// Create a general format.
    [[nodiscard]] constexpr static auto general() noexcept -> FloatFormat { return FloatFormat{Style::General}; }
    /// Create a hexadecimal floating point format.
    [[nodiscard]] constexpr static auto hexadecimal() noexcept -> FloatFormat {
        return FloatFormat{Style::Hexadecimal};
    }

private:
    Style _style{Style::Default};                   ///< The presentation style.
    std::optional<unit::ElementCount> _precision{}; ///< The optional precision.
    LetterCase _letterCase{LetterCase::Lowercase};  ///< The case for ASCII letters.
};

}
