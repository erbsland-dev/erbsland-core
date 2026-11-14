// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ConnectionQuota.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopDriver.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/network/impl/host/HostResolver.hpp>
#include <erbsland/network/impl/tcp/TcpAcceptedSocket.hpp>
#include <erbsland/network/impl/tcp/TcpConnection.hpp>
#include <erbsland/network/impl/tcp/TcpConnectionDevice.hpp>
#include <erbsland/network/impl/tcp/TcpConnectionRequest.hpp>
#include <erbsland/NetworkError.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/TcpConnection.hpp>
#include <erbsland/TimeUnitTags.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;
using namespace el::time;
namespace mem = el::mem;
namespace unit = el::unit;

TESTED_TARGETS(
    TcpConnection TcpConnectOptions TcpAcceptOptions TcpConnectionEventEditor ConnectionCloseContext
        ConnectionCloseOrigin TcpHostResolvedFn TcpConnectionDevice ConnectionQuota ConnectionQuotaLease)
class TcpConnectionTest final : public el::UnitTest {
    using HostResolver = el::network::impl::HostResolver;
    using TcpAcceptedSocket = el::network::impl::TcpAcceptedSocket;
    using TcpAcceptedSocketPtr = el::network::impl::TcpAcceptedSocketPtr;
    using TcpConnection = el::network::impl::TcpConnection;
    using TcpConnectionDevice = el::network::impl::TcpConnectionDevice;
    using TcpConnectionDeviceCallbacks = el::network::impl::TcpConnectionDeviceCallbacks;
    using TcpConnectionDeviceSendStatus = el::network::impl::TcpConnectionDeviceSendStatus;
    using TcpConnectionEventEditor = el::network::impl::TcpConnectionEventEditor;
    using TcpConnectionPtr = el::network::impl::TcpConnectionPtr;
    using TcpConnectionRequest = el::network::impl::TcpConnectionRequest;

    class FakeResolver final : public HostResolver {
    public:
        explicit FakeResolver(el::util::List<IpAddress> addresses) : _addresses{std::move(addresses)} {}
        auto resolve(const HostName &) -> el::util::List<IpAddress> override { return _addresses; }

    private:
        el::util::List<IpAddress> _addresses;
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
        std::optional<IpEndpoint> connectEndpoint;
        unit::ByteCount receiveLimit;
        unit::ByteCount maximumReceive;
        std::deque<TcpConnectionDeviceSendStatus> sendResults;
        std::vector<mem::ByteBlock> sends;
        std::function<void()> onConnect;
        bool destroyed{false};
        bool aliveAfterConnectCallback{false};
        bool accepted{false};
        bool compatible{true};
        bool closed{false};
        bool aborted{false};
    };

    class FakeDevice final : public TcpConnectionDevice {
    public:
        explicit FakeDevice(std::shared_ptr<FakeState> state) : _state{std::move(state)} {}
        ~FakeDevice() override { _state->destroyed = true; }

        void connect(IpEndpoint endpoint) override {
            const auto state = _state;
            state->connectEndpoint = std::move(endpoint);
            if (state->onConnect) {
                state->onConnect();
                state->aliveAfterConnectCallback = !state->destroyed;
            }
        }
        void accept(TcpAcceptedSocketPtr socket) override {
            _state->accepted = true;
            _state->callbacks.connected(socket->localEndpoint(), socket->remoteEndpoint());
        }
        auto canAccept(const TcpAcceptedSocket &socket) const noexcept -> bool override {
            return _state->compatible && dynamic_cast<const FakeSocket *>(&socket) != nullptr;
        }
        auto send(mem::ByteBlock data) -> TcpConnectionDeviceSendStatus override {
            _state->sends.emplace_back(std::move(data));
            if (_state->sendResults.empty()) {
                return TcpConnectionDeviceSendStatus::Complete;
            }
            const auto result = _state->sendResults.front();
            _state->sendResults.pop_front();
            return result;
        }
        void setReceiving(const unit::ByteCount maximumBytes) override { _state->maximumReceive = maximumBytes; }
        void close() noexcept override { _state->closed = true; }
        void abort() noexcept override { _state->aborted = true; }

    private:
        std::shared_ptr<FakeState> _state;
    };

    struct Harness final {
        EventLoopDriverPtr driver;
        EventLoopPtr loop;
        std::shared_ptr<FakeResolver> resolver;
        std::shared_ptr<bool> compatibility;
        std::shared_ptr<std::vector<std::shared_ptr<FakeState>>> devices;
        std::shared_ptr<el::network::impl::TcpConnection> implementation;
        el::network::TcpConnectionPtr connection;
    };

public:
    void testNativeConnectRetainsDeviceDuringReentrantAbort() {
        auto harness = Harness{};
        harness = makeHarness({IpAddress::loopbackV4()}, [&]() -> void { harness.connection->abort(); });
        run(harness,
            [&]() -> void { harness.connection->connect(HostEndpoint{IpAddress::loopbackV4(), Port{9000U}}); });
        REQUIRE_EQUAL(harness.devices->size(), 1U);
        const auto &device = harness.devices->back();
        REQUIRE(device->aborted);
        REQUIRE(device->aliveAfterConnectCallback);
        REQUIRE(device->destroyed);
    }

    void testHostResolvedPrecedesDeviceAndAbortPreventsCreation() {
        auto harness = makeHarness({IpAddress::loopbackV4()});
        auto eventOrder = std::vector<int>{};
        run(harness, [&]() -> void {
            harness.connection->events()
                .onHostResolved([&](const el::util::List<IpEndpoint> &endpoints) -> void {
                    REQUIRE(harness.devices->empty());
                    REQUIRE_EQUAL(endpoints.count(), unit::ItemCount{1U});
                    eventOrder.push_back(1);
                    harness.connection->abort();
                })
                .onFinal([&]() -> void { eventOrder.push_back(2); });
            harness.connection->connect(HostEndpoint{IpAddress::loopbackV4(), Port{9000U}});
        });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(eventOrder, std::vector<int>({1, 2}));
        REQUIRE(harness.devices->empty());
    }

    void testNamedEndpointsAreTriedSequentiallyInResolverOrder() {
        const auto first = IpAddress::loopbackV6();
        const auto second = IpAddress::loopbackV4();
        auto harness = makeHarness({first, second});
        auto delivered = el::util::List<IpEndpoint>{};
        auto connected = false;
        run(harness, [&]() -> void {
            harness.connection->events()
                .onHostResolved([&](const el::util::List<IpEndpoint> &endpoints) -> void { delivered = endpoints; })
                .onConnected([&]() -> void { connected = true; });
            harness.connection->connect(HostEndpoint{HostName::fromStringOrThrow("kaarten.example"_el), Port{9010U}});
        });
        runUntil(harness.loop, [&]() -> bool { return harness.devices->size() == 1U; });
        run(harness, [&]() -> void {
            (*harness.devices)[0]->callbacks.error(
                NetworkErrorContext{"first failed"_el, "simulated refusal"_el}.setReason(
                    NetworkErrorReason::ConnectionRefused));
        });
        REQUIRE_EQUAL(harness.devices->size(), std::size_t{2U});
        REQUIRE_EQUAL((*harness.devices)[0]->connectEndpoint->address(), first);
        REQUIRE_EQUAL((*harness.devices)[1]->connectEndpoint->address(), second);
        run(harness, [&]() -> void {
            (*harness.devices)[1]->callbacks.connected(
                IpEndpoint{second, Port{49152U}}, IpEndpoint{second, Port{9010U}});
        });
        REQUIRE(connected);
        REQUIRE_EQUAL(delivered.count(), unit::ItemCount{2U});
        REQUIRE_EQUAL(delivered.first().address(), first);
        REQUIRE_EQUAL(delivered.last().address(), second);
    }

    void testOverallTimeoutCoversAnUnfinishedNativeAttempt() {
        auto harness = makeHarness({IpAddress::loopbackV4()});
        auto reason = NetworkErrorReason::Unknown;
        auto final = false;
        run(harness, [&]() -> void {
            harness.connection->events()
                .onError([&](const NetworkErrorContext &context) -> void { reason = context.reason(); })
                .onFinal([&]() -> void { final = true; });
            harness.connection->connect(
                HostEndpoint{IpAddress::loopbackV4(), Port{9020U}}, TcpConnectOptions{}.setTimeout(Milliseconds{10}));
        });
        runUntil(harness.loop, [&]() -> bool { return final; });
        REQUIRE_EQUAL(reason, NetworkErrorReason::Timeout);
        REQUIRE(harness.devices->front()->aborted);
    }

    void testBoundedStreamFlowPauseAndCloseOrdering() {
        auto harness = makeHarness({IpAddress::loopbackV4()});
        auto options = TcpConnectOptions{};
        options.setBufferLimits(SocketBufferLimits{unit::ByteCount{5U}, unit::ByteCount{5U}});
        auto received = std::vector<mem::ByteBlock>{};
        auto events = std::vector<int>{};
        run(harness, [&]() -> void {
            harness.connection->events()
                .onConnected([&]() -> void { events.push_back(1); })
                .onData([&](mem::ByteBlock data) -> void { received.emplace_back(std::move(data)); })
                .onWritable([&]() -> void { events.push_back(2); })
                .onClosed([&](const ConnectionCloseContext &context) -> void {
                    REQUIRE_EQUAL(context.origin(), ConnectionCloseOrigin::Local);
                    events.push_back(3);
                })
                .onFinal([&]() -> void { events.push_back(4); });
            harness.connection->connect(HostEndpoint{IpAddress::loopbackV4(), Port{9030U}}, options);
        });
        run(harness, [&]() -> void {
            auto &state = *harness.devices->front();
            state.sendResults.push_back(TcpConnectionDeviceSendStatus::Pending);
            state.callbacks.connected(
                IpEndpoint{IpAddress::loopbackV4(), Port{49153U}}, IpEndpoint{IpAddress::loopbackV4(), Port{9030U}});
        });
        run(harness, [&]() -> void {
            const auto first = mem::ByteBlock{unit::ByteCount{4U}, mem::Byte{0x31U}};
            const auto blocked = mem::ByteBlock{unit::ByteCount{2U}, mem::Byte{0x32U}};
            REQUIRE(harness.connection->send(first).isAccepted());
            REQUIRE(harness.connection->send(blocked).wouldBlock());
            harness.connection->pauseReceiving();
            REQUIRE(harness.devices->front()->maximumReceive.isZero());
            harness.devices->front()->callbacks.data(mem::ByteBlock{unit::ByteCount{3U}, mem::Byte{0x44U}});
            REQUIRE(harness.devices->front()->maximumReceive.isZero());
        });
        REQUIRE(received.empty());
        run(harness, [&]() -> void {
            harness.connection->resumeReceiving();
            REQUIRE_EQUAL(harness.devices->front()->maximumReceive, unit::ByteCount{2U});
            harness.devices->front()->callbacks.sendCompleted();
        });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(received.size(), std::size_t{1U});
        run(harness, [&]() -> void { harness.connection->close(); });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(events, std::vector<int>({1, 2, 3, 4}));
        REQUIRE(harness.devices->front()->closed);
    }

    void testAcceptOptionsAndIncompatibleRequestAreNonConsuming() {
        auto harness = makeHarness({IpAddress::loopbackV4()});
        auto releaseCount = 0;
        const auto local = IpEndpoint{IpAddress::loopbackV4(), Port{9040U}};
        const auto remote = IpEndpoint{IpAddress::loopbackV4(), Port{49154U}};
        const auto quota = ConnectionQuota::create(unit::ItemCount{1U});
        auto lease = quota->tryAcquire(remote);
        REQUIRE(lease.has_value());
        auto request = std::make_shared<el::network::impl::TcpConnectionRequest>(
            std::make_unique<FakeSocket>(local, remote), std::move(*lease), [&]() -> void { releaseCount += 1; });
        *harness.compatibility = false;
        run(harness, [&]() -> void {
            REQUIRE_THROWS_AS(el::err::ParameterFailure, harness.connection->accept(request));
            REQUIRE_EQUAL(request->state(), TcpConnectionRequestState::Pending);
            REQUIRE_EQUAL(harness.connection->state(), ConnectionState::Inactive);
        });

        *harness.compatibility = true;
        auto connected = false;
        run(harness, [&]() -> void {
            harness.connection->events().onConnected([&]() -> void { connected = true; });
            harness.connection->accept(
                request,
                TcpAcceptOptions{}.setBufferLimits(SocketBufferLimits{unit::ByteCount{17U}, unit::ByteCount{19U}}));
        });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE(connected);
        REQUIRE_EQUAL(request->state(), TcpConnectionRequestState::Accepted);
        REQUIRE_EQUAL(releaseCount, 1);
        REQUIRE_EQUAL(harness.connection->bufferLimits().send(), unit::ByteCount{17U});
        REQUIRE_EQUAL(harness.devices->back()->receiveLimit, unit::ByteCount{19U});
        REQUIRE(harness.devices->back()->accepted);
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{1U});

        run(harness, [&]() -> void { harness.devices->back()->callbacks.remoteClosed(); });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{});
    }

private:
    [[nodiscard]] static auto makeHarness(
        std::initializer_list<IpAddress> addresses, std::function<void()> onConnect = {}) -> Harness {
        auto result = Harness{};
        result.driver = EventLoopDriver::createDefault();
        result.loop = EventLoop::create(result.driver);
        result.resolver = std::make_shared<FakeResolver>(el::util::List<IpAddress>{addresses});
        result.compatibility = std::make_shared<bool>(true);
        result.devices = std::make_shared<std::vector<std::shared_ptr<FakeState>>>();
        const auto devices = result.devices;
        const auto compatibility = result.compatibility;
        result.implementation = std::make_shared<el::network::impl::TcpConnection>(
            result.loop,
            result.driver,
            result.resolver,
            [devices, compatibility, onConnect = std::move(onConnect)](
                EventLoopDriverPtr, const unit::ByteCount receiveLimit, TcpConnectionDeviceCallbacks callbacks) {
                auto state = std::make_shared<FakeState>();
                state->receiveLimit = receiveLimit;
                state->compatible = *compatibility;
                state->callbacks = std::move(callbacks);
                state->onConnect = onConnect;
                devices->emplace_back(state);
                return std::make_shared<FakeDevice>(state);
            });
        result.connection = result.implementation;
        return result;
    }

    static void run(Harness &harness, std::function<void()> callback) {
        harness.loop->invoke(std::move(callback));
        static_cast<void>(harness.loop->runUntilIdle());
    }

    template <typename Predicate>
    void runUntil(const EventLoopPtr &loop, Predicate predicate) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
        while (!predicate() && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(Milliseconds{20}));
        }
        REQUIRE(predicate());
    }
};
