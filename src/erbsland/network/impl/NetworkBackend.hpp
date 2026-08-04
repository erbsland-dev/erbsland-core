// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostResolver.hpp"

#include "../Network.hpp"

#include "../../event/EventBackend.hpp"
#include "../../event/EventBackendTarget_fwd.hpp"
#include "../../event/Events_fwd.hpp"
#include "../../text/String.hpp"

namespace erbsland::network::impl {

/// Built-in network backend with asynchronous host lookup support.
/// @tested{HostLookupTest HostLookupLiveDnsTest}
class NetworkBackend final : public event::EventBackend, public Network {
public:
    /// Create a backend with its platform resolver.
    NetworkBackend();
    /// Create a backend with an injected resolver.
    /// @param resolver The resolver used by named lookups.
    explicit NetworkBackend(HostResolverPtr resolver);

public: // implement EventBackend
    [[nodiscard]] auto backendId() const noexcept -> event::EventBackendId override;
    void attach(event::EventBackendTargetWeakPtr target, event::EventLoopDriverWeakPtr driver) override;
    void poll(time::TimePoint now) override;
    [[nodiscard]] auto handleEvent(const event::Event &event) -> bool override;
    [[nodiscard]] auto nextWakeTime() const -> std::optional<time::TimePoint> override;

public: // implement Network
    [[nodiscard]] auto createHostLookup() -> HostLookupPtr override;
    [[nodiscard]] auto createTcpListener() -> network::TcpListenerPtr override;
    [[nodiscard]] auto createTcpConnection() -> network::TcpConnectionPtr override;
    [[nodiscard]] auto createTlsClientConnection() -> network::TlsClientConnectionPtr override;
    [[nodiscard]] auto createTlsServerConnection() -> network::TlsServerConnectionPtr override;
    [[nodiscard]] auto createUdpSocket() -> UdpSocketPtr override;

private:
    /// Lock and return the events owner of this backend.
    [[nodiscard]] auto ownerEvents() const -> event::EventsPtr;

private:
    HostResolverPtr _resolver;                ///< Native host resolver.
    event::EventBackendTargetWeakPtr _target; ///< Attached event-loop target.
    event::EventLoopDriverWeakPtr _driver;    ///< Attached native event-loop driver.
};

}
