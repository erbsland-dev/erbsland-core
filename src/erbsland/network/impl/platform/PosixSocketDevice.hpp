// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <mutex>
#include <utility>

namespace erbsland::network::impl {

/// Serializes native socket operations and releases the lock while notifying the owning source.
/// @tested{PosixSocketLifecycleTest TcpSocketLiveTest UdpSocketLiveTest}
class PosixSocketDevice {
protected:
    /// Invoke a retained callback outside the operation lock, restoring the lock on every exit.
    /// @tparam tCallback The callback type.
    /// @tparam tArgs The callback argument types.
    /// @param lock The currently owned operation lock.
    /// @param callback A copy of the callback to retain through reentrant source operations.
    /// @param args The callback arguments.
    template <typename tCallback, typename... tArgs>
    static void notify(std::unique_lock<std::mutex> &lock, tCallback callback, tArgs &&...args) {
        lock.unlock();
        try {
            callback(std::forward<tArgs>(args)...);
        } catch (...) {
            lock.lock();
            throw;
        }
        lock.lock();
    }

    mutable std::mutex _operationMutex; ///< Serializes native operations, registration changes and close.
    bool _closed{false};                ///< Prevents registration or native operations after shutdown.
};

}
