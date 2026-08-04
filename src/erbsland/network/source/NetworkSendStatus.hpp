// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../util/Result.hpp"

namespace erbsland::network {

/// The immediate result of an atomic non-blocking network send.
/// @tested{NetworkFacadeTest}
class NetworkSendStatus final : public util::Result {
public:
    /// Create a send status from a result value.
    /// @param value The internal result value.
    constexpr NetworkSendStatus(const Value value) noexcept : Result{value} {} // NOLINT(*-explicit-constructor)

public:
    /// Test if the complete message was accepted.
    /// @return `true` for `Accepted`.
    [[nodiscard]] auto isAccepted() const noexcept -> bool { return *this == Accepted; }
    /// Test if capacity must become writable before retrying.
    /// @return `true` for `WouldBlock`.
    [[nodiscard]] auto wouldBlock() const noexcept -> bool { return *this == WouldBlock; }
    /// Test if the source no longer accepts output.
    /// @return `true` for `Closed`.
    [[nodiscard]] auto isClosed() const noexcept -> bool { return *this == Closed; }

public:
    static const NetworkSendStatus Accepted;   ///< The complete message was accepted atomically.
    static const NetworkSendStatus WouldBlock; ///< The send queue currently has insufficient capacity.
    static const NetworkSendStatus Closed;     ///< The source no longer accepts output.
};

inline constexpr NetworkSendStatus NetworkSendStatus::Accepted = NetworkSendStatus::Value::success<0>();
inline constexpr NetworkSendStatus NetworkSendStatus::WouldBlock = NetworkSendStatus::Value::failure<0>();
inline constexpr NetworkSendStatus NetworkSendStatus::Closed = NetworkSendStatus::Value::failure<1>();

}
