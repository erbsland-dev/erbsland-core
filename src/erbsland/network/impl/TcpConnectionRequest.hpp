// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpAcceptedSocket.hpp"
#include "TcpConnectionDevice_fwd.hpp"
#include "TcpConnectionRequest_fwd.hpp"

#include "../source/ConnectionQuota.hpp"
#include "../tcp/TcpConnectionRequest.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// Built-in transferable pending TCP connection request.
/// @tested{TcpListenerTest TcpSocketLiveTest}
class TcpConnectionRequest final : public network::TcpConnectionRequest {
public:
    /// The accepted native socket and its transferred connection-quota lease.
    struct Claim final {
        TcpAcceptedSocketPtr socket;              ///< Accepted native socket.
        network::ConnectionQuotaLease quotaLease; ///< Accepted-connection lifetime lease.
    };

public:
    /// Create a pending request around an accepted native socket.
    /// @param socket The transferable accepted socket.
    /// @param quotaLease The accepted-connection quota lease transferred with the socket.
    /// @param releaseSlot Callback releasing one listener pending-request slot.
    TcpConnectionRequest(
        TcpAcceptedSocketPtr socket, network::ConnectionQuotaLease quotaLease, std::function<void()> releaseSlot);
    /// Create a request without a quota lease for deterministic backend tests.
    TcpConnectionRequest(TcpAcceptedSocketPtr socket, std::function<void()> releaseSlot);
    ~TcpConnectionRequest() override;

public: // implement network::TcpConnectionRequest
    [[nodiscard]] auto remoteEndpoint() const noexcept -> const IpEndpoint & override;
    [[nodiscard]] auto state() const noexcept -> TcpConnectionRequestState override;
    void reject() noexcept override;

public: // implementation interface
    /// Test whether a native connection device can adopt this request without consuming it.
    [[nodiscard]] auto isCompatibleWith(const TcpConnectionDevice &device) const noexcept -> bool;
    /// Atomically claim the accepted socket.
    /// @return The socket, or an empty pointer if the request is no longer pending.
    [[nodiscard]] auto claim() noexcept -> std::optional<Claim>;

private:
    /// Release this request's listener pending-request slot exactly once.
    void releaseSlot() noexcept;

private:
    mutable std::mutex _mutex;                 ///< Protects native socket transfer.
    TcpAcceptedSocketPtr _socket;              ///< Pending accepted socket.
    network::ConnectionQuotaLease _quotaLease; ///< Lease transferred with the accepted socket.
    IpEndpoint _remoteEndpoint;                ///< Immutable peer endpoint.
    std::function<void()> _releaseSlot;        ///< Pending-slot release callback.
    std::atomic<TcpConnectionRequestState> _state{TcpConnectionRequestState::Pending}; ///< Decision state.
};

}
