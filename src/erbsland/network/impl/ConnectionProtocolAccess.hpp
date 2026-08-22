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
    /// Verify that the caller runs on the connection owner loop.
    static void verifyOwner(Connection &connection) { connection.verifyCurrentOwnerEvents(); }
    /// Claim the connection for one protocol engine.
    [[nodiscard]] static auto claim(Connection &connection, std::shared_ptr<void> owner) -> bool {
        if (connection._exclusiveProtocolOwner) {
            return false;
        }
        connection._exclusiveProtocolOwner = std::move(owner);
        return true;
    }
    /// Release the current protocol engine.
    static void release(Connection &connection) noexcept { connection._exclusiveProtocolOwner.reset(); }
    /// Store bounded bytes for the next sequential protocol engine.
    static void storeRetainedInput(Connection &connection, mem::ByteBlock data) {
        connection._retainedProtocolInput = std::move(data);
    }
    /// Detach bytes retained for the next sequential protocol engine.
    [[nodiscard]] static auto takeRetainedInput(Connection &connection) -> mem::ByteBlock {
        return std::exchange(connection._retainedProtocolInput, mem::ByteBlock{});
    }
};

}
