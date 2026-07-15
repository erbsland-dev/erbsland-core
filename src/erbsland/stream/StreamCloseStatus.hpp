// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/Result.hpp"

namespace erbsland::stream {

/// The status of a bounded stream close operation.
/// @tested{StreamResultTest}
class StreamCloseStatus final : public util::Result {
public:
    using Result::Result;

public: // operators
    constexpr auto operator==(const StreamCloseStatus &other) const noexcept -> bool {
        return _value.value == other._value.value;
    }

public: // tests
    /// Test if the stream is closed.
    [[nodiscard]] constexpr auto isClosed() const noexcept -> bool { return *this == Closed; }
    /// Test if closing did not finish within its bounded wait.
    [[nodiscard]] constexpr auto isTimeout() const noexcept -> bool { return *this == Timeout; }

public:
    static const StreamCloseStatus Closed;  ///< The stream is closed.
    static const StreamCloseStatus Timeout; ///< Closing continues, but did not finish before the deadline.
};

inline constexpr StreamCloseStatus StreamCloseStatus::Closed = Value::success<0>();
inline constexpr StreamCloseStatus StreamCloseStatus::Timeout = Value::failure<0>();

}
