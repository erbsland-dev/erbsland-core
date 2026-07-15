// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/Result.hpp"

namespace erbsland::stream {

/// The status of a bounded stream write or flush operation.
/// @tested{StreamResultTest}
class StreamWriteStatus final : public util::Result {
public:
    using Result::Result;

public: // operators
    constexpr auto operator==(const StreamWriteStatus &other) const noexcept -> bool {
        return _value.value == other._value.value;
    }

public: // tests
    /// Test if the complete output operation succeeded.
    [[nodiscard]] constexpr auto isSuccess() const noexcept -> bool { return *this == Success; }
    /// Test if the output operation did not complete within its bounded wait.
    [[nodiscard]] constexpr auto isTimeout() const noexcept -> bool { return *this == Timeout; }

public:
    static const StreamWriteStatus Success; ///< The complete output operation succeeded.
    static const StreamWriteStatus Timeout; ///< The output operation did not complete before the deadline.
};

inline constexpr StreamWriteStatus StreamWriteStatus::Success = Value::success<0>();
inline constexpr StreamWriteStatus StreamWriteStatus::Timeout = Value::failure<0>();

}
