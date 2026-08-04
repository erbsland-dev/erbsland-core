// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/source/NetworkErrorContext.hpp>
#include <erbsland/network/udp/UdpDatagramDropContext.hpp>
#include <erbsland/network/udp/UdpDatagramDropReason.hpp>
#include <erbsland/network/udp/UdpSocket.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/time/TimeUnitTags.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;
using namespace el::time;

TESTED_TARGETS(UdpSocket Network UdpDatagramDropContext)
class UdpSocketLiveTest final : public el::UnitTest {
public:
    void testIpv4MultipleRemotesAndRepeatedDatagrams() {
        const auto loop = EventLoop::create();
        auto receiver = UdpSocketPtr{};
        auto firstSender = UdpSocketPtr{};
        auto secondSender = UdpSocketPtr{};
        auto boundCount = 0;
        auto closedCount = 0;
        auto error = std::optional<NetworkErrorContext>{};
        auto received = std::vector<UdpDatagram>{};

        loop->invoke([&]() -> void {
            auto &network = loop->get<Network>();
            receiver = network.createUdpSocket();
            firstSender = network.createUdpSocket();
            secondSender = network.createUdpSocket();
            configure(receiver, boundCount, closedCount, error);
            configure(firstSender, boundCount, closedCount, error);
            configure(secondSender, boundCount, closedCount, error);
            receiver->events().onDatagram(
                [&](UdpDatagram datagram) -> void { received.emplace_back(std::move(datagram)); });
            receiver->start(IpAddress::loopbackV4());
            firstSender->start(IpAddress::loopbackV4());
            secondSender->start(IpAddress::loopbackV4());
        });
        runUntil(loop, [&]() -> bool { return boundCount == 3 || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE(receiver->localEndpoint().has_value());
        REQUIRE_FALSE(receiver->localEndpoint()->port().isAutomatic());
        const auto destination = *receiver->localEndpoint();
        const auto firstSenderFirstData = block(0x11U, 3U);
        const auto firstSenderSecondData = block(0x12U, 5U);
        const auto secondSenderFirstData = block(0x21U, 4U);
        const auto secondSenderSecondData = block(0x22U, 6U);

        loop->invoke([&]() -> void {
            REQUIRE(firstSender->send(destination, firstSenderFirstData).isAccepted());
            REQUIRE(secondSender->send(destination, secondSenderFirstData).isAccepted());
            REQUIRE(firstSender->send(destination, firstSenderSecondData).isAccepted());
            REQUIRE(secondSender->send(destination, secondSenderSecondData).isAccepted());
        });
        runUntil(loop, [&]() -> bool { return received.size() == 4U || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(received.size(), std::size_t{4U});

        auto fromFirst = std::size_t{};
        auto fromSecond = std::size_t{};
        for (const auto &datagram : received) {
            if (datagram.remoteEndpoint() == *firstSender->localEndpoint()) {
                fromFirst += 1U;
                const auto data = datagram.data();
                if (data != firstSenderFirstData && data != firstSenderSecondData) {
                    REQUIRE_EQUAL(data, firstSenderFirstData);
                }
            } else if (datagram.remoteEndpoint() == *secondSender->localEndpoint()) {
                fromSecond += 1U;
                const auto data = datagram.data();
                if (data != secondSenderFirstData && data != secondSenderSecondData) {
                    REQUIRE_EQUAL(data, secondSenderFirstData);
                }
            } else {
                REQUIRE(false);
            }
        }
        REQUIRE_EQUAL(fromFirst, std::size_t{2U});
        REQUIRE_EQUAL(fromSecond, std::size_t{2U});

        loop->invoke([&]() -> void {
            receiver->close();
            firstSender->close();
            secondSender->close();
        });
        runUntil(loop, [&]() -> bool { return closedCount == 3 || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(closedCount, 3);
    }

    void testIpv6BindingAndDatagram() {
        const auto loop = EventLoop::create();
        auto receiver = UdpSocketPtr{};
        auto sender = UdpSocketPtr{};
        auto boundCount = 0;
        auto received = std::optional<UdpDatagram>{};
        auto error = std::optional<NetworkErrorContext>{};

        loop->invoke([&]() -> void {
            auto &network = loop->get<Network>();
            receiver = network.createUdpSocket();
            sender = network.createUdpSocket();
            receiver->events()
                .onBound([&]() -> void { boundCount += 1; })
                .onDatagram([&](UdpDatagram datagram) -> void { received = std::move(datagram); })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; });
            sender->events()
                .onBound([&]() -> void { boundCount += 1; })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; });
            receiver->start(IpAddress::loopbackV6());
            sender->start(IpAddress::loopbackV6());
        });
        runUntil(loop, [&]() -> bool { return boundCount == 2 || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(receiver->localEndpoint()->address(), IpAddress::loopbackV6());

        loop->invoke(
            [&]() -> void { REQUIRE(sender->send(*receiver->localEndpoint(), block(0x6aU, 7U)).isAccepted()); });
        runUntil(loop, [&]() -> bool { return received.has_value() || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(received->remoteEndpoint(), *sender->localEndpoint());
        REQUIRE_EQUAL(received->data(), block(0x6aU, 7U));
        receiver->abort();
        sender->abort();
    }

    void testOversizedDatagramIsDroppedWithoutFailingSocket() {
        const auto loop = EventLoop::create();
        auto receiver = UdpSocketPtr{};
        auto sender = UdpSocketPtr{};
        auto boundCount = 0;
        auto deliveredCount = 0;
        auto drop = std::optional<UdpDatagramDropContext>{};
        auto error = std::optional<NetworkErrorContext>{};

        loop->invoke([&]() -> void {
            auto &network = loop->get<Network>();
            receiver = network.createUdpSocket();
            sender = network.createUdpSocket();
            receiver->events()
                .onBound([&]() -> void { boundCount += 1; })
                .onDatagram([&](UdpDatagram) -> void { deliveredCount += 1; })
                .onDatagramDropped([&](const UdpDatagramDropContext &context) -> void { drop = context; })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; });
            sender->events()
                .onBound([&]() -> void { boundCount += 1; })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; });
            auto options = UdpSocketOptions{};
            options.setMaximumDatagramSize(el::unit::ByteLength{4U});
            receiver->start(IpAddress::loopbackV4(), options);
            sender->start(IpAddress::loopbackV4());
        });
        runUntil(loop, [&]() -> bool { return boundCount == 2 || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        loop->invoke(
            [&]() -> void { REQUIRE(sender->send(*receiver->localEndpoint(), block(0x7bU, 5U)).isAccepted()); });
        runUntil(loop, [&]() -> bool { return drop.has_value() || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE_EQUAL(deliveredCount, 0);
        REQUIRE_EQUAL(receiver->state(), NetworkSourceState::Active);
        REQUIRE_EQUAL(drop->reason(), UdpDatagramDropReason::TooLarge);
        REQUIRE_EQUAL(drop->maximumSize(), el::unit::ByteLength{4U});
        REQUIRE(drop->remoteEndpoint().has_value());
        REQUIRE_EQUAL(*drop->remoteEndpoint(), *sender->localEndpoint());
        receiver->abort();
        sender->abort();
    }

    void testCrossThreadAbortEmitsNoClosureCallback() {
        const auto loop = EventLoop::create();
        auto socket = UdpSocketPtr{};
        auto bound = false;
        auto terminalCallbackCount = 0;
        loop->invoke([&]() -> void {
            socket = loop->get<Network>().createUdpSocket();
            socket->events()
                .onBound([&]() -> void { bound = true; })
                .onClosed([&]() -> void { terminalCallbackCount += 1; })
                .onError([&](const NetworkErrorContext &) -> void { terminalCallbackCount += 1; });
            socket->start(IpAddress::loopbackV4());
        });
        runUntil(loop, [&]() -> bool { return bound; });
        auto thread = std::thread{[socket]() -> void { socket->abort(); }};
        thread.join();
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_EQUAL(socket->state(), NetworkSourceState::Closed);
        REQUIRE_EQUAL(terminalCallbackCount, 0);
    }

private:
    static void configure(
        const UdpSocketPtr &socket, int &boundCount, int &closedCount, std::optional<NetworkErrorContext> &error) {
        socket->events()
            .onBound([&boundCount]() -> void { boundCount += 1; })
            .onClosed([&closedCount]() -> void { closedCount += 1; })
            .onError([&error](const NetworkErrorContext &context) -> void { error = context; });
    }

    [[nodiscard]] static auto block(const std::uint8_t value, const std::size_t length) -> el::mem::ByteBlock {
        return el::mem::ByteBlock{el::unit::ByteLength::fromSizeT(length), el::mem::Byte{value}};
    }

    template <typename Predicate>
    void runUntil(const EventLoopPtr &loop, Predicate predicate) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
        while (!predicate() && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(TimeDelta{Milliseconds{25}}));
        }
        REQUIRE(predicate());
    }
};
