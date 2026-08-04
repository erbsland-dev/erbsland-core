// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NetworkBackend.hpp"

#include "HostLookup.hpp"
#include "TcpConnection.hpp"
#include "TcpListener.hpp"
#include "TlsClientConnection.hpp"
#include "TlsServerConnection.hpp"
#include "UdpSocket.hpp"

#include "../../err/LogicError.hpp"
#include "../../event/CurrentEvents.hpp"
#include "../../event/EventBackendTarget.hpp"
#include "../../event/EventRegistry.hpp"
#include "../../event/Events.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

NetworkBackend::NetworkBackend() : NetworkBackend{createHostResolver()} {
}

NetworkBackend::NetworkBackend(HostResolverPtr resolver) : _resolver{std::move(resolver)} {
    if (_resolver == nullptr) {
        throw err::LogicError{"The network backend requires a host resolver."_el};
    }
}

auto NetworkBackend::backendId() const noexcept -> event::EventBackendId {
    return event::id::NetworkBackend;
}

void NetworkBackend::attach(event::EventBackendTargetWeakPtr target, event::EventLoopDriverWeakPtr driver) {
    _target = std::move(target);
    _driver = std::move(driver);
}

void NetworkBackend::poll([[maybe_unused]] const time::TimePoint now) {
}

auto NetworkBackend::handleEvent([[maybe_unused]] const event::Event &event) -> bool {
    return false;
}

auto NetworkBackend::nextWakeTime() const -> std::optional<time::TimePoint> {
    return std::nullopt;
}

auto NetworkBackend::createHostLookup() -> HostLookupPtr {
    const auto owner = ownerEvents();
    if (event::currentEvents() != owner) {
        throw err::LogicError{"Host lookups must be created on their owner event loop."_el};
    }
    return std::make_shared<impl::HostLookup>(owner, _resolver);
}

auto NetworkBackend::createTcpListener() -> network::TcpListenerPtr {
    const auto owner = ownerEvents();
    if (event::currentEvents() != owner) {
        throw err::LogicError{"TCP listeners must be created on their owner event loop."_el};
    }
    const auto driver = _driver.lock();
    if (driver == nullptr) {
        throw err::LogicError{"The network backend is not attached to an event-loop driver."_el};
    }
    return std::make_shared<impl::TcpListener>(owner, driver);
}

auto NetworkBackend::createTcpConnection() -> network::TcpConnectionPtr {
    const auto owner = ownerEvents();
    if (event::currentEvents() != owner) {
        throw err::LogicError{"TCP connections must be created on their owner event loop."_el};
    }
    const auto driver = _driver.lock();
    if (driver == nullptr) {
        throw err::LogicError{"The network backend is not attached to an event-loop driver."_el};
    }
    return std::make_shared<impl::TcpConnection>(owner, driver, _resolver);
}

auto NetworkBackend::createTlsClientConnection() -> network::TlsClientConnectionPtr {
    const auto owner = ownerEvents();
    if (event::currentEvents() != owner) {
        throw err::LogicError{"TLS client connections must be created on their owner event loop."_el};
    }
    const auto driver = _driver.lock();
    if (driver == nullptr) {
        throw err::LogicError{"The network backend is not attached to an event-loop driver."_el};
    }
    auto tcpConnection = std::make_shared<impl::TcpConnection>(owner, driver, _resolver);
    return std::make_shared<impl::TlsClientConnection>(owner, std::move(tcpConnection));
}

auto NetworkBackend::createTlsServerConnection() -> network::TlsServerConnectionPtr {
    const auto owner = ownerEvents();
    if (event::currentEvents() != owner) {
        throw err::LogicError{"TLS server connections must be created on their owner event loop."_el};
    }
    const auto driver = _driver.lock();
    if (driver == nullptr) {
        throw err::LogicError{"The network backend is not attached to an event-loop driver."_el};
    }
    auto tcpConnection = std::make_shared<impl::TcpConnection>(owner, driver, _resolver);
    return std::make_shared<impl::TlsServerConnection>(owner, std::move(tcpConnection));
}

auto NetworkBackend::createUdpSocket() -> UdpSocketPtr {
    const auto owner = ownerEvents();
    if (event::currentEvents() != owner) {
        throw err::LogicError{"UDP sockets must be created on their owner event loop."_el};
    }
    const auto driver = _driver.lock();
    if (driver == nullptr) {
        throw err::LogicError{"The network backend is not attached to an event-loop driver."_el};
    }
    return std::make_shared<impl::UdpSocket>(owner, driver);
}

auto NetworkBackend::ownerEvents() const -> event::EventsPtr {
    const auto target = _target.lock();
    const auto result = std::dynamic_pointer_cast<event::Events>(target);
    if (result == nullptr) {
        throw err::LogicError{"The network backend is not attached to an event loop."_el};
    }
    return result;
}

}
