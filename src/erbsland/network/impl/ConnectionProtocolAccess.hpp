// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConnectionProtocolAccess_fwd.hpp"

#include "../source/Connection.hpp"

namespace erbsland::network::impl {

/// Internal exclusive-protocol state attached to a common connection.
/// @tested{Http1TransactionTest}
class ConnectionProtocolAccess final {
public:
    /// Bind access to the target instance, which must outlive this helper.
    /// @param connection The accessed instance.
    explicit ConnectionProtocolAccess(Connection &connection) noexcept : _connection{connection} {}

public: // access
    /// Verify that the caller runs on the connection owner loop.
    void verifyOwner() { _connection.verifyCurrentOwnerLoop(); }
    /// Claim the connection for one protocol engine.
    /// @param owner The engine retained while it owns the connection.
    /// @return Whether the connection was available and is now claimed.
    [[nodiscard]] auto claim(std::shared_ptr<void> owner) -> bool;
    /// Release the current protocol engine.
    void release() noexcept { _connection._exclusiveProtocolOwner.reset(); }
    /// Store bounded bytes for the next sequential protocol engine.
    /// @param data The input to retain.
    void storeRetainedInput(mem::ByteBlock data) { _connection._retainedProtocolInput = std::move(data); }
    /// Detach bytes retained for the next sequential protocol engine.
    /// @return The previously retained input, or an empty block.
    [[nodiscard]] auto takeRetainedInput() -> mem::ByteBlock {
        return std::exchange(_connection._retainedProtocolInput, mem::ByteBlock{});
    }

private:
    Connection &_connection; ///< The accessed instance.
};

}
