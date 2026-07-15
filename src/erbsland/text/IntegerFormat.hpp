// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerBase.hpp"
#include "IntegerFormatFlag.hpp"
#include "IntegerSignMode.hpp"
#include "LetterCase.hpp"

#include "../unit/CpLength.hpp"

#include <optional>

namespace erbsland::text {

/// Options for integer text formatting.
/// @tested{IntegerConversionTest}
class IntegerFormat final {
public:
    /// Create the default decimal integer format.
    constexpr IntegerFormat() noexcept = default;
    /// Create an integer format for the given base.
    constexpr IntegerFormat(const IntegerBase base) noexcept : _base{base} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~IntegerFormat() = default;
    IntegerFormat(const IntegerFormat &) = default;
    IntegerFormat(IntegerFormat &&) = default;
    auto operator=(const IntegerFormat &) -> IntegerFormat & = default;
    auto operator=(IntegerFormat &&) -> IntegerFormat & = default;

public: // accessors
    /// Get the integer base.
    [[nodiscard]] constexpr auto base() const noexcept -> IntegerBase { return _base; }
    /// Set the integer base.
    constexpr auto setBase(IntegerBase base) noexcept -> IntegerFormat & {
        _base = base;
        return *this;
    }
    /// Set the integer base and letter case from an ASCII base prefix character.
    /// @return `true` if the prefix character was supported.
    auto setFromBasePrefix(Char character) noexcept -> bool;
    /// Get the format flags.
    [[nodiscard]] constexpr auto flags() const noexcept -> IntegerFormatFlags { return _flags; }
    /// Set the format flags.
    constexpr auto setFlags(IntegerFormatFlags flags) noexcept -> IntegerFormat & {
        _flags = flags;
        return *this;
    }
    /// Add format flags.
    constexpr auto addFlags(IntegerFormatFlags flags) noexcept -> IntegerFormat & {
        _flags.set(flags);
        return *this;
    }
    /// Clear format flags.
    constexpr auto clearFlags(IntegerFormatFlags flags) noexcept -> IntegerFormat & {
        _flags.clear(flags);
        return *this;
    }
    /// Test if the given format flag is set.
    [[nodiscard]] constexpr auto hasFlag(IntegerFormatFlag flag) const noexcept -> bool { return _flags.isSet(flag); }
    /// Get the letter case.
    [[nodiscard]] constexpr auto letterCase() const noexcept -> LetterCase { return _letterCase; }
    /// Set the letter case.
    constexpr auto setLetterCase(LetterCase letterCase) noexcept -> IntegerFormat & {
        _letterCase = letterCase;
        return *this;
    }
    /// Get the minimum digit field width.
    [[nodiscard]] constexpr auto fieldWidth() const noexcept -> unit::CpLength { return _fieldWidth; }
    /// Set the minimum digit field width.
    constexpr auto setFieldWidth(unit::CpLength fieldWidth) noexcept -> IntegerFormat & {
        _fieldWidth = fieldWidth;
        return *this;
    }
    /// Test if a minimum digit precision is configured.
    [[nodiscard]] constexpr auto hasPrecision() const noexcept -> bool { return _precision.has_value(); }
    /// Get the minimum digit precision, or zero if no precision is configured.
    [[nodiscard]] constexpr auto precision() const noexcept -> unit::CpLength {
        return _precision.value_or(unit::CpLength::zero());
    }
    /// Set the minimum digit precision.
    constexpr auto setPrecision(unit::CpLength precision) noexcept -> IntegerFormat & {
        _precision = precision;
        return *this;
    }
    /// Clear the minimum digit precision.
    constexpr auto clearPrecision() noexcept -> IntegerFormat & {
        _precision.reset();
        return *this;
    }
    /// Get the sign handling mode.
    [[nodiscard]] constexpr auto signMode() const noexcept -> IntegerSignMode { return _signMode; }
    /// Set the sign handling mode.
    constexpr auto setSignMode(IntegerSignMode signMode) noexcept -> IntegerFormat & {
        _signMode = signMode;
        return *this;
    }

public: // factories
    /// Create the default format.
    [[nodiscard]] constexpr static auto defaultFormat() noexcept -> IntegerFormat { return {}; }
    /// Create a decimal format.
    [[nodiscard]] constexpr static auto decimal() noexcept -> IntegerFormat {
        return IntegerFormat{}.setBase(IntegerBase::Decimal);
    }
    /// Create a hexadecimal format.
    [[nodiscard]] constexpr static auto hexadecimal() noexcept -> IntegerFormat {
        return IntegerFormat{}.setBase(IntegerBase::Hexadecimal);
    }
    /// Create a binary format.
    [[nodiscard]] constexpr static auto binary() noexcept -> IntegerFormat {
        return IntegerFormat{}.setBase(IntegerBase::Binary);
    }
    /// Create an octal format.
    [[nodiscard]] constexpr static auto octal() noexcept -> IntegerFormat {
        return IntegerFormat{}.setBase(IntegerBase::Octal);
    }

private:
    IntegerBase _base{IntegerBase::Decimal};                  ///< The integer base.
    IntegerFormatFlags _flags{};                              ///< The active format flags.
    LetterCase _letterCase{LetterCase::Lowercase};            ///< The case for ASCII letters.
    unit::CpLength _fieldWidth{unit::CpLength::zero()};       ///< The minimum digit field width.
    std::optional<unit::CpLength> _precision{};               ///< The minimum digit precision.
    IntegerSignMode _signMode{IntegerSignMode::NegativeOnly}; ///< The sign handling mode.
};

}
