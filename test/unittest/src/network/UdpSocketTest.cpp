// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/EventLoopDriver.hpp>
#include <erbsland/network/impl/udp/UdpSocket.hpp>
#include <erbsland/network/impl/udp/UdpSocketDevice.hpp>
#include <erbsland/network/source/NetworkError.hpp>
#include <erbsland/network/udp/UdpSocket.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

using namespace el::event;
using namespace el::network;
using namespace el::network::impl;
using namespace el::text::literals;
namespace mem = el::mem;
namespace unit = el::unit;

TESTED_TARGETS(
    UdpSocket UdpSocketOptions UdpSocketEventEditor UdpDatagram UdpDatagramDropContext UdpDatagramDropReason
        NetworkSendStatus)
class UdpSocketTest final : public el::UnitTest {
    struct FakeState final {
        UdpSocketDeviceCallbacks callbacks;
        std::optional<IpEndpoint> requestedEndpoint;
        std::optional<IpEndpoint> actualEndpoint;
        unit::ByteLength maximumDatagramSize;
        std::deque<UdpSocketDeviceSendStatus> sendResults;
        std::vector<UdpDatagram> sendAttempts;
        bool receiving{false};
        bool closed{false};
        bool aborted{false};
    };

    class FakeDevice final : public UdpSocketDevice {
    public:
        explicit FakeDevice(std::shared_ptr<FakeState> state) : _state{std::move(state)} {}

    public: // implement UdpSocketDevice
        auto bind(IpEndpoint localEndpoint, const unit::ByteLength maximumDatagramSize) -> IpEndpoint override {
            _state->requestedEndpoint = localEndpoint;
            _state->maximumDatagramSize = maximumDatagramSize;
            if (localEndpoint.port().isAutomatic()) {
                localEndpoint = IpEndpoint{localEndpoint.address(), Port{49152U}, localEndpoint.scopeId()};
            }
            _state->actualEndpoint = localEndpoint;
            return localEndpoint;
        }

        auto send(const UdpDatagram &datagram) -> UdpSocketDeviceSendStatus override {
            _state->sendAttempts.emplace_back(datagram);
            if (_state->sendResults.empty()) {
                return UdpSocketDeviceSendStatus::Complete;
            }
            const auto result = _state->sendResults.front();
            _state->sendResults.pop_front();
            return result;
        }

        void setReceiving(const bool enabled) override { _state->receiving = enabled; }
        void close() noexcept override {
            _state->closed = true;
            _state->receiving = false;
        }
        void abort() noexcept override {
            _state->aborted = true;
            _state->receiving = false;
        }

    private:
        std::shared_ptr<FakeState> _state;
    };

    struct Harness final {
        EventLoopDriverPtr driver;
        EventLoopPtr loop;
        std::shared_ptr<FakeState> device;
        std::shared_ptr<el::network::impl::UdpSocket> implementation;
        UdpSocketPtr socket;
    };

public:
    void testOptionAndStatusValues() {
        auto options = UdpSocketOptions{};
        REQUIRE_EQUAL(options.maximumDatagramSize(), unit::ByteLength{65'507U});
        REQUIRE_EQUAL(options.sendQueueLimit(), unit::ByteLength{1024U * 1024U});
        options.setMaximumDatagramSize(unit::ByteLength{1200U}).setSendQueueLimit(unit::ByteLength{8192U});
        REQUIRE_EQUAL(options.maximumDatagramSize(), unit::ByteLength{1200U});
        REQUIRE_EQUAL(options.sendQueueLimit(), unit::ByteLength{8192U});

        REQUIRE(NetworkSendStatus::Accepted.isAccepted());
        REQUIRE(NetworkSendStatus::WouldBlock.wouldBlock());
        REQUIRE(NetworkSendStatus::Closed.isClosed());
        REQUIRE_FALSE(NetworkSendStatus::Accepted.wouldBlock());
    }

    void testStartConveniencesNormalizeEndpoints() {
        auto first = makeHarness();
        run(first, [&]() -> void { first.socket->start(); });
        REQUIRE_EQUAL(first.device->requestedEndpoint->address(), IpAddress::anyV4());
        REQUIRE(first.device->requestedEndpoint->port().isAutomatic());

        auto second = makeHarness();
        run(second, [&]() -> void { second.socket->start(IpAddress::loopbackV6()); });
        REQUIRE_EQUAL(second.device->requestedEndpoint->address(), IpAddress::loopbackV6());
        REQUIRE(second.device->requestedEndpoint->port().isAutomatic());

        auto third = makeHarness();
        run(third, [&]() -> void { third.socket->start(Port{5300U}); });
        REQUIRE_EQUAL(third.device->requestedEndpoint->address(), IpAddress::anyV4());
        REQUIRE_EQUAL(third.device->requestedEndpoint->port(), Port{5300U});

        auto fourth = makeHarness();
        run(fourth, [&]() -> void { fourth.socket->start(IpAddress::loopbackV6(), Port{5301U}); });
        REQUIRE_EQUAL(fourth.device->requestedEndpoint->address(), IpAddress::loopbackV6());
        REQUIRE_EQUAL(fourth.device->requestedEndpoint->port(), Port{5301U});

        auto fifth = makeHarness();
        const auto scoped = IpEndpoint{IpAddress::loopbackV6(), Port{5302U}, ScopeId{7U}};
        run(fifth, [&]() -> void { fifth.socket->start(scoped); });
        REQUIRE_EQUAL(*fifth.device->requestedEndpoint, scoped);
    }

    void testBindingPublishesActualEndpointAsynchronouslyAndCapturesOptions() {
        auto harness = makeHarness();
        auto bound = false;
        auto callbackState = NetworkSourceState::Inactive;
        auto beforeCallback = std::optional<IpEndpoint>{};
        auto options = UdpSocketOptions{};
        options.setMaximumDatagramSize(unit::ByteLength{1400U}).setSendQueueLimit(unit::ByteLength{4000U});

        harness.loop->invoke([&]() -> void {
            REQUIRE_FALSE(harness.socket->localEndpoint().has_value());
            harness.socket->events().onBound([&]() -> void {
                bound = true;
                callbackState = harness.socket->state();
            });
            harness.socket->start(options);
            beforeCallback = harness.socket->localEndpoint();
            REQUIRE_EQUAL(harness.socket->state(), NetworkSourceState::Starting);
            REQUIRE_FALSE(bound);
        });
        REQUIRE(harness.loop->runOnce());
        REQUIRE(beforeCallback.has_value());
        REQUIRE_EQUAL(beforeCallback->port(), Port{49152U});
        REQUIRE_FALSE(bound);
        static_cast<void>(harness.loop->runUntilIdle());

        REQUIRE(bound);
        REQUIRE_EQUAL(callbackState, NetworkSourceState::Active);
        REQUIRE_EQUAL(harness.device->maximumDatagramSize, unit::ByteLength{1400U});
        REQUIRE(harness.device->receiving);
        run(harness, [&]() -> void { REQUIRE_THROWS_AS(el::err::LogicError, harness.socket->start()); });
    }

    void testStartValidationDoesNotConsumeTheSocket() {
        auto harness = makeHarness();
        harness.loop->invoke([&]() -> void {
            auto options = UdpSocketOptions{};
            options.setMaximumDatagramSize(unit::ByteLength{});
            REQUIRE_THROWS_AS(el::err::ParameterError, harness.socket->start(options));
            options.setMaximumDatagramSize(unit::ByteLength{65'508U});
            REQUIRE_THROWS_AS(el::err::ParameterError, harness.socket->start(options));
            options.setMaximumDatagramSize(unit::ByteLength{1000U}).setSendQueueLimit(unit::ByteLength{});
            REQUIRE_THROWS_AS(el::err::ParameterError, harness.socket->start(options));
            options.setSendQueueLimit(unit::ByteLength::infinite());
            REQUIRE_THROWS_AS(el::err::ParameterError, harness.socket->start(options));
            REQUIRE_THROWS_AS(
                el::err::ParameterError,
                harness.socket->start(IpEndpoint{IpAddress::loopbackV4(), Port{}, ScopeId{3U}}));
            harness.socket->start(IpAddress::loopbackV4());
        });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(harness.socket->state(), NetworkSourceState::Active);
    }

    void testSendValidationQueueAccountingAndWritableThreshold() {
        auto harness = makeHarness();
        harness.device->sendResults.push_back(UdpSocketDeviceSendStatus::Pending);
        auto writableCount = 0;
        auto options = UdpSocketOptions{};
        options.setMaximumDatagramSize(unit::ByteLength{5U}).setSendQueueLimit(unit::ByteLength{6U});
        const auto destination = IpEndpoint{IpAddress::loopbackV4(), Port{9000U}};
        const auto first = UdpDatagram{destination, mem::ByteBlock{unit::ByteLength{4U}, mem::Byte{0x31U}}};
        const auto rejected = UdpDatagram{destination, mem::ByteBlock{unit::ByteLength{5U}, mem::Byte{0x52U}}};

        run(harness, [&]() -> void {
            harness.socket->events().onWritable([&]() -> void { writableCount += 1; });
            harness.socket->start(options);
        });
        run(harness, [&]() -> void {
            REQUIRE(harness.socket->send(first).isAccepted());
            REQUIRE(harness.socket->send(rejected).wouldBlock());
            REQUIRE_EQUAL(rejected.data(), (mem::ByteBlock{unit::ByteLength{5U}, mem::Byte{0x52U}}));
            REQUIRE_THROWS_AS(
                el::err::ParameterError,
                harness.socket->send(IpEndpoint{IpAddress::loopbackV4(), Port{}}, mem::ByteBlock{}));
            REQUIRE_THROWS_AS(
                el::err::ParameterError,
                harness.socket->send(IpEndpoint{IpAddress::loopbackV6(), Port{9000U}}, mem::ByteBlock{}));
            REQUIRE_THROWS_AS(
                el::err::ParameterError, harness.socket->send(destination, mem::ByteBlock{unit::ByteLength{6U}}));
            harness.device->callbacks.sendCompleted();
            REQUIRE_EQUAL(writableCount, 0);
        });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(writableCount, 1);
        REQUIRE_EQUAL(harness.device->sendAttempts.size(), std::size_t{1U});
        REQUIRE_EQUAL(harness.device->sendAttempts.front().data(), first.data());
    }

    void testEmptyDatagramsConsumeQueueCapacity() {
        auto harness = makeHarness();
        harness.device->sendResults.push_back(UdpSocketDeviceSendStatus::Pending);
        auto options = UdpSocketOptions{};
        options.setSendQueueLimit(unit::ByteLength::one());
        const auto destination = IpEndpoint{IpAddress::loopbackV4(), Port{9000U}};
        run(harness, [&]() -> void { harness.socket->start(options); });
        run(harness, [&]() -> void {
            REQUIRE(harness.socket->send(destination, mem::ByteBlock{}).isAccepted());
            REQUIRE(harness.socket->send(destination, mem::ByteBlock{}).wouldBlock());
        });
    }

    void testReceivePauseDropAndHandlerReplacement() {
        auto harness = makeHarness();
        auto oldHandlerCount = 0;
        auto newHandlerCount = 0;
        auto dropCount = 0;
        auto dropSize = std::optional<unit::ByteLength>{};
        const auto remote = IpEndpoint{IpAddress::loopbackV4(), Port{9001U}};
        run(harness, [&]() -> void {
            harness.socket->events().onDatagram([&](UdpDatagram) -> void { oldHandlerCount += 1; });
            harness.socket->events().onDatagram([&](UdpDatagram) -> void { newHandlerCount += 1; });
            harness.socket->events().onDatagramDropped([&](const UdpDatagramDropContext &context) -> void {
                dropCount += 1;
                REQUIRE_EQUAL(context.reason(), UdpDatagramDropReason::TooLarge);
                REQUIRE_EQUAL(context.maximumSize(), unit::ByteLength{65'507U});
                REQUIRE_EQUAL(context.remoteEndpoint(), std::optional<IpEndpoint>{remote});
                dropSize = context.datagramSize();
            });
            harness.socket->start();
        });
        run(harness, [&]() -> void {
            harness.socket->pauseReceiving();
            REQUIRE_FALSE(harness.device->receiving);
            harness.device->callbacks.datagram(UdpDatagram{remote, mem::ByteBlock{unit::ByteLength{2U}}});
        });
        run(harness, [&]() -> void {
            harness.socket->resumeReceiving();
            REQUIRE(harness.device->receiving);
            harness.device->callbacks.datagram(UdpDatagram{remote, mem::ByteBlock{unit::ByteLength{3U}}});
            harness.device->callbacks.datagramDropped(
                UdpDatagramDropContext{
                    UdpDatagramDropReason::TooLarge, unit::ByteLength{65'507U}, remote, unit::ByteLength{65'508U}});
        });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(oldHandlerCount, 0);
        REQUIRE_EQUAL(newHandlerCount, 1);
        REQUIRE_EQUAL(dropCount, 1);
        REQUIRE_EQUAL(dropSize, std::optional<unit::ByteLength>{unit::ByteLength{65'508U}});
    }

    void testGracefulCloseDrainsAndAbortSuppressesStaleCallbacks() {
        auto closing = makeHarness();
        closing.device->sendResults.push_back(UdpSocketDeviceSendStatus::Pending);
        auto closedCount = 0;
        const auto destination = IpEndpoint{IpAddress::loopbackV4(), Port{9000U}};
        run(closing, [&]() -> void {
            closing.socket->events().onClosed([&]() -> void {
                REQUIRE_EQUAL(closing.socket->state(), NetworkSourceState::Closed);
                closedCount += 1;
            });
            closing.socket->start();
        });
        run(closing, [&]() -> void {
            REQUIRE(closing.socket->send(destination, mem::ByteBlock{unit::ByteLength{2U}}).isAccepted());
            closing.socket->close();
            REQUIRE_EQUAL(closing.socket->state(), NetworkSourceState::Closing);
            REQUIRE_FALSE(closing.device->receiving);
        });
        REQUIRE_EQUAL(closedCount, 0);
        run(closing, [&]() -> void { closing.device->callbacks.sendCompleted(); });
        static_cast<void>(closing.loop->runUntilIdle());
        REQUIRE_EQUAL(closedCount, 1);
        REQUIRE(closing.device->closed);

        auto aborted = makeHarness();
        auto callbackCount = 0;
        run(aborted, [&]() -> void {
            aborted.socket->events()
                .onDatagram([&](UdpDatagram) -> void { callbackCount += 1; })
                .onClosed([&]() -> void { callbackCount += 1; })
                .onError([&](const NetworkErrorContext &) -> void { callbackCount += 1; });
            aborted.socket->start();
        });
        const auto callbacks = aborted.device->callbacks;
        auto abortThread = std::thread{[socket = aborted.socket]() -> void { socket->abort(); }};
        abortThread.join();
        callbacks.datagram(UdpDatagram{destination, mem::ByteBlock{unit::ByteLength{1U}}});
        callbacks.error(NetworkErrorContext{"stale"_el, "stale completion"_el});
        static_cast<void>(aborted.loop->runUntilIdle());
        REQUIRE_EQUAL(callbackCount, 0);
        REQUIRE(aborted.device->aborted);
        REQUIRE_EQUAL(aborted.socket->state(), NetworkSourceState::Closed);
    }

    void testInactiveAbortConsumesSocketLifetime() {
        auto harness = makeHarness();
        harness.socket->abort();
        REQUIRE_EQUAL(harness.socket->state(), NetworkSourceState::Closed);
        run(harness, [&]() -> void {
            REQUIRE_THROWS_AS(el::err::LogicError, harness.socket->start());
            REQUIRE(
                harness.socket->send(IpEndpoint{IpAddress::loopbackV4(), Port{9000U}}, mem::ByteBlock{}).isClosed());
        });
    }

    void testErrorStateOrderingAndCallbackExceptions() {
        auto failed = makeHarness();
        auto errorCount = 0;
        run(failed, [&]() -> void {
            failed.socket->events().onError([&](const NetworkErrorContext &context) -> void {
                REQUIRE_EQUAL(failed.socket->state(), NetworkSourceState::Failed);
                REQUIRE_EQUAL(context.reason(), NetworkErrorReason::NetworkUnreachable);
                errorCount += 1;
            });
            failed.socket->start();
        });
        run(failed, [&]() -> void {
            failed.device->callbacks.error(
                NetworkErrorContext{"UDP failed"_el, "network unavailable"_el}.setReason(
                    NetworkErrorReason::NetworkUnreachable));
        });
        static_cast<void>(failed.loop->runUntilIdle());
        REQUIRE_EQUAL(errorCount, 1);
        REQUIRE_EQUAL(failed.socket->state(), NetworkSourceState::Failed);
        run(failed, [&]() -> void { REQUIRE_THROWS_AS(el::err::LogicError, failed.socket->start()); });

        auto throwing = makeHarness();
        throwing.loop->invoke([&]() -> void {
            throwing.socket->events().onBound([]() -> void { throw std::runtime_error{"handler failure"}; });
            throwing.socket->start();
        });
        static_cast<void>(throwing.loop->runUntilIdle());
        REQUIRE(throwing.loop->hasError());
        REQUIRE_EQUAL(throwing.socket->state(), NetworkSourceState::Active);
        REQUIRE_THROWS_AS(std::runtime_error, std::rethrow_exception(throwing.loop->takeError()));
    }

private:
    [[nodiscard]] static auto makeHarness() -> Harness {
        auto result = Harness{};
        result.driver = EventLoopDriver::createDefault();
        result.loop = EventLoop::create(result.driver);
        result.device = std::make_shared<FakeState>();
        const auto state = result.device;
        result.implementation = std::make_shared<el::network::impl::UdpSocket>(
            result.loop,
            result.driver,
            [state](EventLoopDriverPtr, UdpSocketDeviceCallbacks callbacks) -> UdpSocketDevicePtr {
                state->callbacks = std::move(callbacks);
                return std::make_unique<FakeDevice>(state);
            });
        result.socket = result.implementation;
        return result;
    }

    static void run(Harness &harness, std::function<void()> callback) {
        harness.loop->invoke(std::move(callback));
        static_cast<void>(harness.loop->runUntilIdle());
    }
};
