// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/NetworkError.hpp>
#include <erbsland/network/TcpConnection.hpp>
#include <erbsland/network/TcpConnectionAttempt.hpp>
#include <erbsland/network/TcpConnectionAttemptEvents.hpp>
#include <erbsland/network/TcpConnectionEvents.hpp>
#include <erbsland/network/UdpPeer.hpp>
#include <erbsland/network/UdpPeerEvents.hpp>
#include <erbsland/network/UdpSocket.hpp>
#include <erbsland/network/UdpSocketEvents.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(TcpConnectionAttempt TcpConnectionAttemptEvents UdpSocketEvents UdpPeerEvents UdpDatagram NetworkError)
class NetworkProtocolFacadeTest final : public el::UnitTest {
    class ConnectionEvents final : public TcpConnectionEvents {
    public:
        explicit ConnectionEvents(EventsPtr target) : TcpConnectionEvents{std::move(target)} {}
        auto onData(NetworkDataFn callback) -> TcpConnectionEvents & override {
            data = std::move(callback);
            return *this;
        }
        auto onWritable(NetworkEventFn callback) -> TcpConnectionEvents & override {
            writable = std::move(callback);
            return *this;
        }
        auto onClosed(NetworkEventFn callback) -> TcpConnectionEvents & override {
            closed = std::move(callback);
            return *this;
        }
        auto onError(NetworkErrorFn callback) -> TcpConnectionEvents & override {
            error = std::move(callback);
            return *this;
        }
        NetworkDataFn data;
        NetworkEventFn writable;
        NetworkEventFn closed;
        NetworkErrorFn error;
    };

    class Connection final : public TcpConnection {
    public:
        explicit Connection(EventsPtr owner) : TcpConnection{std::move(owner)} {}
        auto localEndpoint() const noexcept -> const IpEndpoint & override { return _local; }
        auto remoteEndpoint() const noexcept -> const IpEndpoint & override { return _remote; }
        auto bufferLimits() const noexcept -> SocketBufferLimits override { return {}; }
        auto state() const noexcept -> NetworkSourceState override { return _state; }
        void start() override { _state = NetworkSourceState::Active; }
        auto send([[maybe_unused]] el::mem::ByteBlock data) -> NetworkSendStatus override {
            return _state == NetworkSourceState::Active ? NetworkSendStatus::Accepted : NetworkSendStatus::Closed;
        }
        void pauseReceiving() override {}
        void resumeReceiving() override {}
        void close() override { _state = NetworkSourceState::Closed; }
        void abort() noexcept override { _state = NetworkSourceState::Closed; }
        auto events() -> TcpConnectionEventsPtr override {
            auto result = std::make_shared<ConnectionEvents>(currentOwnerEvents());
            registerEditor(result);
            return result;
        }

    private:
        IpEndpoint _local{IpAddress::loopbackV4(), Port{40000U}};
        IpEndpoint _remote{IpAddress::fromStringOrThrow("192.0.2.80"_el), Port{443U}};
        NetworkSourceState _state{NetworkSourceState::Inactive};
    };

    class AttemptEvents final : public TcpConnectionAttemptEvents {
    public:
        explicit AttemptEvents(EventsPtr target) : TcpConnectionAttemptEvents{std::move(target)} {}
        auto onConnected(TcpConnectionFn callback) -> TcpConnectionAttemptEvents & override {
            connected = std::move(callback);
            return *this;
        }
        auto onError(NetworkErrorFn callback) -> TcpConnectionAttemptEvents & override {
            error = std::move(callback);
            return *this;
        }
        TcpConnectionFn connected;
        NetworkErrorFn error;
    };

    class Attempt final : public TcpConnectionAttempt {
    public:
        Attempt(EventsPtr owner, HostEndpoint remote) :
            TcpConnectionAttempt{std::move(owner)}, _remote{std::move(remote)} {}
        auto remoteEndpoint() const noexcept -> const HostEndpoint & override { return _remote; }
        auto state() const noexcept -> NetworkSourceState override { return _state; }
        void start() override {
            _state = NetworkSourceState::Starting;
            _tried.emplace_back(IpAddress::fromStringOrThrow("2001:db8::80"_el));
            _tried.emplace_back(IpAddress::fromStringOrThrow("192.0.2.80"_el));
            const auto connection = std::make_shared<Connection>(ownerEvents());
            for (const auto &editor : connectedEditors<AttemptEvents>()) {
                if (editor->connected) {
                    editor->connected(connection);
                }
            }
            _state = NetworkSourceState::Closed;
        }
        void cancel() noexcept override { _state = NetworkSourceState::Closed; }
        auto events() -> TcpConnectionAttemptEventsPtr override {
            auto result = std::make_shared<AttemptEvents>(currentOwnerEvents());
            registerEditor(result);
            return result;
        }
        auto tried() const noexcept -> const std::vector<IpAddress> & { return _tried; }

    private:
        HostEndpoint _remote;
        std::vector<IpAddress> _tried;
        NetworkSourceState _state{NetworkSourceState::Inactive};
    };

    class SocketEvents final : public UdpSocketEvents {
    public:
        explicit SocketEvents(EventsPtr target) : UdpSocketEvents{std::move(target)} {}
        auto onBound(NetworkEventFn callback) -> UdpSocketEvents & override {
            bound = std::move(callback);
            return *this;
        }
        auto onDatagram(UdpDatagramFn callback) -> UdpSocketEvents & override {
            datagram = std::move(callback);
            return *this;
        }
        auto onWritable(NetworkEventFn callback) -> UdpSocketEvents & override {
            writable = std::move(callback);
            return *this;
        }
        auto onClosed(NetworkEventFn callback) -> UdpSocketEvents & override {
            closed = std::move(callback);
            return *this;
        }
        auto onError(NetworkErrorFn callback) -> UdpSocketEvents & override {
            error = std::move(callback);
            return *this;
        }
        NetworkEventFn bound;
        UdpDatagramFn datagram;
        NetworkEventFn writable;
        NetworkEventFn closed;
        NetworkErrorFn error;
    };

    class Socket final : public UdpSocket {
    public:
        Socket(EventsPtr owner, SocketBufferLimits limits) : UdpSocket{std::move(owner)}, _limits{limits} {}
        auto localEndpoint() const noexcept -> const IpEndpoint & override { return _local; }
        auto bufferLimits() const noexcept -> SocketBufferLimits override { return _limits; }
        auto state() const noexcept -> NetworkSourceState override { return _state; }
        void start() override {
            _state = NetworkSourceState::Active;
            for (const auto &editor : connectedEditors<SocketEvents>()) {
                if (editor->bound) {
                    editor->bound();
                }
            }
        }
        auto send(UdpDatagram datagram) -> NetworkSendStatus override {
            if (_state != NetworkSourceState::Active) {
                return NetworkSendStatus::Closed;
            }
            if (_queued.toSizeT() + datagram.data().length().toSizeT() > _limits.send().toSizeT()) {
                _backPressure = true;
                return NetworkSendStatus::BackPressure;
            }
            _queued += datagram.data().length();
            return NetworkSendStatus::Accepted;
        }
        void pauseReceiving() override { _paused = true; }
        void resumeReceiving() override { _paused = false; }
        void close() override {
            if (_state == NetworkSourceState::Closed) {
                return;
            }
            _state = NetworkSourceState::Closed;
            for (const auto &editor : connectedEditors<SocketEvents>()) {
                if (editor->closed) {
                    editor->closed();
                }
            }
        }
        void abort() noexcept override { _state = NetworkSourceState::Closed; }
        auto events() -> UdpSocketEventsPtr override {
            auto result = std::make_shared<SocketEvents>(currentOwnerEvents());
            registerEditor(result);
            return result;
        }
        void emit(UdpDatagram datagram) {
            if (_paused) {
                return;
            }
            for (const auto &editor : connectedEditors<SocketEvents>()) {
                if (editor->datagram) {
                    editor->datagram(std::move(datagram));
                }
            }
        }
        void drain() {
            _queued = {};
            if (!_backPressure) {
                return;
            }
            _backPressure = false;
            for (const auto &editor : connectedEditors<SocketEvents>()) {
                if (editor->writable) {
                    editor->writable();
                }
            }
        }
        void fail() {
            const auto error =
                NetworkError{NetworkErrorContext{"UDP receive failed"_el, "The mock receive failed."_el}};
            for (const auto &editor : connectedEditors<SocketEvents>()) {
                if (editor->error) {
                    editor->error(error);
                }
            }
            _state = NetworkSourceState::Failed;
        }

    private:
        IpEndpoint _local{IpAddress::loopbackV4(), Port{9000U}};
        SocketBufferLimits _limits;
        el::unit::ByteLength _queued;
        NetworkSourceState _state{NetworkSourceState::Inactive};
        bool _paused{false};
        bool _backPressure{false};
    };

    class PeerEvents final : public UdpPeerEvents {
    public:
        explicit PeerEvents(EventsPtr target) : UdpPeerEvents{std::move(target)} {}
        auto onReady(NetworkEventFn callback) -> UdpPeerEvents & override {
            ready = std::move(callback);
            return *this;
        }
        auto onData(NetworkDataFn callback) -> UdpPeerEvents & override {
            data = std::move(callback);
            return *this;
        }
        auto onWritable(NetworkEventFn callback) -> UdpPeerEvents & override {
            writable = std::move(callback);
            return *this;
        }
        auto onClosed(NetworkEventFn callback) -> UdpPeerEvents & override {
            closed = std::move(callback);
            return *this;
        }
        auto onError(NetworkErrorFn callback) -> UdpPeerEvents & override {
            error = std::move(callback);
            return *this;
        }
        NetworkEventFn ready;
        NetworkDataFn data;
        NetworkEventFn writable;
        NetworkEventFn closed;
        NetworkErrorFn error;
    };

    class Peer final : public UdpPeer {
    public:
        Peer(EventsPtr owner, HostEndpoint remote) : UdpPeer{std::move(owner)}, _remoteHost{std::move(remote)} {}
        auto remoteHost() const noexcept -> const HostEndpoint & override { return _remoteHost; }
        auto remoteEndpoint() const noexcept -> const std::optional<IpEndpoint> & override { return _remote; }
        auto bufferLimits() const noexcept -> SocketBufferLimits override { return {}; }
        auto state() const noexcept -> NetworkSourceState override { return _state; }
        void start() override {
            _state = NetworkSourceState::Active;
            _remote = IpEndpoint{IpAddress::fromStringOrThrow("192.0.2.53"_el), _remoteHost.port()};
            for (const auto &editor : connectedEditors<PeerEvents>()) {
                if (editor->ready) {
                    editor->ready();
                }
            }
        }
        auto send([[maybe_unused]] el::mem::ByteBlock data) -> NetworkSendStatus override {
            return _state == NetworkSourceState::Active ? NetworkSendStatus::Accepted : NetworkSendStatus::Closed;
        }
        void pauseReceiving() override { _paused = true; }
        void resumeReceiving() override { _paused = false; }
        void close() override { _state = NetworkSourceState::Closed; }
        void abort() noexcept override { _state = NetworkSourceState::Closed; }
        auto events() -> UdpPeerEventsPtr override {
            auto result = std::make_shared<PeerEvents>(currentOwnerEvents());
            registerEditor(result);
            return result;
        }
        void emit(el::mem::ByteBlock data) {
            if (_paused) {
                return;
            }
            for (const auto &editor : connectedEditors<PeerEvents>()) {
                if (editor->data) {
                    editor->data(data);
                }
            }
        }

    private:
        HostEndpoint _remoteHost;
        std::optional<IpEndpoint> _remote;
        NetworkSourceState _state{NetworkSourceState::Inactive};
        bool _paused{false};
    };

public:
    void testOutgoingAttemptPreservesResolverOrder() {
        const auto loop = EventLoop::create();
        const auto attempt = std::make_shared<Attempt>(loop, HostEndpoint::fromStringOrThrow("service.test:443"_el));
        auto editor = TcpConnectionAttemptEventsPtr{};
        auto connection = TcpConnectionPtr{};
        loop->invoke([&]() -> void {
            editor = attempt->events();
            editor->onConnected([&connection](TcpConnectionPtr result) -> void { connection = std::move(result); });
            REQUIRE_EQUAL(attempt->state(), NetworkSourceState::Inactive);
            attempt->start();
        });

        REQUIRE(loop->runOnce());
        REQUIRE_EQUAL(attempt->tried().size(), std::size_t{2U});
        REQUIRE_EQUAL(attempt->tried()[0].toString(), "2001:db8::80"_el);
        REQUIRE_EQUAL(attempt->tried()[1].toString(), "192.0.2.80"_el);
        REQUIRE(connection != nullptr);
        REQUIRE_EQUAL(connection->state(), NetworkSourceState::Inactive);
    }

    void testUdpSocketPreservesDatagramsAndBackPressure() {
        const auto loop = EventLoop::create();
        const auto limits = SocketBufferLimits{el::unit::ByteLength{4U}, el::unit::ByteLength{4U}};
        const auto socket = std::make_shared<Socket>(loop, limits);
        auto editor = UdpSocketEventsPtr{};
        auto received = std::size_t{};
        auto remote = IpEndpoint{};
        auto writable = false;
        auto closed = 0;
        loop->invoke([&]() -> void {
            editor = socket->events();
            editor->onDatagram([&](UdpDatagram datagram) -> void {
                received += datagram.data().length().toSizeT();
                remote = datagram.remoteEndpoint();
            });
            editor->onWritable([&]() -> void { writable = true; });
            editor->onClosed([&]() -> void { closed += 1; });
            socket->start();
            const auto endpoint = IpEndpoint::fromStringOrThrow("192.0.2.20:53"_el);
            REQUIRE(socket->send(UdpDatagram{endpoint, el::mem::ByteBlock{el::unit::ByteLength{4U}}}).isAccepted());
            REQUIRE(
                socket->send(UdpDatagram{endpoint, el::mem::ByteBlock{el::unit::ByteLength{1U}}}).hasBackPressure());
            socket->pauseReceiving();
            socket->emit(UdpDatagram{endpoint, el::mem::ByteBlock{el::unit::ByteLength{2U}}});
            socket->resumeReceiving();
            socket->emit(UdpDatagram{endpoint, el::mem::ByteBlock{el::unit::ByteLength{3U}}});
            socket->drain();
            socket->close();
            socket->close();
        });

        REQUIRE(loop->runOnce());
        REQUIRE_EQUAL(received, std::size_t{3U});
        REQUIRE_EQUAL(remote.toString(), "192.0.2.20:53"_el);
        REQUIRE(writable);
        REQUIRE_EQUAL(closed, 1);
    }

    void testUdpPeerResolutionPayloadAndTerminalError() {
        const auto loop = EventLoop::create();
        const auto peer = std::make_shared<Peer>(loop, HostEndpoint::fromStringOrThrow("dns.test:53"_el));
        const auto socket = std::make_shared<Socket>(loop, SocketBufferLimits{});
        auto peerEditor = UdpPeerEventsPtr{};
        auto socketEditor = UdpSocketEventsPtr{};
        auto ready = false;
        auto received = std::size_t{};
        auto errorSeenBeforeFailed = false;
        loop->invoke([&]() -> void {
            peerEditor = peer->events();
            peerEditor->onReady([&]() -> void { ready = peer->remoteEndpoint().has_value(); });
            peerEditor->onData([&](el::mem::ByteBlock data) -> void { received += data.length().toSizeT(); });
            peer->start();
            REQUIRE(peer->send(el::mem::ByteBlock{el::unit::ByteLength{2U}}).isAccepted());
            peer->emit(el::mem::ByteBlock{el::unit::ByteLength{3U}});

            socketEditor = socket->events();
            socketEditor->onError([&]([[maybe_unused]] const NetworkError &error) -> void {
                errorSeenBeforeFailed = socket->state() != NetworkSourceState::Failed;
            });
            socket->start();
            socket->fail();
        });

        REQUIRE(loop->runOnce());
        REQUIRE(ready);
        REQUIRE_EQUAL(peer->remoteEndpoint()->toString(), "192.0.2.53:53"_el);
        REQUIRE_EQUAL(received, std::size_t{3U});
        REQUIRE(errorSeenBeforeFailed);
        REQUIRE_EQUAL(socket->state(), NetworkSourceState::Failed);
    }
};
