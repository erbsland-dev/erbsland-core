// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/cryptology/impl/system_trust/SystemTrustBackend.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopDriver.hpp>
#include <erbsland/KeyAgreementPrivateKey.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/network/impl/host/HostResolver.hpp>
#include <erbsland/network/impl/tcp/TcpAcceptedSocket.hpp>
#include <erbsland/network/impl/tcp/TcpConnection.hpp>
#include <erbsland/network/impl/tcp/TcpConnectionDevice.hpp>
#include <erbsland/network/impl/tls/client/TlsClientConnection.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolTestAccess.hpp>
#include <erbsland/network/impl/tls/TlsWireWriter.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/TlsClientConnection.hpp>
#include <erbsland/TlsConfiguration.hpp>
#include <erbsland/TlsRecordContentType.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/X509CertificateBundle.hpp>
#include <erbsland/X509TlsCertificatePolicy.hpp>

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace el::cryptology;
using namespace el::event;
using namespace el::network;
using namespace el::text::literals;
namespace mem = el::mem;
namespace unit = el::unit;

TESTED_TARGETS(TlsClientConnection TlsClientConnectOptions TlsClientConnectionEventEditor ConnectionCloseContext)
class TlsClientConnectionTest final : public el::UnitTest {
    using HostResolver = el::network::impl::HostResolver;
    using TcpAcceptedSocket = el::network::impl::TcpAcceptedSocket;
    using TcpAcceptedSocketPtr = el::network::impl::TcpAcceptedSocketPtr;
    using TcpConnection = el::network::impl::TcpConnection;
    using TcpConnectionDevice = el::network::impl::TcpConnectionDevice;
    using TcpConnectionDeviceCallbacks = el::network::impl::TcpConnectionDeviceCallbacks;
    using TcpConnectionDeviceSendStatus = el::network::impl::TcpConnectionDeviceSendStatus;
    using TlsClientConnection = el::network::impl::TlsClientConnection;
    using TlsClientConnectionEventEditor = el::network::impl::TlsClientConnectionEventEditor;
    using TlsClientProtocol = el::network::impl::TlsClientProtocol;
    using TlsClientProtocolTestAccess = el::network::impl::TlsClientProtocolTestAccess;
    using TlsWireWriter = el::network::impl::TlsWireWriter;

private:
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
            const std::optional<Host> &,
            el::time::Timestamp) const -> X509CertificateValidation override {
            _state->started.store(true);
            static_cast<void>(_state->gate.wait_for(std::chrono::seconds{2}));
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
        auto resolve(const HostName &) -> el::util::List<IpAddress> override {
            return el::util::List<IpAddress>{IpAddress::loopbackV4()};
        }
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
        void accept(TcpAcceptedSocketPtr) override {}
        [[nodiscard]] auto canAccept(const TcpAcceptedSocket &) const noexcept -> bool override { return false; }
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
        std::shared_ptr<TlsClientProtocol *> protocol;
        std::shared_ptr<el::network::impl::TlsClientConnection> implementation;
        el::network::TlsClientConnectionPtr connection;
    };

    [[nodiscard]] static auto privateKey(const uint8_t value) -> KeyAgreementPrivateKey {
        return KeyAgreementPrivateKey::fromBytes(
            KeyAgreementAlgorithm::X25519, mem::ByteBlock{unit::ByteCount{32U}, value}.span());
    }

    [[nodiscard]] static auto handshake(const uint8_t type, const mem::ByteBlock &body) -> mem::ByteBlock {
        auto writer = TlsWireWriter{};
        writer.writeU8(type);
        writer.writeU24(static_cast<uint32_t>(body.length().toSizeT()));
        writer.writeBytes(body.span());
        return writer.finish();
    }

    [[nodiscard]] static auto serverHello(const mem::ByteBlock &sessionId, const KeyAgreementPrivateKey &serverPrivate)
        -> mem::ByteBlock {
        auto extensions = TlsWireWriter{};
        extensions.writeU16(43U);
        extensions.writeVector16(mem::ByteBlock({3U, 4U}).span());
        auto keyShare = TlsWireWriter{};
        keyShare.writeU16(0x001dU);
        keyShare.writeVector16(serverPrivate.publicKey().span());
        const auto keyShareBytes = keyShare.finish();
        extensions.writeU16(51U);
        extensions.writeVector16(keyShareBytes.span());
        const auto extensionBytes = extensions.finish();

        auto body = TlsWireWriter{};
        body.writeU16(0x0303U);
        body.writeBytes(mem::ByteBlock{unit::ByteCount{32U}, 0x33U}.span());
        body.writeVector8(sessionId.span());
        body.writeU16(0x1301U);
        body.writeU8(0U);
        body.writeVector16(extensionBytes.span());
        const auto bodyBytes = body.finish();
        auto message = handshake(2U, bodyBytes);
        auto record = TlsWireWriter{};
        record.writeU8(22U);
        record.writeU16(0x0303U);
        record.writeVector16(message.span());
        return record.finish();
    }

    [[nodiscard]] static auto makeHarness(EventLoopOptions options = {}) -> Harness {
        auto result = Harness{};
        result.driver = EventLoopDriver::createDefault();
        result.loop = EventLoop::create(result.driver, std::move(options));
        result.device = std::make_shared<FakeState>();
        result.protocol = std::make_shared<TlsClientProtocol *>(nullptr);
        const auto device = result.device;
        auto tcp = std::make_shared<el::network::impl::TcpConnection>(
            result.loop,
            result.driver,
            std::make_shared<FakeResolver>(),
            [device](EventLoopDriverPtr, unit::ByteCount, TcpConnectionDeviceCallbacks callbacks) {
                device->callbacks = std::move(callbacks);
                return std::make_shared<FakeDevice>(device);
            });
        const auto sessionId = mem::ByteBlock{unit::ByteCount{32U}, 0x22U};
        const auto protocol = result.protocol;
        result.implementation = std::make_shared<el::network::impl::TlsClientConnection>(
            result.loop, tcp, [protocol, sessionId](TlsClientProtocol &clientProtocol) -> void {
                *protocol = &clientProtocol;
                TlsClientProtocolTestAccess{clientProtocol}.start(
                    mem::ByteBlock{unit::ByteCount{32U}, 0x11U}, sessionId, privateKey(0x44U));
            });
        result.connection = result.implementation;
        return result;
    }

    static void registerPolicy(const el::text::String &label) {
        el::core::application().cryptologyConfiguration().setTlsConfiguration(
            label, TlsConfiguration{TlsClientConfiguration{}});
    }

    static void sendCertificate(Harness &harness) {
        const auto sessionId = mem::ByteBlock{unit::ByteCount{32U}, 0x22U};
        auto serverPrivate = privateKey(0x55U);
        const auto helloRecord = serverHello(sessionId, serverPrivate);
        run(harness, [&]() -> void { harness.device->callbacks.data(helloRecord); });
        auto sender = TlsClientProtocolTestAccess{**harness.protocol}.serverHandshakeEncryptor();
        const auto extensions = handshake(8U, mem::ByteBlock({0U, 0U}));
        const auto extensionRecord = sender.protect(TlsRecordContentType::Handshake, extensions.span());
        const auto bundle = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/cryptology/x509/server_authentication.pem")});
        auto entries = TlsWireWriter{};
        entries.writeVector24(bundle.certificates().toRawValue()[2U].toDer().span());
        entries.writeVector16(mem::ConstByteSpan{});
        const auto entryBytes = entries.finish();
        auto body = TlsWireWriter{};
        body.writeVector8(mem::ConstByteSpan{});
        body.writeVector24(entryBytes.span());
        const auto certificate = handshake(11U, body.finish());
        const auto certificateRecord = sender.protect(TlsRecordContentType::Handshake, certificate.span());
        run(harness, [&]() -> void {
            harness.device->callbacks.data(extensionRecord);
            harness.device->callbacks.data(certificateRecord);
        });
    }

    static void run(Harness &harness, std::function<void()> callback) {
        harness.loop->invoke(std::move(callback));
        static_cast<void>(harness.loop->runUntilIdle());
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

    void testAutomaticSystemTrustWithoutRegistration() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto harness = makeHarness();
        run(harness, [&]() -> void {
            harness.connection->connect(HostEndpoint{IpAddress::loopbackV4(), Port{9443U}});
            REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Connecting);
            REQUIRE(harness.connection->matchedConfigurationLabel().isEmpty());
            harness.connection->abort();
        });
        static_cast<void>(harness.loop->runUntilIdle());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testBlockedTrustTimesOutAndLateResultCannotFinalizeAgain() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto harness = makeHarness();
        auto finalCount = std::size_t{};
        auto reason = NetworkErrorReason::Unknown;
        auto state = std::shared_ptr<TrustState>{};
        {
            const auto fixture = TrustFixture{};
            state = _trustState;
            run(harness, [&]() -> void {
                harness.connection->events()
                    .onError([&](const NetworkErrorContext &context) -> void { reason = context.reason(); })
                    .onFinal([&]() -> void { ++finalCount; });
                harness.connection->connect(
                    HostEndpoint{IpAddress::loopbackV4(), Port{9443U}},
                    TlsClientConnectOptions{}.setHandshakeTimeout(el::time::Milliseconds{200}));
            });
            runUntil(harness.loop, [&]() -> bool { return static_cast<bool>(harness.device->callbacks.connected); });
            run(harness, [&]() -> void {
                harness.device->callbacks.connected(
                    IpEndpoint{IpAddress::loopbackV4(), Port{49152U}},
                    IpEndpoint{IpAddress::loopbackV4(), Port{9443U}});
            });
            sendCertificate(harness);
            runUntil(harness.loop, [&]() -> bool { return state->started.load(); });
            runUntil(harness.loop, [&]() -> bool { return finalCount == 1U; });
            REQUIRE_EQUAL(reason, NetworkErrorReason::Timeout);
            REQUIRE_FALSE(state->finished.load());
        }
        runUntil(harness.loop, [&]() -> bool { return state->finished.load(); });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(finalCount, std::size_t{1U});
        REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Failed);
    }

    void testDefaultLabelFallbackAndSynchronousConfigurationFailure() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto harness = makeHarness();
        registerPolicy("tls"_el);
        run(harness, [&]() -> void {
            harness.connection->connect(HostEndpoint{IpAddress::loopbackV4(), Port{9443U}});
            REQUIRE_EQUAL(harness.connection->requestedConfigurationLabel(), "tls"_el);
            REQUIRE_EQUAL(harness.connection->matchedConfigurationLabel(), "tls"_el);
            REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Connecting);
        });
        run(harness, [&]() -> void { harness.connection->abort(); });
        static_cast<void>(harness.loop->runUntilIdle());
    }

    void testDisabledGlobalTrustLeavesConnectionInactive() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto configurations = TlsConfigurationSet{};
        auto settings = TlsSettings{};
        settings.setSystemTrust(false);
        configurations.setSettings(settings);
        el::core::application().cryptologyConfiguration().replaceTlsConfigurations(configurations);
        auto harness = makeHarness();
        run(harness, [&]() -> void {
            REQUIRE_THROWS_AS(
                el::err::RuntimeError, harness.connection->connect(HostEndpoint{IpAddress::loopbackV4(), Port{9443U}}));
            REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Inactive);
        });
    }

    void testDeterministicLifecycleApplicationDataAndOrderlyClose() {
        const auto applicationScope = ApplicationTestScope<>{};
        registerPolicy("tls"_el);
        auto harness = makeHarness();
        auto events = std::vector<int>{};
        auto received = mem::ByteBlock{};
        auto closeOrigin = ConnectionCloseOrigin::Remote;
        run(harness, [&]() -> void {
            harness.connection->events()
                .onHostResolved([&](const el::util::List<IpEndpoint> &) -> void { events.push_back(1); })
                .onTransportConnected([&]() -> void { events.push_back(2); })
                .onPeerHello([&]() -> void { events.push_back(3); })
                .onPeerAuthenticated([&]() -> void { events.push_back(4); })
                .onHandshakeCompleted([&]() -> void {
                    REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Active);
                    events.push_back(5);
                })
                .onData([&](mem::ByteBlock data) -> void {
                    received = std::move(data);
                    events.push_back(6);
                })
                .onClosed([&](const ConnectionCloseContext &context) -> void {
                    closeOrigin = context.origin();
                    events.push_back(7);
                })
                .onFinal([&]() -> void { events.push_back(8); });
            harness.connection->connect(HostEndpoint{IpAddress::loopbackV4(), Port{9443U}});
        });
        runUntil(harness.loop, [&]() -> bool { return static_cast<bool>(harness.device->callbacks.connected); });
        run(harness, [&]() -> void {
            harness.device->callbacks.connected(
                IpEndpoint{IpAddress::loopbackV4(), Port{49152U}}, IpEndpoint{IpAddress::loopbackV4(), Port{9443U}});
        });
        REQUIRE(*harness.protocol != nullptr);
        REQUIRE_EQUAL(harness.device->sends.size(), std::size_t{1U});

        const auto sessionId = mem::ByteBlock{unit::ByteCount{32U}, 0x22U};
        auto serverPrivate = privateKey(0x55U);
        run(harness, [&]() -> void { harness.device->callbacks.data(serverHello(sessionId, serverPrivate)); });
        auto access = TlsClientProtocolTestAccess{**harness.protocol};
        auto serverHandshakeSender = access.serverHandshakeEncryptor();
        const auto encryptedExtensions = handshake(8U, mem::ByteBlock({0U, 0U}));
        run(harness, [&]() -> void {
            harness.device->callbacks.data(
                serverHandshakeSender.protect(TlsRecordContentType::Handshake, encryptedExtensions.span()));
        });
        auto finishedRecord = mem::ByteBlock{};
        run(harness, [&]() -> void {
            access.acceptServerAuthentication(
                mem::ByteBlock({11U, 0U, 0U, 4U, 0U, 0U, 0U, 0U}).span(),
                mem::ByteBlock({15U, 0U, 0U, 4U, 8U, 4U, 0U, 0U}).span());
            const auto finished = access.serverFinishedMessage();
            finishedRecord = serverHandshakeSender.protect(TlsRecordContentType::Handshake, finished.span());
            harness.device->callbacks.data(finishedRecord);
        });
        REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Active);
        REQUIRE_EQUAL(events, std::vector<int>({1, 2, 3, 4, 5}));
        REQUIRE(harness.connection->cipherSuite().has_value());

        auto serverApplicationSender = access.serverApplicationEncryptor();
        run(harness, [&]() -> void {
            const auto payload = mem::ByteBlock({'p', 'o', 'n', 'g'});
            harness.device->callbacks.data(
                serverApplicationSender.protect(TlsRecordContentType::ApplicationData, payload.span()));
        });
        REQUIRE_EQUAL(received, mem::ByteBlock({'p', 'o', 'n', 'g'}));
        run(harness, [&]() -> void {
            REQUIRE(harness.connection->send(mem::ByteBlock({'p', 'i', 'n', 'g'})).isAccepted());
            harness.connection->close();
            REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Closing);
        });
        run(harness, [&]() -> void {
            harness.device->callbacks.data(
                serverApplicationSender.protect(TlsRecordContentType::Alert, mem::ByteBlock({1U, 0U}).span()));
        });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(closeOrigin, ConnectionCloseOrigin::Local);
        REQUIRE_EQUAL(events, std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8}));
        REQUIRE(harness.device->closed);
    }

    void testHandshakeTimeoutReportsPhaseAndFinalizesOnce() {
        const auto applicationScope = ApplicationTestScope<>{};
        registerPolicy("tls"_el);
        auto harness = makeHarness();
        auto finalCount = std::size_t{};
        auto reason = NetworkErrorReason::Unknown;
        auto phase = NetworkErrorPhase::None;
        run(harness, [&]() -> void {
            harness.connection->events()
                .onError([&](const NetworkErrorContext &context) -> void {
                    reason = context.reason();
                    phase = context.phase();
                })
                .onFinal([&]() -> void { ++finalCount; });
            harness.connection->connect(
                HostEndpoint{IpAddress::loopbackV4(), Port{9443U}},
                TlsClientConnectOptions{}.setHandshakeTimeout(el::time::Milliseconds{5}));
        });
        runUntil(harness.loop, [&]() -> bool { return static_cast<bool>(harness.device->callbacks.connected); });
        run(harness, [&]() -> void {
            harness.device->callbacks.connected(
                IpEndpoint{IpAddress::loopbackV4(), Port{49152U}}, IpEndpoint{IpAddress::loopbackV4(), Port{9443U}});
        });
        runUntil(harness.loop, [&]() -> bool { return finalCount == 1U; });
        REQUIRE_EQUAL(reason, NetworkErrorReason::Timeout);
        REQUIRE_EQUAL(phase, NetworkErrorPhase::Handshaking);
        REQUIRE_EQUAL(finalCount, std::size_t{1U});
        REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Failed);
    }

    void testAbortAtCheckpointPreventsNextTransitionAndEmitsOnlyFinal() {
        const auto applicationScope = ApplicationTestScope<>{};
        registerPolicy("tls"_el);
        auto harness = makeHarness();
        auto events = std::vector<int>{};
        run(harness, [&]() -> void {
            harness.connection->events()
                .onTransportConnected([&]() -> void { events.push_back(1); })
                .onPeerHello([&]() -> void {
                    events.push_back(2);
                    harness.connection->abort();
                })
                .onPeerAuthenticated([&]() -> void { events.push_back(99); })
                .onClosed([&](const ConnectionCloseContext &) -> void { events.push_back(98); })
                .onError([&](const NetworkErrorContext &) -> void { events.push_back(97); })
                .onFinal([&]() -> void { events.push_back(3); });
            harness.connection->connect(HostEndpoint{IpAddress::loopbackV4(), Port{9443U}});
        });
        runUntil(harness.loop, [&]() -> bool { return static_cast<bool>(harness.device->callbacks.connected); });
        run(harness, [&]() -> void {
            harness.device->callbacks.connected(
                IpEndpoint{IpAddress::loopbackV4(), Port{49152U}}, IpEndpoint{IpAddress::loopbackV4(), Port{9443U}});
        });
        const auto sessionId = mem::ByteBlock{unit::ByteCount{32U}, 0x22U};
        auto serverPrivate = privateKey(0x55U);
        run(harness, [&]() -> void { harness.device->callbacks.data(serverHello(sessionId, serverPrivate)); });
        auto access = TlsClientProtocolTestAccess{**harness.protocol};
        auto serverHandshakeSender = access.serverHandshakeEncryptor();
        const auto encryptedExtensions = handshake(8U, mem::ByteBlock({0U, 0U}));
        run(harness, [&]() -> void {
            harness.device->callbacks.data(
                serverHandshakeSender.protect(TlsRecordContentType::Handshake, encryptedExtensions.span()));
        });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(events, std::vector<int>({1, 2, 3}));
        REQUIRE(harness.device->aborted);
        REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Closed);
    }
};
