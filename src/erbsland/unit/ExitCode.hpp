// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/impl/ComparisonHelper.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace erbsland::unit {

/// Represents an exit code for a program.
class ExitCode {
public:
    /// The raw exit-code value type.
    using Value = int32_t;

public:
    /// Create an exit code from a given generic integer value.
    explicit constexpr ExitCode(const Value exitCode) : _value{exitCode} {}

    // defaults
    ExitCode() = default;
    ExitCode(const ExitCode &) = default;
    auto operator=(const ExitCode &) -> ExitCode & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const ExitCode &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, Value value, value);

public: // tests
    /// Test if the exit-code is zero.
    [[nodiscard]] auto isSuccess() const noexcept -> bool { return _value == 0; }
    /// Test if the exit-code is not zero.
    [[nodiscard]] auto isFailure() const noexcept -> bool { return _value != 0; }
    /// Test if the exit-code is zero.
    [[nodiscard]] auto isZero() const noexcept -> bool { return _value == 0; }
    /// Test if the exit-code is one.
    [[nodiscard]] auto isOne() const noexcept -> bool { return _value == 1; }
    /// Test if the exit-code is at its minimum value.
    [[nodiscard]] auto isMinimum() const noexcept -> bool { return _value == std::numeric_limits<Value>::min(); }
    /// Test if the exit-code is at its maximum value.
    [[nodiscard]] auto isMaximum() const noexcept -> bool { return _value == std::numeric_limits<Value>::max(); }
    /// Test if the exit code is in valid POSIX range.
    [[nodiscard]] auto isValidPosix() const noexcept -> bool { return _value >= Value{0} && _value <= Value{255}; }
    /// Test if the exit code is in the recommended POSIX range.
    [[nodiscard]] auto isRecommendedPosix() const noexcept -> bool {
        return _value >= Value{0} && _value <= Value{125};
    }

public: // conversion
    /// Access the raw underlying value.
    [[nodiscard]] auto toRawValue() const noexcept -> Value { return _value; }
    /// Convert the exit code to the recommended POSIX range
    [[nodiscard]] auto toRecommendedPosix() const noexcept -> ExitCode {
        return ExitCode{std::clamp(_value, Value{0}, Value{125})};
    }

public: // factory methods
    /// Get a zero exit-code, indicating success.
    [[nodiscard]] static constexpr auto success() noexcept -> ExitCode { return ExitCode{0}; }
    /// Get a one exit-code, indicating a generic failure.
    [[nodiscard]] static constexpr auto failure() noexcept -> ExitCode { return ExitCode{1}; }
    /// Get a zero exit-code.
    [[nodiscard]] static constexpr auto zero() noexcept -> ExitCode { return ExitCode{0}; }
    /// Get a one exit-code.
    [[nodiscard]] static constexpr auto one() noexcept -> ExitCode { return ExitCode{1}; }
    /// Get the minimum possible exit code.
    [[nodiscard]] static constexpr auto minimum() noexcept -> ExitCode {
        return ExitCode{std::numeric_limits<Value>::min()};
    }
    /// get the maximum possible exit code.
    [[nodiscard]] static constexpr auto maximum() noexcept -> ExitCode {
        return ExitCode{std::numeric_limits<Value>::max()};
    }

private:
    Value _value{0};
};

}
