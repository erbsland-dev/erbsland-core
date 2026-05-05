// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char.hpp"
#include "IntegerBase.hpp"
#include "IntegerParseFlag.hpp"

#include "../unit/CpLength.hpp"

#include <optional>

namespace erbsland::text {

/// Options for integer parsing.
/// @tested{IntegerConversionTest}
class IntegerParseOptions final {
public:
    /// Create the default parse options.
    constexpr IntegerParseOptions() noexcept = default;

    // defaults
    ~IntegerParseOptions() = default;
    constexpr IntegerParseOptions(const IntegerParseOptions &) noexcept = default;
    constexpr IntegerParseOptions(IntegerParseOptions &&) noexcept = default;
    constexpr auto operator=(const IntegerParseOptions &) noexcept -> IntegerParseOptions & = default;
    constexpr auto operator=(IntegerParseOptions &&) noexcept -> IntegerParseOptions & = default;

public: // accessors
    /// Test if a fixed base is configured.
    [[nodiscard]] constexpr auto hasFixedBase() const noexcept -> bool { return _fixedBase.has_value(); }
    /// Get the optional fixed base.
    [[nodiscard]] constexpr auto fixedBase() const noexcept -> std::optional<IntegerBase> { return _fixedBase; }
    /// Set a fixed base.
    constexpr auto setFixedBase(IntegerBase base) noexcept -> IntegerParseOptions & {
        _fixedBase = base;
        return *this;
    }
    /// Clear the fixed base and detect the base from prefixes.
    constexpr auto clearFixedBase() noexcept -> IntegerParseOptions & {
        _fixedBase.reset();
        return *this;
    }
    /// Get the parse flags.
    [[nodiscard]] constexpr auto flags() const noexcept -> IntegerParseFlags { return _flags; }
    /// Set the parse flags.
    constexpr auto setFlags(IntegerParseFlags flags) noexcept -> IntegerParseOptions & {
        _flags = flags;
        return *this;
    }
    /// Add parse flags.
    constexpr auto addFlags(IntegerParseFlags flags) noexcept -> IntegerParseOptions & {
        _flags.set(flags);
        return *this;
    }
    /// Clear parse flags.
    constexpr auto clearFlags(IntegerParseFlags flags) noexcept -> IntegerParseOptions & {
        _flags.clear(flags);
        return *this;
    }
    /// Test if the given parse flag is set.
    [[nodiscard]] constexpr auto hasFlag(IntegerParseFlag flag) const noexcept -> bool { return _flags.isSet(flag); }
    /// Get the minimum number of digits to parse.
    [[nodiscard]] constexpr auto minimumDigits() const noexcept -> unit::CpLength { return _minimumDigits; }
    /// Set the minimum number of digits to parse.
    constexpr auto setMinimumDigits(unit::CpLength minimumDigits) noexcept -> IntegerParseOptions & {
        _minimumDigits = minimumDigits;
        return *this;
    }
    /// Get the maximum number of digits to parse.
    [[nodiscard]] constexpr auto maximumDigits() const noexcept -> unit::CpLength { return _maximumDigits; }
    /// Set the maximum number of digits to parse.
    constexpr auto setMaximumDigits(unit::CpLength maximumDigits) noexcept -> IntegerParseOptions & {
        _maximumDigits = maximumDigits;
        return *this;
    }
    /// Get the digit group separator.
    [[nodiscard]] constexpr auto separator() const noexcept -> Char { return _separator; }
    /// Set the digit group separator.
    constexpr auto setSeparator(Char separator) noexcept -> IntegerParseOptions & {
        _separator = separator;
        return *this;
    }

public: // factories
    /// Create options for parser/tokenizer use.
    [[nodiscard]] constexpr static auto parserDefault() noexcept -> IntegerParseOptions { return {}; }
    /// Create options for full-string integer conversion.
    [[nodiscard]] constexpr static auto stringDefault() noexcept -> IntegerParseOptions {
        auto result = IntegerParseOptions{};
        result.addFlags(IntegerParseFlag::AcceptMinusSign | IntegerParseFlag::IgnorePlusSign);
        return result;
    }
    /// Create the default parse options.
    [[nodiscard]] constexpr static auto defaultOptions() noexcept -> IntegerParseOptions { return parserDefault(); }
    /// Create options to parse exactly the given number of decimal digits.
    [[nodiscard]] constexpr static auto fixedDecimal(unit::CpLength count) noexcept -> IntegerParseOptions {
        auto result = IntegerParseOptions{};
        result.setFixedBase(IntegerBase::Decimal)
            .setMinimumDigits(count)
            .setMaximumDigits(count)
            .addFlags(IntegerParseFlag::StopAtMaximum);
        return result;
    }
    /// Create options to parse exactly the given number of hexadecimal digits.
    [[nodiscard]] constexpr static auto fixedHex(unit::CpLength count) noexcept -> IntegerParseOptions {
        auto result = IntegerParseOptions{};
        result.setFixedBase(IntegerBase::Hexadecimal)
            .setMinimumDigits(count)
            .setMaximumDigits(count)
            .addFlags(IntegerParseFlag::StopAtMaximum);
        return result;
    }

private:
    std::optional<IntegerBase> _fixedBase{};                   ///< The fixed base, or empty to detect prefixes.
    IntegerParseFlags _flags{};                                ///< The active parse flags.
    unit::CpLength _minimumDigits{unit::CpLength::zero()};     ///< The minimum digit count.
    unit::CpLength _maximumDigits{unit::CpLength::infinite()}; ///< The maximum digit count.
    Char _separator{U'\''};                                    ///< The digit separator character.
};

}
