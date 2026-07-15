// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/Result.hpp"

namespace erbsland::stream {

/// The status of a bounded stream read operation.
/// @tested{StreamResultTest}
class StreamReadStatus : public util::Result {
public:
    using Result::Result;

public: // operators
    constexpr auto operator==(const StreamReadStatus &other) const noexcept -> bool {
        return _value.value == other._value.value;
    }

public: // tests
    /// Test if the result contains caller-visible data.
    [[nodiscard]] constexpr auto hasData() const noexcept -> bool { return *this == Data; }
    /// Test if the stream reached its normal end without returning data.
    [[nodiscard]] constexpr auto isFinished() const noexcept -> bool { return *this == Finished; }
    /// Test if the operation did not complete within its bounded wait.
    [[nodiscard]] constexpr auto isTimeout() const noexcept -> bool { return *this == Timeout; }

public:
    static const StreamReadStatus Data;     ///< Data was read.
    static const StreamReadStatus Finished; ///< The stream reached its normal end.
    static const StreamReadStatus Timeout;  ///< No complete result was available before the deadline.
};

inline constexpr StreamReadStatus StreamReadStatus::Data = Value::success<0>();
inline constexpr StreamReadStatus StreamReadStatus::Finished = Value::success<1>();
inline constexpr StreamReadStatus StreamReadStatus::Timeout = Value::failure<0>();

}
