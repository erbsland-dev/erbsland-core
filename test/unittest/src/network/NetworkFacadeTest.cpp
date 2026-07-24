// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/event/EventBackend.hpp>
#include <erbsland/event/EventBackendTarget.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/HostLookup.hpp>
#include <erbsland/network/HostLookupEvents.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/TcpConnection.hpp>
#include <erbsland/network/TcpConnectionEvents.hpp>
#include <erbsland/network/TcpConnectionRequest.hpp>
#include <erbsland/network/TcpListener.hpp>
#include <erbsland/network/TcpListenerEvents.hpp>
#include <erbsland/network/UdpPeer.hpp>
#include <erbsland/network/UdpSocket.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <optional>
#include <utility>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(Network HostLookup TcpListener TcpConnectionRequest TcpConnection UdpSocket UdpPeer NetworkSendStatus)
class NetworkFacadeTest final : public el::UnitTest {
    class LookupEvents final : public HostLookupEvents {
    public:
        explicit LookupEvents(EventsPtr target) : HostLookupEvents{std::move(target)} {}
        auto onResolved(HostResolvedFn callback) -> HostLookupEvents & override {
            resolved = std::move(callback);
            return *this;
        }
        auto onError(NetworkErrorFn callback) -> HostLookupEvents & override {
            error = std::move(callback);
            return *this;
        }
        HostResolvedFn resolved;
        NetworkErrorFn error;
    };

    class Lookup final : public HostLookup {
    public:
        Lookup(EventsPtr owner, Host host) : HostLookup{std::move(owner)}, _host{std::move(host)} {}
        auto host() const noexcept -> const Host & override { return _host; }
        auto state() const noexcept -> NetworkSourceState override { return _state; }
        void start() override {
            _state = NetworkSourceState::Active;
            const auto address =
                _host.address().has_value() ? *_host.address() : IpAddress::fromStringOrThrow("192.0.2.10"_el);
            const auto addresses = el::util::List<IpAddress>{address};
            for (const auto &editor : connectedEditors<LookupEvents>()) {
                if (editor->resolved) {
                    editor->resolved(addresses);
                }
            }
            _state = NetworkSourceState::Closed;
        }
        void cancel() noexcept override { _state = NetworkSourceState::Closed; }
        auto events() -> HostLookupEventsPtr override {
            auto result = std::make_shared<LookupEvents>(currentOwnerEvents());
            registerEditor(result);
            return result;
        }

    private:
        Host _host;
        NetworkSourceState _state{NetworkSourceState::Inactive};
    };

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
        Connection(EventsPtr owner, SocketBufferLimits limits = {}) :
            TcpConnection{std::move(owner)}, _limits{limits} {}
        auto localEndpoint() const noexcept -> const IpEndpoint & override { return _local; }
        auto remoteEndpoint() const noexcept -> const IpEndpoint & override { return _remote; }
        auto bufferLimits() const noexcept -> SocketBufferLimits override { return _limits; }
        auto state() const noexcept -> NetworkSourceState override { return _state; }
        void start() override { _state = NetworkSourceState::Active; }
        auto send(el::mem::ByteBlock data) -> NetworkSendStatus override {
            if (_state != NetworkSourceState::Active) {
                return NetworkSendStatus::Closed;
            }
            if (_queued.toSizeT() + data.length().toSizeT() > _limits.send().toSizeT()) {
                _backPressure = true;
                return NetworkSendStatus::BackPressure;
            }
            _queued += data.length();
            return NetworkSendStatus::Accepted;
        }
        void pauseReceiving() override { _paused = true; }
        void resumeReceiving() override { _paused = false; }
        void close() override {
            _state = NetworkSourceState::Closed;
            for (const auto &editor : connectedEditors<ConnectionEvents>()) {
                if (editor->closed) {
                    editor->closed();
                }
            }
        }
        void abort() noexcept override { _state = NetworkSourceState::Closed; }
        auto events() -> TcpConnectionEventsPtr override {
            auto result = std::make_shared<ConnectionEvents>(currentOwnerEvents());
            registerEditor(result);
            return result;
        }
        void emitData(el::mem::ByteBlock data) {
            if (_paused || _state != NetworkSourceState::Active) {
                return;
            }
            for (const auto &editor : connectedEditors<ConnectionEvents>()) {
                if (editor->data) {
                    editor->data(data);
                }
            }
        }
        void drain() {
            _queued = {};
            if (!_backPressure) {
                return;
            }
            _backPressure = false;
            for (const auto &editor : connectedEditors<ConnectionEvents>()) {
                if (editor->writable) {
                    editor->writable();
                }
            }
        }

    private:
        IpEndpoint _local{IpAddress::loopbackV4(), Port{8080U}};
        IpEndpoint _remote{IpAddress::fromStringOrThrow("192.0.2.20"_el), Port{50000U}};
        SocketBufferLimits _limits;
        el::unit::ByteLength _queued;
        NetworkSourceState _state{NetworkSourceState::Inactive};
        bool _paused{false};
        bool _backPressure{false};
    };

    class Request final : public TcpConnectionRequest {
    public:
        explicit Request(EventsPtr owner, std::shared_ptr<bool> rejectedFlag = {}) :
            TcpConnectionRequest{std::move(owner)}, _rejectedFlag{std::move(rejectedFlag)} {}
        ~Request() override {
            if (_state == NetworkSourceState::Inactive) {
                reject();
            }
        }
        auto remoteEndpoint() const noexcept -> const IpEndpoint & override { return _remote; }
        auto state() const noexcept -> NetworkSourceState override { return _state; }
        void accept(EventsPtr ownerEvents, TcpConnectionFn setupCallback) override {
            _state = NetworkSourceState::Closed;
            ownerEvents->invoke(
                [ownerEvents = std::move(ownerEvents), callback = std::move(setupCallback)]() mutable -> void {
                    callback(std::make_shared<Connection>(std::move(ownerEvents)));
                });
        }
        void reject() noexcept override {
            rejected = true;
            if (_rejectedFlag != nullptr) {
                *_rejectedFlag = true;
            }
            _state = NetworkSourceState::Closed;
        }
        bool rejected{false};

    private:
        IpEndpoint _remote{IpAddress::fromStringOrThrow("192.0.2.30"_el), Port{51000U}};
        NetworkSourceState _state{NetworkSourceState::Inactive};
        std::shared_ptr<bool> _rejectedFlag;
    };

    class ListenerEvents final : public TcpListenerEvents {
    public:
        explicit ListenerEvents(EventsPtr target) : TcpListenerEvents{std::move(target)} {}
        auto onListening(NetworkEventFn callback) -> TcpListenerEvents & override {
            listening = std::move(callback);
            return *this;
        }
        auto onConnection(TcpConnectionRequestFn callback) -> TcpListenerEvents & override {
            connection = std::move(callback);
            return *this;
        }
        auto onClosed(NetworkEventFn callback) -> TcpListenerEvents & override {
            closed = std::move(callback);
            return *this;
        }
        auto onError(NetworkErrorFn callback) -> TcpListenerEvents & override {
            error = std::move(callback);
            return *this;
        }
        NetworkEventFn listening;
        TcpConnectionRequestFn connection;
        NetworkEventFn closed;
        NetworkErrorFn error;
    };

    class Listener final : public TcpListener {
    public:
        Listener(EventsPtr owner, IpEndpoint local) : TcpListener{std::move(owner)}, _local{std::move(local)} {}
        auto localEndpoint() const noexcept -> const IpEndpoint & override { return _local; }
        auto state() const noexcept -> NetworkSourceState override { return _state; }
        void start() override {
            _state = NetworkSourceState::Active;
            for (const auto &editor : connectedEditors<ListenerEvents>()) {
                if (editor->listening) {
                    editor->listening();
                }
            }
        }
        void pauseAccepting() override { _paused = true; }
        void resumeAccepting() override { _paused = false; }
        void close() override { _state = NetworkSourceState::Closed; }
        void abort() noexcept override { _state = NetworkSourceState::Closed; }
        auto events() -> TcpListenerEventsPtr override {
            auto result = std::make_shared<ListenerEvents>(currentOwnerEvents());
            registerEditor(result);
            return result;
        }
        void emitRequest() {
            if (_paused || _state != NetworkSourceState::Active) {
                return;
            }
            for (const auto &editor : connectedEditors<ListenerEvents>()) {
                if (editor->connection) {
                    editor->connection(std::make_shared<Request>(ownerEvents()));
                }
            }
        }

    private:
        IpEndpoint _local;
        NetworkSourceState _state{NetworkSourceState::Inactive};
        bool _paused{false};
    };

    class Backend final : public EventBackend, public Network {
    public:
        auto backendId() const noexcept -> EventBackendId override { return id::NetworkBackend; }
        void attach(EventBackendTargetWeakPtr target, [[maybe_unused]] EventLoopDriverWeakPtr driver) override {
            _target = std::move(target);
        }
        void poll([[maybe_unused]] el::time::TimePoint now) override {}
        auto handleEvent([[maybe_unused]] const Event &event) -> bool override { return false; }
        auto nextWakeTime() const -> std::optional<el::time::TimePoint> override { return std::nullopt; }
        auto createHostLookup(Host host) -> HostLookupPtr override {
            return std::make_shared<Lookup>(events(), std::move(host));
        }
        auto createTcpListener(IpEndpoint localEndpoint, [[maybe_unused]] std::size_t backlog)
            -> TcpListenerPtr override {
            lastListener = std::make_shared<Listener>(events(), std::move(localEndpoint));
            return lastListener;
        }
        auto createTcpConnection(
            [[maybe_unused]] HostEndpoint remoteEndpoint, [[maybe_unused]] SocketBufferLimits bufferLimits)
            -> TcpConnectionAttemptPtr override {
            return {};
        }
        auto createUdpSocket(
            [[maybe_unused]] IpEndpoint localEndpoint, [[maybe_unused]] SocketBufferLimits bufferLimits)
            -> UdpSocketPtr override {
            return {};
        }
        auto createUdpPeer(
            [[maybe_unused]] HostEndpoint remoteEndpoint, [[maybe_unused]] SocketBufferLimits bufferLimits)
            -> UdpPeerPtr override {
            return {};
        }
        auto events() -> EventsPtr {
            const auto target = _target.lock();
            const auto result = std::dynamic_pointer_cast<Events>(target);
            if (result == nullptr) {
                throw el::err::LogicError{"The mock network backend has no event target."};
            }
            return result;
        }

        std::shared_ptr<Listener> lastListener;

    private:
        EventBackendTargetWeakPtr _target;
    };

public:
    void testDefaultNetworkBackendIsUnavailable() {
        const auto loop = EventLoop::create();
        REQUIRE_THROWS_AS(el::err::ParameterError, loop->get<Network>());
    }

    void testLookupRequiresStartAndEditorLifetime() {
        const auto loop = EventLoop::create();
        loop->registerBackend(std::make_unique<Backend>());
        auto lookup = HostLookupPtr{};
        auto editor = HostLookupEventsPtr{};
        auto resultCount = std::size_t{};

        loop->invoke([&]() -> void {
            lookup = loop->get<Network>().createHostLookup(Host::fromStringOrThrow("example.test"_el));
            editor = lookup->events();
            editor->onResolved(
                [&](const el::util::List<IpAddress> &addresses) -> void { resultCount = addresses.count().toSizeT(); });
            REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Inactive);
            lookup->start();
        });

        REQUIRE(loop->runOnce());
        REQUIRE_EQUAL(resultCount, std::size_t{1U});
        REQUIRE_EQUAL(lookup->state(), NetworkSourceState::Closed);
        editor.reset();
    }

    void testAcceptHandoffAndConnectionFlowControl() {
        const auto listenerLoop = EventLoop::create();
        auto backend = std::make_unique<Backend>();
        const auto backendPtr = backend.get();
        listenerLoop->registerBackend(std::move(backend));
        const auto workerLoop = EventLoop::create();
        auto listenerEditor = TcpListenerEventsPtr{};
        auto connectionEditor = TcpConnectionEventsPtr{};
        auto connection = std::shared_ptr<Connection>{};
        auto listening = false;
        auto writable = false;
        auto received = std::size_t{};

        listenerLoop->invoke([&]() -> void {
            const auto listener =
                listenerLoop->get<Network>().createTcpListener(IpEndpoint{IpAddress::loopbackV4(), Port{8080U}});
            listenerEditor = listener->events();
            listenerEditor->onListening([&]() -> void { listening = true; });
            listenerEditor->onConnection([&](TcpConnectionRequestPtr request) -> void {
                request->accept(workerLoop, [&](TcpConnectionPtr accepted) -> void {
                    connection = std::dynamic_pointer_cast<Connection>(accepted);
                    connectionEditor = connection->events();
                    connectionEditor->onData(
                        [&](el::mem::ByteBlock data) -> void { received += data.length().toSizeT(); });
                    connectionEditor->onWritable([&]() -> void { writable = true; });
                    connection->start();
                    connection->pauseReceiving();
                    connection->emitData(el::mem::ByteBlock{el::unit::ByteLength{2U}});
                    connection->resumeReceiving();
                    connection->emitData(el::mem::ByteBlock{el::unit::ByteLength{2U}});
                });
            });
            listener->start();
            backendPtr->lastListener->emitRequest();
        });

        REQUIRE(listenerLoop->runOnce());
        REQUIRE(listening);
        REQUIRE(workerLoop->runOnce());
        REQUIRE(connection != nullptr);
        REQUIRE_EQUAL(received, std::size_t{2U});

        const auto small = SocketBufferLimits{el::unit::ByteLength{4U}, el::unit::ByteLength{4U}};
        auto smallConnection = std::make_shared<Connection>(workerLoop, small);
        workerLoop->invoke([&]() -> void {
            connectionEditor = smallConnection->events();
            connectionEditor->onWritable([&]() -> void { writable = true; });
            smallConnection->start();
            REQUIRE(smallConnection->send(el::mem::ByteBlock{el::unit::ByteLength{4U}}).isAccepted());
            REQUIRE(smallConnection->send(el::mem::ByteBlock{el::unit::ByteLength{1U}}).hasBackPressure());
            smallConnection->drain();
        });
        REQUIRE(workerLoop->runOnce());
        REQUIRE(writable);
    }

    void testUndecidedRequestRejectsOnDestruction() {
        const auto loop = EventLoop::create();
        const auto rejected = std::make_shared<bool>(false);
        loop->invoke([&]() -> void {
            auto request = std::make_shared<Request>(loop, rejected);
            request.reset();
        });
        REQUIRE(loop->runOnce());
        REQUIRE(*rejected);
    }
};
