// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeDeltaUnit.hpp"

#include "../text/String.hpp"

#include <cstdint>

namespace erbsland::time {

/// Options for formatting fixed and calendar time deltas.
/// @seedoc{/reference/time/duration_and_time_amounts}
/// @tested{CalendarDeltaTest}
class TimeDeltaFormat final {
public:
    /// Style used for unit names.
    enum class UnitStyle : uint8_t {
        Short,
        Long,
    };

public:
    /// Create the default compact human-readable format.
    TimeDeltaFormat();

    // defaults
    ~TimeDeltaFormat() = default;
    TimeDeltaFormat(const TimeDeltaFormat &) = default;
    auto operator=(const TimeDeltaFormat &) -> TimeDeltaFormat & = default;
    TimeDeltaFormat(TimeDeltaFormat &&) noexcept = default;
    auto operator=(TimeDeltaFormat &&) noexcept -> TimeDeltaFormat & = default;

public: // accessors
    [[nodiscard]] auto unitStyle() const noexcept -> UnitStyle { return _unitStyle; }
    auto setUnitStyle(UnitStyle value) noexcept -> TimeDeltaFormat & {
        _unitStyle = value;
        return *this;
    }
    [[nodiscard]] auto valueSeparator() const noexcept -> const text::String & { return _valueSeparator; }
    auto setValueSeparator(text::String value) noexcept -> TimeDeltaFormat & {
        _valueSeparator = std::move(value);
        return *this;
    }
    [[nodiscard]] auto unitSeparator() const noexcept -> const text::String & { return _unitSeparator; }
    auto setUnitSeparator(text::String value) noexcept -> TimeDeltaFormat & {
        _unitSeparator = std::move(value);
        return *this;
    }
    [[nodiscard]] auto smallestUnit() const noexcept -> TimeDeltaUnit { return _smallestUnit; }
    auto setSmallestUnit(TimeDeltaUnit value) noexcept -> TimeDeltaFormat & {
        _smallestUnit = value;
        return *this;
    }
    [[nodiscard]] auto showFractions() const noexcept -> bool { return _showFractions; }
    auto setShowFractions(bool value) noexcept -> TimeDeltaFormat & {
        _showFractions = value;
        return *this;
    }
    [[nodiscard]] auto maximumFractionDigits() const noexcept -> uint8_t { return _maximumFractionDigits; }
    auto setMaximumFractionDigits(uint8_t value) noexcept -> TimeDeltaFormat &;
    /// Test if ELCL-specific short aliases are selected.
    [[nodiscard]] auto usesElclUnitNames() const noexcept -> bool { return _usesElclUnitNames; }

public:
    /// Create the default short-unit format.
    [[nodiscard]] static auto shortUnits() -> TimeDeltaFormat;
    /// Create the long-unit format.
    [[nodiscard]] static auto longUnits() -> TimeDeltaFormat;
    /// Create an ELCL-compatible format.
    [[nodiscard]] static auto elcl() -> TimeDeltaFormat;

private:
    UnitStyle _unitStyle{UnitStyle::Short};
    text::String _valueSeparator;
    text::String _unitSeparator;
    TimeDeltaUnit _smallestUnit{TimeDeltaUnit::Nanoseconds};
    bool _showFractions{false};
    uint8_t _maximumFractionDigits{0};
    bool _usesElclUnitNames{false};
};

}
