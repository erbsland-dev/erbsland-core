// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NetworkBackend.hpp"

#include "host/HostLookup.hpp"
#include "http/client/HttpClientSession.hpp"
#include "http/server/HttpServer.hpp"
#include "tcp/TcpConnection.hpp"
#include "tcp/TcpListener.hpp"
#include "tls/client/TlsClientConnection.hpp"
#include "tls/server/TlsServerConnection.hpp"
#include "udp/UdpSocket.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../event/CurrentLoop.hpp"
#include "../../event/EventBackendTarget.hpp"
#include "../../event/EventRegistry.hpp"
#include "../../event/EventTarget.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace err;
using namespace text::literals;

NetworkBackend::NetworkBackend() : NetworkBackend{createHostResolver()} {
}

NetworkBackend::NetworkBackend(HostResolverPtr resolver) : _resolver{std::move(resolver)} {
    if (_resolver == nullptr) {
        throw LogicFailure{"The network backend requires a host resolver."_el};
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
    const auto owner = ownerLoop();
    if (event::currentLoop() != owner) {
        throw LogicFailure{"Host lookups must be created on their owner event loop."_el};
    }
    return std::make_shared<impl::HostLookup>(owner, _resolver);
}

auto NetworkBackend::createHttpClientSession() -> HttpClientSessionPtr {
    const auto owner = ownerLoop();
    if (event::currentLoop() != owner) {
        throw LogicFailure{"HTTP client sessions must be created on their owner event loop."_el};
    }
    return std::make_shared<impl::HttpClientSession>(owner);
}

auto NetworkBackend::createHttpServer() -> HttpServerPtr {
    const auto owner = ownerLoop();
    if (event::currentLoop() != owner) {
        throw LogicFailure{"HTTP servers must be created on their owner event loop."_el};
    }
    return std::make_shared<impl::HttpServer>(owner);
}

auto NetworkBackend::createTcpListener() -> network::TcpListenerPtr {
    const auto owner = ownerLoop();
    if (event::currentLoop() != owner) {
        throw LogicFailure{"TCP listeners must be created on their owner event loop."_el};
    }
    const auto driver = _driver.lock();
    if (driver == nullptr) {
        throw LogicFailure{"The network backend is not attached to an event-loop driver."_el};
    }
    return std::make_shared<impl::TcpListener>(owner, driver);
}

auto NetworkBackend::createTcpConnection() -> network::TcpConnectionPtr {
    const auto owner = ownerLoop();
    if (event::currentLoop() != owner) {
        throw LogicFailure{"TCP connections must be created on their owner event loop."_el};
    }
    const auto driver = _driver.lock();
    if (driver == nullptr) {
        throw LogicFailure{"The network backend is not attached to an event-loop driver."_el};
    }
    return std::make_shared<impl::TcpConnection>(owner, driver, _resolver);
}

auto NetworkBackend::createTlsClientConnection() -> network::TlsClientConnectionPtr {
    const auto owner = ownerLoop();
    if (event::currentLoop() != owner) {
        throw LogicFailure{"TLS client connections must be created on their owner event loop."_el};
    }
    const auto driver = _driver.lock();
    if (driver == nullptr) {
        throw LogicFailure{"The network backend is not attached to an event-loop driver."_el};
    }
    auto tcpConnection = std::make_shared<impl::TcpConnection>(owner, driver, _resolver);
    return std::make_shared<impl::TlsClientConnection>(owner, std::move(tcpConnection));
}

auto NetworkBackend::createTlsServerConnection() -> network::TlsServerConnectionPtr {
    const auto owner = ownerLoop();
    if (event::currentLoop() != owner) {
        throw LogicFailure{"TLS server connections must be created on their owner event loop."_el};
    }
    const auto driver = _driver.lock();
    if (driver == nullptr) {
        throw LogicFailure{"The network backend is not attached to an event-loop driver."_el};
    }
    auto tcpConnection = std::make_shared<impl::TcpConnection>(owner, driver, _resolver);
    return std::make_shared<impl::TlsServerConnection>(owner, std::move(tcpConnection));
}

auto NetworkBackend::createUdpSocket() -> UdpSocketPtr {
    const auto owner = ownerLoop();
    if (event::currentLoop() != owner) {
        throw LogicFailure{"UDP sockets must be created on their owner event loop."_el};
    }
    const auto driver = _driver.lock();
    if (driver == nullptr) {
        throw LogicFailure{"The network backend is not attached to an event-loop driver."_el};
    }
    return std::make_shared<impl::UdpSocket>(owner, driver);
}

auto NetworkBackend::ownerLoop() const -> event::EventTargetPtr {
    const auto target = _target.lock();
    const auto result = std::dynamic_pointer_cast<event::EventTarget>(target);
    if (result == nullptr) {
        throw LogicFailure{"The network backend is not attached to an event loop."_el};
    }
    return result;
}

}
