// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/Result.hpp"

namespace erbsland::stream {

/// The status of a bounded stream positioning operation.
/// @tested{StreamPositionTest}
class StreamPositionStatus final : public util::Result {
public:
    using Result::Result;

public: // operators
    constexpr auto operator==(const StreamPositionStatus &other) const noexcept -> bool {
        return _value.value == other._value.value;
    }

public: // tests
    /// Test if the stream position was changed.
    [[nodiscard]] constexpr auto isSuccess() const noexcept -> bool { return *this == Success; }
    /// Test if positioning did not complete within its bounded wait.
    [[nodiscard]] constexpr auto isTimeout() const noexcept -> bool { return *this == Timeout; }

public:
    static const StreamPositionStatus Success; ///< The stream position was changed.
    static const StreamPositionStatus Timeout; ///< Positioning did not complete before the deadline.
};

inline constexpr StreamPositionStatus StreamPositionStatus::Success = Value::success<0>();
inline constexpr StreamPositionStatus StreamPositionStatus::Timeout = Value::failure<0>();

}
