// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FloatParseFlag.hpp"

#include <cstdint>

namespace erbsland::text {

/// Options for floating point parsing.
/// @tested{FloatConversionTest}
class FloatParseOptions final {
public:
    /// The accepted floating point presentation style.
    enum class Style : uint8_t {
        General = 0,     ///< Accept fixed or scientific notation.
        Fixed = 1,       ///< Accept fixed-point notation.
        Scientific = 2,  ///< Accept scientific notation.
        Hexadecimal = 3, ///< Accept hexadecimal floating point notation.
    };

public:
    /// Create the default parse options.
    constexpr FloatParseOptions() noexcept = default;

    // defaults
    ~FloatParseOptions() = default;
    FloatParseOptions(const FloatParseOptions &) = default;
    FloatParseOptions(FloatParseOptions &&) = default;
    auto operator=(const FloatParseOptions &) -> FloatParseOptions & = default;
    auto operator=(FloatParseOptions &&) -> FloatParseOptions & = default;

public: // accessors
    /// Get the accepted presentation style.
    [[nodiscard]] constexpr auto style() const noexcept -> Style { return _style; }
    /// Set the accepted presentation style.
    constexpr auto setStyle(Style style) noexcept -> FloatParseOptions & {
        _style = style;
        return *this;
    }
    /// Get the parse flags.
    [[nodiscard]] constexpr auto flags() const noexcept -> FloatParseFlags { return _flags; }
    /// Set the parse flags.
    constexpr auto setFlags(FloatParseFlags flags) noexcept -> FloatParseOptions & {
        _flags = flags;
        return *this;
    }
    /// Add parse flags.
    constexpr auto addFlags(FloatParseFlags flags) noexcept -> FloatParseOptions & {
        _flags.set(flags);
        return *this;
    }
    /// Clear parse flags.
    constexpr auto clearFlags(FloatParseFlags flags) noexcept -> FloatParseOptions & {
        _flags.clear(flags);
        return *this;
    }
    /// Test if the given parse flag is set.
    [[nodiscard]] constexpr auto hasFlag(FloatParseFlag flag) const noexcept -> bool { return _flags.isSet(flag); }

public: // factories
    /// Create the default parse options.
    [[nodiscard]] constexpr static auto defaultOptions() noexcept -> FloatParseOptions { return {}; }

private:
    Style _style{Style::General}; ///< The accepted presentation style.
    FloatParseFlags _flags{};     ///< The active parse flags.
};

}
