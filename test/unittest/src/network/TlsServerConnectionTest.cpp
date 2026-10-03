// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ConnectionQuota.hpp>
#include <erbsland/cryptology/impl/system_trust/SystemTrustBackend.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopDriver.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/network/impl/host/HostResolver.hpp>
#include <erbsland/network/impl/tcp/TcpAcceptedSocket.hpp>
#include <erbsland/network/impl/tcp/TcpConnection.hpp>
#include <erbsland/network/impl/tcp/TcpConnectionDevice.hpp>
#include <erbsland/network/impl/tcp/TcpConnectionRequest.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocol.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolOptions.hpp>
#include <erbsland/network/impl/tls/server/TlsServerConnection.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/SigningPrivateKey.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/Timestamp.hpp>
#include <erbsland/TlsConfiguration.hpp>
#include <erbsland/TlsIdentity.hpp>
#include <erbsland/TlsServerConnection.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/X509CertificateBundle.hpp>
#include <erbsland/X509TlsCertificatePolicy.hpp>

#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <utility>
#include <vector>

using namespace el::cryptology;
using namespace el::event;
using namespace el::network;
using namespace el::text::literals;
namespace mem = el::mem;
namespace unit = el::unit;

TESTED_TARGETS(
    TlsServerConnection TlsServerAcceptOptions TlsConfigurationMapping TlsServerConnectionEventEditor
        ConnectionCloseContext ConnectionState ConnectionQuota ConnectionQuotaLease)
class TlsServerConnectionTest final : public el::UnitTest {
    using HostResolver = el::network::impl::HostResolver;
    using TcpAcceptedSocket = el::network::impl::TcpAcceptedSocket;
    using TcpAcceptedSocketPtr = el::network::impl::TcpAcceptedSocketPtr;
    using TcpConnection = el::network::impl::TcpConnection;
    using TcpConnectionDevice = el::network::impl::TcpConnectionDevice;
    using TcpConnectionDeviceCallbacks = el::network::impl::TcpConnectionDeviceCallbacks;
    using TcpConnectionDeviceSendStatus = el::network::impl::TcpConnectionDeviceSendStatus;
    using TcpConnectionRequest = el::network::impl::TcpConnectionRequest;
    using TcpConnectionRequestPtr = el::network::impl::TcpConnectionRequestPtr;
    using TlsClientProtocol = el::network::impl::TlsClientProtocol;
    using TlsClientProtocolOptions = el::network::impl::TlsClientProtocolOptions;
    using TlsClientProtocolState = el::network::impl::TlsClientProtocolState;
    using TlsServerConnection = el::network::impl::TlsServerConnection;
    using TlsServerConnectionEventEditor = el::network::impl::TlsServerConnectionEventEditor;

private:
    el::String _diagnostic;
    auto additionalErrorMessages() -> std::string override { return el::StringConverter{_diagnostic}.toStdString(); }
    struct TrustState final {
        std::atomic<bool> started{};
        std::atomic<bool> finished{};
        std::promise<void> release;
        std::shared_future<void> gate{release.get_future().share()};
    };
    class BlockingTrust final : public el::cryptology::impl::SystemTrustBackend {
    public:
        explicit BlockingTrust(std::shared_ptr<TrustState> state) : _state{std::move(state)} {}
        auto validate(
            const X509TlsCertificatePolicy &,
            const X509CertificateBundle &,
            const std::optional<Host> &host,
            el::time::Timestamp) const -> X509CertificateValidation override {
            if (host.has_value()) {
                return reject(X509CertificateValidationFailureCategory::NativeFailure, "Expected client purpose"_el);
            }
            _state->started.store(true);
            static_cast<void>(_state->gate.wait_for(std::chrono::seconds{10}));
            _state->finished.store(true);
            return reject(X509CertificateValidationFailureCategory::NativeFailure, "Test native failure"_el);
        }

    private:
        std::shared_ptr<TrustState> _state;
    };
    inline static std::shared_ptr<TrustState> _trustState;
    static auto createBlockingTrust() -> std::unique_ptr<el::cryptology::impl::SystemTrustBackend> {
        return std::make_unique<BlockingTrust>(_trustState);
    }
    struct TrustFixture final {
        TrustFixture() {
            _trustState = std::make_shared<TrustState>();
            el::cryptology::impl::SystemTrustBackend::setTestFactory(&createBlockingTrust);
        }
        ~TrustFixture() {
            el::cryptology::impl::SystemTrustBackend::setTestFactory(nullptr);
            _trustState->release.set_value();
            _trustState.reset();
        }
    };

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
        void setReceiving(unit::ByteCount) override {}
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
        configuration.setServer(
            el::cryptology::TlsServerConfiguration{TlsIdentity{
                X509CertificateBundle::fromPemOrThrow(readText(certificatePath)),
                SigningPrivateKey::fromPemOrThrow(readText(keyPath))}});
        el::core::application().cryptologyConfiguration().setTlsConfiguration(label, std::move(configuration));
    }

    [[nodiscard]] static auto clientOptions(const el::text::String &host = "localhost"_el, bool withIdentity = false)
        -> TlsClientProtocolOptions {
        auto identity = TlsIdentityConstPtr{};
        if (withIdentity) {
            identity = std::make_shared<const TlsIdentity>(
                X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/client.pem")),
                SigningPrivateKey::fromPemOrThrow(readText("data/network/tls-interop/client-key.pem")));
        }
        return TlsClientProtocolOptions{
            Host::fromStringOrThrow(host),
            X509TlsCertificatePolicy{
                X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/ca.pem"))},
            el::time::Timestamp::now(),
            {},
            {},
            {KeyAgreementAlgorithm::X25519},
            identity};
    }

    [[nodiscard]] static auto makeHarness(EventLoopOptions options = {}) -> Harness {
        auto result = Harness{};
        result.driver = EventLoopDriver::createDefault();
        result.loop = EventLoop::create(result.driver, std::move(options));
        result.device = std::make_shared<FakeState>();
        const auto device = result.device;
        auto tcp = std::make_shared<el::network::impl::TcpConnection>(
            result.loop,
            result.driver,
            std::make_shared<FakeResolver>(),
            [device](EventLoopDriverPtr, unit::ByteCount, TcpConnectionDeviceCallbacks callbacks) {
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
            if (const auto &request = client.pendingCertificateValidation()) {
                client.completeCertificateValidation(request->execute());
            } else if (const auto &request = client.pendingSignature()) {
                client.completeSignature(request->execute());
            } else {
                client.resume();
            }
            progress = true;
        }
        return progress;
    }

    template <typename Predicate>
    void runUntil(const EventLoopPtr &loop, Predicate predicate) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
        while (!predicate() && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(el::time::Milliseconds{10}));
        }
        REQUIRE(predicate());
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testBlockedClientValidationTimeoutAbortAndLateCompletion() {
        for (const auto cancel : {false, true}) {
            auto applicationScope = ApplicationTestScope<>{};
            registerIdentity("tls"_el);
            auto &registry = applicationScope.app().cryptologyConfiguration();
            auto profile = *registry.resolveTlsConfiguration("tls"_el, TlsRole::Server).configuration()->server();
            profile.setClientAuthentication(TlsClientAuthentication::Required);
            auto trust = TlsTrustConfiguration{};
            trust.setSystemTrust(true);
            profile.setTrust(trust);
            registry.setTlsConfiguration("tls"_el, TlsConfiguration{profile});
            auto harness = makeHarness(
                EventLoopOptions{}.setMaximumQueuedEvents(32U).setQueueOverflowAction(
                    EventQueueOverflowAction::Ignore));
            auto quota = ConnectionQuota::create(unit::ItemCount{1U});
            auto finalCount = std::size_t{};
            auto reason = NetworkErrorReason::Unknown;
            auto state = std::shared_ptr<TrustState>{};
            {
                const auto fixture = TrustFixture{};
                state = _trustState;
                run(harness, [&]() {
                    harness.connection->events()
                        .onHandshakeCompleted([&]() { REQUIRE(false); })
                        .onData([&](mem::ByteBlock) { REQUIRE(false); })
                        .onError([&](const NetworkErrorContext &context) { reason = context.reason(); })
                        .onFinal([&]() { ++finalCount; });
                    harness.connection->accept(
                        makeRequest(), TlsServerAcceptOptions{quota}.setHandshakeTimeout(el::time::Milliseconds{3000}));
                });
                auto client = TlsClientProtocol{clientOptions("localhost"_el, true)};
                client.start();
                auto cursor = std::size_t{};
                auto applicationSent = false;
                runUntil(harness.loop, [&]() {
                    static_cast<void>(transferClientOutput(harness, client));
                    static_cast<void>(transferServerOutput(harness, client, cursor));
                    if (!applicationSent && client.state() == TlsClientProtocolState::Established) {
                        REQUIRE(client.sendApplication(mem::ByteBlock{'n', 'o'}.span()).isAccepted());
                        applicationSent = true;
                    }
                    _diagnostic = el::StringFormat{"Started={} final={} reason={} client={}"_el}.build(
                        state->started.load(), finalCount, static_cast<int>(reason), client.failureDiagnostic());
                    return state->started.load();
                });
                REQUIRE_FALSE(harness.connection->isPeerAuthenticated());
                REQUIRE(harness.connection->peerCertificatePath().isEmpty());
                run(harness, [&]() {
                    for (auto i = 0U; i < 32U; ++i) {
                        harness.loop->invoke([]() {});
                    }
                    if (cancel) {
                        harness.connection->abort();
                    }
                });
                runUntil(harness.loop, [&]() {
                    _diagnostic = el::StringFormat{"Waiting final: cancel={} final={} started={} finished={}"_el}.build(
                        cancel, finalCount, state->started.load(), state->finished.load());
                    return finalCount == 1U;
                });
                if (!cancel) {
                    REQUIRE_EQUAL(reason, NetworkErrorReason::Timeout);
                }
                REQUIRE_FALSE(state->finished.load());
                REQUIRE_EQUAL(quota->current(), unit::ItemCount{});
            }
            runUntil(harness.loop, [&]() {
                _diagnostic = "Waiting for released backend completion"_el;
                return state->finished.load();
            });
            static_cast<void>(harness.loop->runUntilIdle());
            REQUIRE_EQUAL(finalCount, std::size_t{1U});
            REQUIRE_FALSE(harness.connection->isPeerAuthenticated());
        }
    }

    void testInactiveAbortSurvivesFullOwnerQueue() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto harness = makeHarness(
            EventLoopOptions{}.setMaximumQueuedEvents(1U).setQueueOverflowAction(EventQueueOverflowAction::Ignore));
        auto finalCount = std::size_t{};
        run(harness, [&]() -> void {
            harness.connection->events().onFinal([&]() -> void { ++finalCount; });
            harness.loop->invoke([]() -> void {});
            harness.connection->abort();
        });
        REQUIRE_EQUAL(finalCount, std::size_t{1U});
    }

    void testConfigurationFailureAndHandshakeQuotaAdmission() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto harness = makeHarness();
        auto request = makeRequest();
        const auto quota = ConnectionQuota::create(unit::ItemCount{1U});
        run(harness, [&]() -> void {
            REQUIRE_THROWS_AS(
                el::err::RuntimeError, harness.connection->accept(request, TlsServerAcceptOptions{quota}));
            REQUIRE_EQUAL(request->state(), TcpConnectionRequestState::Pending);
            REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Inactive);
        });

        registerIdentity("tls"_el);
        run(harness, [&]() -> void { harness.connection->accept(request, TlsServerAcceptOptions{quota}); });
        REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Handshaking);
        auto client = TlsClientProtocol{clientOptions()};
        client.start();
        REQUIRE(transferClientOutput(harness, client));
        REQUIRE_EQUAL(harness.connection->requestedConfigurationLabel(), "tls"_el);
        REQUIRE_EQUAL(harness.connection->matchedConfigurationLabel(), "tls"_el);
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{1U});
        run(harness, [&]() -> void { harness.connection->abort(); });
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{});

        auto rejectedHarness = makeHarness(
            EventLoopOptions{}.setMaximumQueuedEvents(1U).setQueueOverflowAction(EventQueueOverflowAction::Ignore));
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
            rejectedHarness.loop->invoke([]() -> void {});
            rejectedHarness.connection->accept(rejectedRequest, TlsServerAcceptOptions{quota});
        });
        REQUIRE_EQUAL(rejectedRequest->state(), TcpConnectionRequestState::Rejected);
        REQUIRE_EQUAL(reason, NetworkErrorReason::ResourceLimitExceeded);
        REQUIRE_EQUAL(phase, NetworkErrorPhase::Accepting);
        REQUIRE_EQUAL(finalCount, std::size_t{1U});
        REQUIRE_EQUAL(rejectedHarness.connection->state(), ConnectionState::Failed);
    }

    void testHandshakeTimeoutAndAdmissionCallbackExceptionFinalizeOnce() {
        const auto applicationScope = ApplicationTestScope<>{};
        registerIdentity("tls"_el);
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
        REQUIRE_EQUAL(timeoutHarness.connection->state(), ConnectionState::Failed);
        REQUIRE_EQUAL(timeoutQuota->current(), unit::ItemCount{});

        auto callbackHarness = makeHarness();
        const auto callbackQuota = ConnectionQuota::create(unit::ItemCount{1U});
        auto retainedLease = callbackQuota->tryAcquire();
        REQUIRE(retainedLease.has_value());
        auto callbackFinalCount = std::size_t{};
        callbackHarness.loop->invoke([&]() -> void {
            callbackHarness.connection->events()
                .onError([](const NetworkErrorContext &) -> void { throw el::err::RuntimeError{"expected"}; })
                .onFinal([&]() -> void { ++callbackFinalCount; });
            callbackHarness.connection->accept(makeRequest(), TlsServerAcceptOptions{callbackQuota});
        });
        REQUIRE(callbackHarness.loop->runOnce());
        REQUIRE(callbackHarness.loop->runOnce());
        REQUIRE(callbackHarness.loop->hasError());
        REQUIRE_EQUAL(callbackFinalCount, std::size_t{1U});
        REQUIRE_THROWS_AS(el::err::RuntimeError, std::rethrow_exception(callbackHarness.loop->takeError()));
        REQUIRE_EQUAL(callbackHarness.connection->state(), ConnectionState::Failed);
    }

    void testClientHelloSelectionAndCheckpointAbort() {
        const auto applicationScope = ApplicationTestScope<>{};
        registerIdentity("tls"_el);
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
            options.setConfigurationMappings({{HostName::fromStringOrThrow("LOCALHOST"_el), "identity/exact"_el}});
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
        registerIdentity("tls"_el);
        auto harness = makeHarness();
        const auto quota = ConnectionQuota::create(unit::ItemCount{1U});
        auto events = std::vector<int>{};
        auto received = mem::ByteBlock{};
        auto closeOrigin = ConnectionCloseOrigin::Remote;
        run(harness, [&]() -> void {
            harness.connection->events()
                .onTransportConnected([&]() -> void { events.push_back(1); })
                .onClientHello([&]() -> void { events.push_back(2); })
                .onHandshakeCompleted([&]() -> void {
                    REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Active);
                    events.push_back(3);
                })
                .onData([&](mem::ByteBlock data) -> void {
                    received = std::move(data);
                    events.push_back(4);
                })
                .onClosed([&](const ConnectionCloseContext &context) -> void {
                    closeOrigin = context.origin();
                    events.push_back(5);
                })
                .onFinal([&]() -> void { events.push_back(6); });
            harness.connection->accept(makeRequest(), TlsServerAcceptOptions{quota});
        });

        auto client = TlsClientProtocol{clientOptions()};
        auto serverOutputCursor = std::size_t{};
        client.start();
        runUntil(harness.loop, [&]() -> bool {
            static_cast<void>(transferClientOutput(harness, client));
            static_cast<void>(transferServerOutput(harness, client, serverOutputCursor));
            return client.state() == TlsClientProtocolState::Established &&
                harness.connection->state() == ConnectionState::Active;
        });
        REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Active);
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
        REQUIRE_EQUAL(closeOrigin, ConnectionCloseOrigin::Local);
        REQUIRE_EQUAL(events, std::vector<int>({1, 2, 3, 4, 5, 6}));
        REQUIRE(harness.device->closed);
        REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Closed);
    }
};
