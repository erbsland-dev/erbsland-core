// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/Result.hpp"

namespace erbsland::stream {

/// The status of waiting for a stream to become ready.
/// @tested{StreamResultTest}
class StreamWaitStatus final : public util::Result {
public:
    using Result::Result;

public: // operators
    /// Compare two stream-wait statuses.
    constexpr auto operator==(const StreamWaitStatus &other) const noexcept -> bool {
        return _value.value == other._value.value;
    }

public: // tests
    /// Test if the stream reached its ready state.
    [[nodiscard]] constexpr auto isReady() const noexcept -> bool { return *this == Ready; }
    /// Test if the stream did not become ready within its bounded wait.
    [[nodiscard]] constexpr auto isTimeout() const noexcept -> bool { return *this == Timeout; }

public:
    static const StreamWaitStatus Ready;   ///< The stream reached its ready state.
    static const StreamWaitStatus Timeout; ///< The stream did not become ready before the deadline.
};

inline constexpr StreamWaitStatus StreamWaitStatus::Ready = Value::success<0>();
inline constexpr StreamWaitStatus StreamWaitStatus::Timeout = Value::failure<0>();

}
