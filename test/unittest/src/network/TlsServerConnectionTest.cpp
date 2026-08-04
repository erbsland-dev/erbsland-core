// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/tls/TlsConfiguration.hpp>
#include <erbsland/cryptology/tls/TlsServerIdentity.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/cryptology/x509/X509ServerCertificatePolicy.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/EventLoopDriver.hpp>
#include <erbsland/network/impl/HostResolver.hpp>
#include <erbsland/network/impl/TcpAcceptedSocket.hpp>
#include <erbsland/network/impl/TcpConnection.hpp>
#include <erbsland/network/impl/TcpConnectionDevice.hpp>
#include <erbsland/network/impl/TcpConnectionRequest.hpp>
#include <erbsland/network/impl/TlsClientProtocol.hpp>
#include <erbsland/network/impl/TlsClientProtocolOptions.hpp>
#include <erbsland/network/impl/TlsServerConnection.hpp>
#include <erbsland/network/source/ConnectionQuota.hpp>
#include <erbsland/network/tls/TlsServerConnection.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace el::cryptology;
using namespace el::event;
using namespace el::network;
using namespace el::network::impl;
using namespace el::text::literals;
namespace mem = el::mem;
namespace unit = el::unit;

TESTED_TARGETS(
    TlsServerConnection TlsServerAcceptOptions TlsServerIdentityMapping TlsServerConnectionEventEditor
        TlsServerConnectionCloseContext TlsServerConnectionState ConnectionQuota ConnectionQuotaLease)
class TlsServerConnectionTest final : public el::UnitTest {
private:
    class FakeResolver final : public HostResolver {
    public:
        auto resolve(const HostName &) -> el::util::List<IpAddress> override { return {}; }
    };

    class FakeSocket final : public TcpAcceptedSocket {
    public:
        FakeSocket(IpEndpoint local, IpEndpoint remote) : _local{std::move(local)}, _remote{std::move(remote)} {}
        auto localEndpoint() const noexcept -> const IpEndpoint & override { return _local; }
        auto remoteEndpoint() const noexcept -> const IpEndpoint & override { return _remote; }

    private:
        IpEndpoint _local;
        IpEndpoint _remote;
    };

    struct FakeState final {
        TcpConnectionDeviceCallbacks callbacks;
        std::vector<mem::ByteBlock> sends;
        bool closed{};
        bool aborted{};
    };

    class FakeDevice final : public TcpConnectionDevice {
    public:
        explicit FakeDevice(std::shared_ptr<FakeState> state) : _state{std::move(state)} {}
        void connect(IpEndpoint) override {}
        void accept(TcpAcceptedSocketPtr socket) override {
            _state->callbacks.connected(socket->localEndpoint(), socket->remoteEndpoint());
        }
        [[nodiscard]] auto canAccept(const TcpAcceptedSocket &socket) const noexcept -> bool override {
            return dynamic_cast<const FakeSocket *>(&socket) != nullptr;
        }
        auto send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus override {
            _state->sends.emplace_back(std::move(data));
            return TcpConnectionDeviceSendStatus::Complete;
        }
        void setReceiving(unit::ByteLength) override {}
        void close() noexcept override { _state->closed = true; }
        void abort() noexcept override { _state->aborted = true; }

    private:
        std::shared_ptr<FakeState> _state;
    };

    struct Harness final {
        EventLoopDriverPtr driver;
        EventLoopPtr loop;
        std::shared_ptr<FakeState> device;
        std::shared_ptr<el::network::impl::TlsServerConnection> implementation;
        el::network::TlsServerConnectionPtr connection;
    };

    [[nodiscard]] static auto readText(const char *path) -> el::text::String {
        return el::text::String{el::unittest::fh::readDataText(path)};
    }

    static void registerIdentity(
        const el::text::String &label,
        const char *certificatePath = "data/network/tls-interop/server.pem",
        const char *keyPath = "data/network/tls-interop/server-key.pem") {
        auto configuration = TlsConfiguration{};
        configuration.setServerIdentity(
            TlsServerIdentity{
                X509CertificateBundle::fromPemOrThrow(readText(certificatePath)),
                SigningPrivateKey::fromPemOrThrow(readText(keyPath))});
        el::core::application().cryptologyConfiguration().setTlsConfiguration(label, std::move(configuration));
    }

    [[nodiscard]] static auto clientOptions(const el::text::String &host = "localhost"_el) -> TlsClientProtocolOptions {
        return TlsClientProtocolOptions{
            Host::fromStringOrThrow(host),
            X509ServerCertificatePolicy{
                X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/ca.pem"))},
            el::time::DateTime::now()};
    }

    [[nodiscard]] static auto makeHarness() -> Harness {
        auto result = Harness{};
        result.driver = EventLoopDriver::createDefault();
        result.loop = EventLoop::create(result.driver);
        result.device = std::make_shared<FakeState>();
        const auto device = result.device;
        auto tcp = std::make_shared<el::network::impl::TcpConnection>(
            result.loop,
            result.driver,
            std::make_shared<FakeResolver>(),
            [device](EventLoopDriverPtr, unit::ByteLength, TcpConnectionDeviceCallbacks callbacks) {
                device->callbacks = std::move(callbacks);
                return std::make_unique<FakeDevice>(device);
            });
        result.implementation = std::make_shared<el::network::impl::TlsServerConnection>(result.loop, tcp);
        result.connection = result.implementation;
        return result;
    }

    [[nodiscard]] static auto makeRequest() -> el::network::TcpConnectionRequestPtr {
        return std::make_shared<el::network::impl::TcpConnectionRequest>(
            std::make_unique<FakeSocket>(
                IpEndpoint{IpAddress::loopbackV4(), Port{9443U}}, IpEndpoint{IpAddress::loopbackV4(), Port{49152U}}),
            []() -> void {});
    }

    static void run(Harness &harness, std::function<void()> callback) {
        harness.loop->invoke(std::move(callback));
        static_cast<void>(harness.loop->runUntilIdle());
    }

    static auto transferClientOutput(Harness &harness, TlsClientProtocol &client) -> bool {
        auto progress = false;
        while (auto record = client.takeTransportOutput()) {
            progress = true;
            run(harness, [&harness, record = std::move(*record)]() mutable -> void {
                harness.device->callbacks.data(std::move(record));
            });
        }
        return progress;
    }

    static auto transferServerOutput(Harness &harness, TlsClientProtocol &client, std::size_t &cursor) -> bool {
        auto progress = false;
        while (cursor < harness.device->sends.size()) {
            client.feedTransport(harness.device->sends[cursor++].span());
            progress = true;
        }
        while (client.hasCheckpoint()) {
            client.resume();
            progress = true;
        }
        return progress;
    }

    template <typename Predicate>
    void runUntil(const EventLoopPtr &loop, Predicate predicate) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
        while (!predicate() && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(el::time::Milliseconds{10}));
        }
        REQUIRE(predicate());
    }

public:
    void testConfigurationFailureAndHandshakeQuotaAdmission() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto harness = makeHarness();
        auto request = makeRequest();
        const auto quota = ConnectionQuota::create(unit::ItemCount{1U});
        run(harness, [&]() -> void {
            REQUIRE_THROWS_AS(
                el::err::RuntimeError, harness.connection->accept(request, TlsServerAcceptOptions{quota}));
            REQUIRE_EQUAL(request->state(), TcpConnectionRequestState::Pending);
            REQUIRE_EQUAL(harness.connection->state(), TlsServerConnectionState::Inactive);
        });

        registerIdentity("tls"_el);
        run(harness, [&]() -> void { harness.connection->accept(request, TlsServerAcceptOptions{quota}); });
        REQUIRE_EQUAL(harness.connection->state(), TlsServerConnectionState::Handshaking);
        auto client = TlsClientProtocol{clientOptions()};
        client.start();
        REQUIRE(transferClientOutput(harness, client));
        REQUIRE_EQUAL(harness.connection->requestedConfigurationLabel(), "tls/server"_el);
        REQUIRE_EQUAL(harness.connection->matchedConfigurationLabel(), "tls"_el);
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{1U});
        run(harness, [&]() -> void { harness.connection->abort(); });
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{});

        auto rejectedHarness = makeHarness();
        auto rejectedRequest = makeRequest();
        auto retainedLease = quota->tryAcquire();
        REQUIRE(retainedLease.has_value());
        auto reason = NetworkErrorReason::Unknown;
        auto phase = NetworkErrorPhase::None;
        auto finalCount = std::size_t{};
        run(rejectedHarness, [&]() -> void {
            rejectedHarness.connection->events()
                .onError([&](const NetworkErrorContext &context) -> void {
                    reason = context.reason();
                    phase = context.phase();
                })
                .onFinal([&]() -> void { ++finalCount; });
            rejectedHarness.connection->accept(rejectedRequest, TlsServerAcceptOptions{quota});
        });
        REQUIRE_EQUAL(rejectedRequest->state(), TcpConnectionRequestState::Rejected);
        REQUIRE_EQUAL(reason, NetworkErrorReason::ResourceLimitExceeded);
        REQUIRE_EQUAL(phase, NetworkErrorPhase::Accepting);
        REQUIRE_EQUAL(finalCount, std::size_t{1U});
        REQUIRE_EQUAL(rejectedHarness.connection->state(), TlsServerConnectionState::Failed);
    }

    void testHandshakeTimeoutAndAdmissionCallbackExceptionFinalizeOnce() {
        const auto applicationScope = ApplicationTestScope<>{};
        registerIdentity("tls/server"_el);
        auto timeoutHarness = makeHarness();
        const auto timeoutQuota = ConnectionQuota::create(unit::ItemCount{1U});
        auto timeoutFinalCount = std::size_t{};
        auto timeoutReason = NetworkErrorReason::Unknown;
        run(timeoutHarness, [&]() -> void {
            timeoutHarness.connection->events()
                .onError([&](const NetworkErrorContext &context) -> void { timeoutReason = context.reason(); })
                .onFinal([&]() -> void { ++timeoutFinalCount; });
            timeoutHarness.connection->accept(
                makeRequest(), TlsServerAcceptOptions{timeoutQuota}.setHandshakeTimeout(el::time::Milliseconds{5}));
        });
        runUntil(timeoutHarness.loop, [&]() -> bool { return timeoutFinalCount == 1U; });
        REQUIRE_EQUAL(timeoutReason, NetworkErrorReason::Timeout);
        REQUIRE_EQUAL(timeoutHarness.connection->state(), TlsServerConnectionState::Failed);
        REQUIRE_EQUAL(timeoutQuota->current(), unit::ItemCount{});

        auto callbackHarness = makeHarness();
        const auto callbackQuota = ConnectionQuota::create(unit::ItemCount{1U});
        auto retainedLease = callbackQuota->tryAcquire();
        REQUIRE(retainedLease.has_value());
        auto callbackFinalCount = std::size_t{};
        callbackHarness.loop->invoke([&]() -> void {
            callbackHarness.connection->events()
                .onError([](const NetworkErrorContext &) -> void { throw std::runtime_error{"expected"}; })
                .onFinal([&]() -> void { ++callbackFinalCount; });
            callbackHarness.connection->accept(makeRequest(), TlsServerAcceptOptions{callbackQuota});
        });
        REQUIRE(callbackHarness.loop->runOnce());
        REQUIRE(callbackHarness.loop->runOnce());
        REQUIRE(callbackHarness.loop->hasError());
        REQUIRE_EQUAL(callbackFinalCount, std::size_t{1U});
        REQUIRE_THROWS_AS(std::runtime_error, std::rethrow_exception(callbackHarness.loop->takeError()));
        REQUIRE_EQUAL(callbackHarness.connection->state(), TlsServerConnectionState::Failed);
    }

    void testClientHelloSelectionAndCheckpointAbort() {
        const auto applicationScope = ApplicationTestScope<>{};
        registerIdentity("tls/server"_el);
        registerIdentity(
            "identity/exact"_el,
            "data/network/tls-interop/server-ecdsa.pem",
            "data/network/tls-interop/server-ecdsa-key.pem");
        auto harness = makeHarness();
        const auto quota = ConnectionQuota::create(unit::ItemCount{1U});
        auto events = std::vector<int>{};
        run(harness, [&]() -> void {
            harness.connection->events()
                .onTransportConnected([&]() -> void { events.push_back(1); })
                .onClientHello([&]() -> void {
                    REQUIRE_EQUAL(
                        harness.connection->serverName()->toString(HostNameFormat::IdnaAscii), "localhost"_el);
                    REQUIRE_EQUAL(harness.connection->requestedConfigurationLabel(), "identity/exact"_el);
                    REQUIRE_EQUAL(harness.connection->matchedConfigurationLabel(), "identity/exact"_el);
                    events.push_back(2);
                    harness.connection->abort();
                })
                .onHandshakeCompleted([&]() -> void { events.push_back(99); })
                .onFinal([&]() -> void { events.push_back(3); });
            auto options = TlsServerAcceptOptions{quota};
            options.setIdentityMappings({{HostName::fromStringOrThrow("LOCALHOST"_el), "identity/exact"_el}});
            harness.connection->accept(makeRequest(), std::move(options));
        });
        auto client = TlsClientProtocol{clientOptions()};
        client.start();
        REQUIRE(transferClientOutput(harness, client));
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(events, std::vector<int>({1, 2, 3}));
        REQUIRE(harness.device->sends.empty());
        REQUIRE(harness.device->aborted);
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{});
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAuthenticatedLifecycleApplicationDataAndGracefulClose() {
        const auto applicationScope = ApplicationTestScope<>{};
        registerIdentity("tls/server"_el);
        auto harness = makeHarness();
        const auto quota = ConnectionQuota::create(unit::ItemCount{1U});
        auto events = std::vector<int>{};
        auto received = mem::ByteBlock{};
        auto closeOrigin = TlsServerConnectionCloseOrigin::Remote;
        run(harness, [&]() -> void {
            harness.connection->events()
                .onTransportConnected([&]() -> void { events.push_back(1); })
                .onClientHello([&]() -> void { events.push_back(2); })
                .onHandshakeCompleted([&]() -> void {
                    REQUIRE_EQUAL(harness.connection->state(), TlsServerConnectionState::Active);
                    events.push_back(3);
                })
                .onData([&](mem::ByteBlock data) -> void {
                    received = std::move(data);
                    events.push_back(4);
                })
                .onClosed([&](const TlsServerConnectionCloseContext &context) -> void {
                    closeOrigin = context.origin();
                    events.push_back(5);
                })
                .onFinal([&]() -> void { events.push_back(6); });
            harness.connection->accept(makeRequest(), TlsServerAcceptOptions{quota});
        });

        auto client = TlsClientProtocol{clientOptions()};
        auto serverOutputCursor = std::size_t{};
        client.start();
        for (auto iteration = 0U; iteration < 100U; ++iteration) {
            auto progress = transferClientOutput(harness, client);
            progress = transferServerOutput(harness, client, serverOutputCursor) || progress;
            if (client.state() == TlsClientProtocolState::Established &&
                harness.connection->state() == TlsServerConnectionState::Active) {
                break;
            }
            REQUIRE(progress);
        }
        REQUIRE_EQUAL(harness.connection->state(), TlsServerConnectionState::Active);
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{});
        REQUIRE_EQUAL(events, std::vector<int>({1, 2, 3}));

        REQUIRE(client.sendApplication(mem::ByteBlock({'p', 'i', 'n', 'g'}).span()).isAccepted());
        REQUIRE(transferClientOutput(harness, client));
        REQUIRE_EQUAL(received, mem::ByteBlock({'p', 'i', 'n', 'g'}));
        run(harness,
            [&]() -> void { REQUIRE(harness.connection->send(mem::ByteBlock({'p', 'o', 'n', 'g'})).isAccepted()); });
        REQUIRE(transferServerOutput(harness, client, serverOutputCursor));
        REQUIRE_EQUAL(*client.takeApplicationData(), mem::ByteBlock({'p', 'o', 'n', 'g'}));

        run(harness, [&]() -> void { harness.connection->close(); });
        REQUIRE(transferServerOutput(harness, client, serverOutputCursor));
        REQUIRE(transferClientOutput(harness, client));
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(closeOrigin, TlsServerConnectionCloseOrigin::Local);
        REQUIRE_EQUAL(events, std::vector<int>({1, 2, 3, 4, 5, 6}));
        REQUIRE(harness.device->closed);
        REQUIRE_EQUAL(harness.connection->state(), TlsServerConnectionState::Closed);
    }
};
