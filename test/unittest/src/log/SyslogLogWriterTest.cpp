// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventBackend.hpp>
#include <erbsland/event/EventBackendTarget.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/log/LogEntry.hpp>
#include <erbsland/log/LogLine.hpp>
#include <erbsland/log/SyslogLogWriter.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/source/ConnectionCloseContext.hpp>
#include <erbsland/network/tcp/TcpAcceptOptions.hpp>
#include <erbsland/network/tcp/TcpConnection.hpp>
#include <erbsland/network/tcp/TcpConnectionEventEditor.hpp>
#include <erbsland/network/tls/TlsClientConnection.hpp>
#include <erbsland/network/tls/TlsClientConnectionEventEditor.hpp>
#include <erbsland/network/udp/UdpSocket.hpp>
#include <erbsland/network/udp/UdpSocketEventEditor.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringDecoder.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace el::text::literals;

namespace erbsland::test::logtest {

struct FakeSyslogNetworkState final {
    std::vector<el::mem::ByteBlock> udpData;
    std::vector<el::mem::ByteBlock> tcpData;
    std::vector<el::mem::ByteBlock> tlsData;
    el::text::String tlsLabel;
    std::size_t tcpConnectionCount{};
    bool failFirstTcpSend{};
};

class FakeUdpSocketEventEditor final : public el::network::UdpSocketEventEditor {
public:
    FakeUdpSocketEventEditor(el::event::EventSourcePtr source, el::event::EventsPtr target) :
        UdpSocketEventEditor{std::move(source), std::move(target)} {}

    auto onBound(el::network::NetworkEventFn callback) -> FakeUdpSocketEventEditor & override {
        bound = std::move(callback);
        return *this;
    }
    auto onDatagram(el::network::UdpDatagramFn callback) -> FakeUdpSocketEventEditor & override {
        datagram = std::move(callback);
        return *this;
    }
    auto onDatagramDropped(el::network::UdpDatagramDropFn callback) -> FakeUdpSocketEventEditor & override {
        datagramDropped = std::move(callback);
        return *this;
    }
    auto onWritable(el::network::NetworkEventFn callback) -> FakeUdpSocketEventEditor & override {
        writable = std::move(callback);
        return *this;
    }
    auto onClosed(el::network::NetworkEventFn callback) -> FakeUdpSocketEventEditor & override {
        closed = std::move(callback);
        return *this;
    }
    auto onError(el::network::NetworkErrorFn callback) -> FakeUdpSocketEventEditor & override {
        error = std::move(callback);
        return *this;
    }

    el::network::NetworkEventFn bound;
    el::network::UdpDatagramFn datagram;
    el::network::UdpDatagramDropFn datagramDropped;
    el::network::NetworkEventFn writable;
    el::network::NetworkEventFn closed;
    el::network::NetworkErrorFn error;
};

class FakeUdpSocket final : public el::network::UdpSocket {
public:
    FakeUdpSocket(el::event::EventsPtr owner, std::shared_ptr<FakeSyslogNetworkState> state) :
        UdpSocket{std::move(owner)}, _state{std::move(state)} {}

    auto localEndpoint() const -> std::optional<el::network::IpEndpoint> override { return _localEndpoint; }
    auto state() const noexcept -> el::network::NetworkSourceState override { return _sourceState; }
    void start(el::network::IpEndpoint endpoint, el::network::UdpSocketOptions) override {
        _localEndpoint = std::move(endpoint);
        _sourceState = el::network::NetworkSourceState::Active;
        if (_events->bound) {
            _events->bound();
        }
    }
    auto send(const el::network::UdpDatagram &datagram) -> el::network::NetworkSendStatus override {
        _state->udpData.push_back(datagram.data());
        return el::network::NetworkSendStatus::Accepted;
    }
    void pauseReceiving() override {}
    void resumeReceiving() override {}
    void close() override {
        _sourceState = el::network::NetworkSourceState::Closed;
        if (_events && _events->closed) {
            _events->closed();
        }
    }
    void abort() noexcept override { _sourceState = el::network::NetworkSourceState::Closed; }
    auto events() -> el::network::UdpSocketEventEditor & override {
        if (!_events) {
            _events = std::make_unique<FakeUdpSocketEventEditor>(shared_from_this(), ownerEvents());
        }
        return *_events;
    }

private:
    std::shared_ptr<FakeSyslogNetworkState> _state;
    std::unique_ptr<FakeUdpSocketEventEditor> _events;
    std::optional<el::network::IpEndpoint> _localEndpoint;
    el::network::NetworkSourceState _sourceState{el::network::NetworkSourceState::Inactive};
};

class FakeTcpConnectionEventEditor final : public el::network::TcpConnectionEventEditor {
public:
    FakeTcpConnectionEventEditor(el::event::EventSourcePtr source, el::event::EventsPtr target) :
        TcpConnectionEventEditor{std::move(source), std::move(target)} {}

    auto onHostResolved(el::network::TcpHostResolvedFn callback) -> FakeTcpConnectionEventEditor & override {
        hostResolved = std::move(callback);
        return *this;
    }
    auto onConnected(el::network::NetworkEventFn callback) -> FakeTcpConnectionEventEditor & override {
        connected = std::move(callback);
        return *this;
    }
    auto onData(el::network::NetworkDataFn callback) -> FakeTcpConnectionEventEditor & override {
        data = std::move(callback);
        return *this;
    }
    auto onWritable(el::network::NetworkEventFn callback) -> FakeTcpConnectionEventEditor & override {
        writable = std::move(callback);
        return *this;
    }
    auto onClosed(el::network::ConnectionCloseFn callback) -> FakeTcpConnectionEventEditor & override {
        closed = std::move(callback);
        return *this;
    }
    auto onError(el::network::NetworkErrorFn callback) -> FakeTcpConnectionEventEditor & override {
        error = std::move(callback);
        return *this;
    }
    auto onFinal(el::network::NetworkEventFn callback) -> FakeTcpConnectionEventEditor & override {
        final = std::move(callback);
        return *this;
    }

    el::network::TcpHostResolvedFn hostResolved;
    el::network::NetworkEventFn connected;
    el::network::NetworkDataFn data;
    el::network::NetworkEventFn writable;
    el::network::ConnectionCloseFn closed;
    el::network::NetworkErrorFn error;
    el::network::NetworkEventFn final;
};

class FakeTcpConnection final : public el::network::TcpConnection {
public:
    FakeTcpConnection(el::event::EventsPtr owner, std::shared_ptr<FakeSyslogNetworkState> state, const bool failSend) :
        TcpConnection{std::move(owner)}, _state{std::move(state)}, _failSend{failSend} {}

    void connect(el::network::HostEndpoint endpoint, el::network::TcpConnectOptions) override {
        _requestedEndpoint = std::move(endpoint);
        _connectionState = el::network::ConnectionState::Active;
        if (_events->connected) {
            _events->connected();
        }
    }
    void accept(el::network::TcpConnectionRequestPtr, el::network::TcpAcceptOptions) override {}
    auto localEndpoint() const -> std::optional<el::network::IpEndpoint> override { return std::nullopt; }
    auto remoteEndpoint() const -> std::optional<el::network::IpEndpoint> override { return std::nullopt; }
    auto bufferLimits() const noexcept -> el::network::SocketBufferLimits override { return {}; }
    auto state() const noexcept -> el::network::ConnectionState override { return _connectionState; }
    auto send(const el::mem::ByteBlock &data) -> el::network::NetworkSendStatus override {
        if (_failSend) {
            _failSend = false;
            _connectionState = el::network::ConnectionState::Failed;
            return el::network::NetworkSendStatus::Closed;
        }
        _state->tcpData.push_back(data);
        return el::network::NetworkSendStatus::Accepted;
    }
    void pauseReceiving() override {}
    void resumeReceiving() override {}
    void close() override { _connectionState = el::network::ConnectionState::Closed; }
    void abort() noexcept override { _connectionState = el::network::ConnectionState::Closed; }
    auto events() -> el::network::TcpConnectionEventEditor & override {
        if (!_events) {
            _events = std::make_unique<FakeTcpConnectionEventEditor>(shared_from_this(), ownerEvents());
        }
        return *_events;
    }

private:
    std::shared_ptr<FakeSyslogNetworkState> _state;
    std::unique_ptr<FakeTcpConnectionEventEditor> _events;
    std::optional<el::network::HostEndpoint> _requestedEndpoint;
    el::network::ConnectionState _connectionState{el::network::ConnectionState::Inactive};
    bool _failSend{};
};

class FakeTlsConnectionEventEditor final : public el::network::TlsClientConnectionEventEditor {
public:
    FakeTlsConnectionEventEditor(el::event::EventSourcePtr source, el::event::EventsPtr target) :
        TlsClientConnectionEventEditor{std::move(source), std::move(target)} {}

    auto onHostResolved(el::network::TcpHostResolvedFn callback) -> FakeTlsConnectionEventEditor & override {
        hostResolved = std::move(callback);
        return *this;
    }
    auto onTransportConnected(el::network::NetworkEventFn callback) -> FakeTlsConnectionEventEditor & override {
        transportConnected = std::move(callback);
        return *this;
    }
    auto onPeerHello(el::network::NetworkEventFn callback) -> FakeTlsConnectionEventEditor & override {
        peerHello = std::move(callback);
        return *this;
    }
    auto onPeerAuthenticated(el::network::NetworkEventFn callback) -> FakeTlsConnectionEventEditor & override {
        peerAuthenticated = std::move(callback);
        return *this;
    }
    auto onHandshakeCompleted(el::network::NetworkEventFn callback) -> FakeTlsConnectionEventEditor & override {
        handshakeCompleted = std::move(callback);
        return *this;
    }
    auto onData(el::network::NetworkDataFn callback) -> FakeTlsConnectionEventEditor & override {
        data = std::move(callback);
        return *this;
    }
    auto onWritable(el::network::NetworkEventFn callback) -> FakeTlsConnectionEventEditor & override {
        writable = std::move(callback);
        return *this;
    }
    auto onClosed(el::network::ConnectionCloseFn callback) -> FakeTlsConnectionEventEditor & override {
        closed = std::move(callback);
        return *this;
    }
    auto onError(el::network::NetworkErrorFn callback) -> FakeTlsConnectionEventEditor & override {
        error = std::move(callback);
        return *this;
    }
    auto onFinal(el::network::NetworkEventFn callback) -> FakeTlsConnectionEventEditor & override {
        final = std::move(callback);
        return *this;
    }

    el::network::TcpHostResolvedFn hostResolved;
    el::network::NetworkEventFn transportConnected;
    el::network::NetworkEventFn peerHello;
    el::network::NetworkEventFn peerAuthenticated;
    el::network::NetworkEventFn handshakeCompleted;
    el::network::NetworkDataFn data;
    el::network::NetworkEventFn writable;
    el::network::ConnectionCloseFn closed;
    el::network::NetworkErrorFn error;
    el::network::NetworkEventFn final;
};

class FakeTlsConnection final : public el::network::TlsClientConnection {
public:
    FakeTlsConnection(el::event::EventsPtr owner, std::shared_ptr<FakeSyslogNetworkState> state) :
        TlsClientConnection{std::move(owner)}, _state{std::move(state)} {}

    auto requestedConfigurationLabel() const -> el::text::String override { return _state->tlsLabel; }
    auto matchedConfigurationLabel() const -> el::text::String override { return _state->tlsLabel; }
    auto requestedEndpoint() const -> std::optional<el::network::HostEndpoint> override { return _endpoint; }
    auto cipherSuite() const -> std::optional<el::cryptology::TlsCipherSuite> override { return std::nullopt; }
    auto negotiatedAlpn() const -> el::text::String override { return {}; }
    auto peerCertificatePath() const -> el::util::List<el::cryptology::X509Certificate> override { return {}; }
    void connect(el::network::HostEndpoint endpoint, el::network::TlsClientConnectOptions options) override {
        _endpoint = std::move(endpoint);
        _state->tlsLabel = options.configurationLabel();
        _connectionState = el::network::ConnectionState::Active;
        if (_events->handshakeCompleted) {
            _events->handshakeCompleted();
        }
    }
    auto localEndpoint() const -> std::optional<el::network::IpEndpoint> override { return std::nullopt; }
    auto remoteEndpoint() const -> std::optional<el::network::IpEndpoint> override { return std::nullopt; }
    auto bufferLimits() const noexcept -> el::network::SocketBufferLimits override { return {}; }
    auto state() const noexcept -> el::network::ConnectionState override { return _connectionState; }
    auto send(const el::mem::ByteBlock &data) -> el::network::NetworkSendStatus override {
        _state->tlsData.push_back(data);
        return el::network::NetworkSendStatus::Accepted;
    }
    void pauseReceiving() override {}
    void resumeReceiving() override {}
    void close() override { _connectionState = el::network::ConnectionState::Closed; }
    void abort() noexcept override { _connectionState = el::network::ConnectionState::Closed; }
    auto events() -> el::network::TlsClientConnectionEventEditor & override {
        if (!_events) {
            _events = std::make_unique<FakeTlsConnectionEventEditor>(shared_from_this(), ownerEvents());
        }
        return *_events;
    }

private:
    std::shared_ptr<FakeSyslogNetworkState> _state;
    std::unique_ptr<FakeTlsConnectionEventEditor> _events;
    std::optional<el::network::HostEndpoint> _endpoint;
    el::network::ConnectionState _connectionState{el::network::ConnectionState::Inactive};
};

class FakeSyslogNetworkBackend final : public el::event::EventBackend, public el::network::Network {
public:
    explicit FakeSyslogNetworkBackend(std::shared_ptr<FakeSyslogNetworkState> state) : _state{std::move(state)} {}

    auto backendId() const noexcept -> el::event::EventBackendId override { return el::network::Network::backendId(); }
    void attach(el::event::EventBackendTargetWeakPtr target, el::event::EventLoopDriverWeakPtr) override {
        _owner = std::dynamic_pointer_cast<el::event::Events>(target.lock());
    }
    void poll(el::time::TimePoint) override {}
    auto handleEvent(const el::event::Event &) -> bool override { return false; }
    auto nextWakeTime() const -> std::optional<el::time::TimePoint> override { return std::nullopt; }

    auto createHostLookup() -> el::network::HostLookupPtr override { return {}; }
    auto createHttpClientSession() -> el::network::HttpClientSessionPtr override { return {}; }
    auto createHttpServer() -> el::network::HttpServerPtr override { return {}; }
    auto createTcpListener() -> el::network::TcpListenerPtr override { return {}; }
    auto createTcpConnection() -> el::network::TcpConnectionPtr override {
        const auto fail = _state->failFirstTcpSend && _state->tcpConnectionCount == 0U;
        ++_state->tcpConnectionCount;
        return std::make_shared<FakeTcpConnection>(_owner, _state, fail);
    }
    auto createTlsClientConnection() -> el::network::TlsClientConnectionPtr override {
        return std::make_shared<FakeTlsConnection>(_owner, _state);
    }
    auto createTlsServerConnection() -> el::network::TlsServerConnectionPtr override { return {}; }
    auto createUdpSocket() -> el::network::UdpSocketPtr override {
        return std::make_shared<FakeUdpSocket>(_owner, _state);
    }

private:
    std::shared_ptr<FakeSyslogNetworkState> _state;
    el::event::EventsPtr _owner;
};

}

using namespace erbsland::test::logtest;

TESTED_TARGETS(SyslogLogWriter)
class SyslogLogWriterTest final : public el::UnitTest {
public:
    void testUdpTcpAndTlsUseFakeNetworkSources() {
        const auto state = std::make_shared<FakeSyslogNetworkState>();
        const auto loop = el::event::EventLoop::create();
        loop->registerBackend(std::make_unique<FakeSyslogNetworkBackend>(state));
        auto writers = std::vector<std::shared_ptr<el::log::SyslogLogWriter>>{};
        const auto entry = makeEntry();
        const auto line = makeLine();
        loop->invoke([&]() -> void {
            auto udpOptions = makeOptions(el::log::SyslogTransport::Udp);
            auto tcpOptions = makeOptions(el::log::SyslogTransport::Tcp);
            auto tlsOptions = makeOptions(el::log::SyslogTransport::Tls);
            tlsOptions.setTlsConfigurationLabel("custom/syslog"_el);
            writers.push_back(std::make_shared<el::log::SyslogLogWriter>(udpOptions));
            writers.push_back(std::make_shared<el::log::SyslogLogWriter>(tcpOptions));
            writers.push_back(std::make_shared<el::log::SyslogLogWriter>(tlsOptions));
            for (const auto &writer : writers) {
                writer->write(entry, line);
            }
        });
        REQUIRE(loop->runOnce());

        const auto message = el::log::SyslogLogWriter::formatMessage(*entry, *line, makeOptions({}));
        const auto framed = el::log::SyslogLogWriter::frameMessage(message);
        REQUIRE_EQUAL(state->udpData.size(), std::size_t{1U});
        REQUIRE_EQUAL(state->tcpData.size(), std::size_t{1U});
        REQUIRE_EQUAL(state->tlsData.size(), std::size_t{1U});
        REQUIRE_EQUAL(decode(state->udpData.front()), message);
        REQUIRE_EQUAL(decode(state->tcpData.front()), framed);
        REQUIRE_EQUAL(decode(state->tlsData.front()), framed);
        REQUIRE_EQUAL(state->tlsLabel, "custom/syslog"_el);
        writers.clear();
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testTcpReconnectRetainsPendingMessage() {
        const auto state = std::make_shared<FakeSyslogNetworkState>();
        state->failFirstTcpSend = true;
        const auto loop = el::event::EventLoop::create();
        loop->registerBackend(std::make_unique<FakeSyslogNetworkBackend>(state));
        auto writer = std::shared_ptr<el::log::SyslogLogWriter>{};
        const auto entry = makeEntry();
        const auto line = makeLine();
        loop->invoke([&]() -> void {
            writer = std::make_shared<el::log::SyslogLogWriter>(makeOptions(el::log::SyslogTransport::Tcp));
            writer->write(entry, line);
        });
        REQUIRE(loop->runOnce());
        REQUIRE_EQUAL(state->tcpConnectionCount, std::size_t{1U});
        REQUIRE(state->tcpData.empty());
        REQUIRE(loop->runOnce());

        REQUIRE_EQUAL(state->tcpConnectionCount, std::size_t{2U});
        REQUIRE_EQUAL(state->tcpData.size(), std::size_t{1U});
        REQUIRE_EQUAL(
            decode(state->tcpData.front()),
            el::log::SyslogLogWriter::frameMessage(
                el::log::SyslogLogWriter::formatMessage(*entry, *line, makeOptions({}))));
        writer.reset();
    }

private:
    [[nodiscard]] static auto makeOptions(const el::log::SyslogTransport transport) -> el::log::SyslogLogWriterOptions {
        auto result = el::log::SyslogLogWriterOptions{};
        result.setTransport(transport)
            .setEndpoint(el::network::HostEndpoint::fromStringOrThrow("127.0.0.1:514"_el))
            .setHostName("host"_el)
            .setApplicationName("app"_el)
            .setProcessId("42"_el)
            .setMessageId("event"_el);
        return result;
    }

    [[nodiscard]] static auto makeEntry() -> el::log::LogEntryConstPtr {
        return std::make_shared<el::log::LogEntry>(
            1U,
            el::time::DateTime{
                el::time::Date::fromYearMonthDay(2026, 8, 31),
                el::time::Time{el::time::Hour{12}, el::time::Minute{34}, el::time::Second{56}}},
            el::log::LogLevel::Information,
            el::log::LogPath{"application"_el},
            "message"_el);
    }

    [[nodiscard]] static auto makeLine() -> el::log::LogLineConstPtr {
        return std::make_shared<el::log::LogLine>(
            std::vector<el::log::LogLineSegment>{{el::log::LogLinePart::Message, "message"_el}});
    }

    [[nodiscard]] static auto decode(const el::mem::ByteBlock &data) -> el::text::String {
        return el::text::StringDecoder{data}.decode(el::text::StringEncoding::Utf8);
    }
};
