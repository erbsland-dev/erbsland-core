// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/EventLoopDriver.hpp>
#include <erbsland/network/impl/tcp/TcpAcceptedSocket.hpp>
#include <erbsland/network/impl/tcp/TcpListener.hpp>
#include <erbsland/network/impl/tcp/TcpListenerDevice.hpp>
#include <erbsland/network/source/ConnectionQuota.hpp>
#include <erbsland/network/tcp/TcpConnectionRequest.hpp>
#include <erbsland/network/tcp/TcpListener.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

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
namespace unit = el::unit;

TESTED_TARGETS(
    TcpListener TcpListenerOptions TcpListenerEventEditor TcpConnectionRequest TcpConnectionRequestState
        TcpConnectionFilterFn TcpConnectionFilterResult TcpListenerDevice ConnectionQuota ConnectionQuotaLease)
class TcpListenerTest final : public el::UnitTest {
    class FakeSocket final : public TcpAcceptedSocket {
    public:
        FakeSocket(IpEndpoint local, IpEndpoint remote, std::shared_ptr<int> destructionCount = {}) :
            _local{std::move(local)}, _remote{std::move(remote)}, _destructionCount{std::move(destructionCount)} {}
        ~FakeSocket() override {
            if (_destructionCount != nullptr) {
                *_destructionCount += 1;
            }
        }
        auto localEndpoint() const noexcept -> const IpEndpoint & override { return _local; }
        auto remoteEndpoint() const noexcept -> const IpEndpoint & override { return _remote; }

    private:
        IpEndpoint _local;
        IpEndpoint _remote;
        std::shared_ptr<int> _destructionCount;
    };

    struct FakeState final {
        TcpListenerDeviceCallbacks callbacks;
        std::optional<IpEndpoint> requestedEndpoint;
        unit::ItemCount backlog;
        std::vector<bool> acceptingChanges;
        bool closed{false};
        bool aborted{false};
    };

    class FakeDevice final : public TcpListenerDevice {
    public:
        explicit FakeDevice(std::shared_ptr<FakeState> state) : _state{std::move(state)} {}
        auto start(IpEndpoint endpoint, const unit::ItemCount backlog) -> IpEndpoint override {
            _state->requestedEndpoint = endpoint;
            _state->backlog = backlog;
            return IpEndpoint{endpoint.address(), Port{49000U}, endpoint.scopeId()};
        }
        void setAccepting(const bool enabled) override { _state->acceptingChanges.push_back(enabled); }
        void close() noexcept override { _state->closed = true; }
        void abort() noexcept override { _state->aborted = true; }

    private:
        std::shared_ptr<FakeState> _state;
    };

    struct Harness final {
        EventLoopDriverPtr driver;
        EventLoopPtr loop;
        std::shared_ptr<FakeState> device;
        std::shared_ptr<el::network::impl::TcpListener> implementation;
        el::network::TcpListenerPtr listener;
    };

public:
    void testStartFilterPendingLimitAndThreadSafeReject() {
        auto harness = makeHarness();
        auto filterEndpoints = std::vector<IpEndpoint>{};
        auto requests = std::vector<el::network::TcpConnectionRequestPtr>{};
        auto listening = false;
        auto options = TcpListenerOptions{};
        options.setBacklog(unit::ItemCount{23U})
            .setMaximumPendingRequests(unit::ItemCount{1U})
            .setConnectionFilter([&](const IpEndpoint &endpoint) {
                filterEndpoints.emplace_back(endpoint);
                return endpoint.port() == Port{41000U} ? TcpConnectionFilterResult::Reject
                                                       : TcpConnectionFilterResult::Accept;
            });
        run(harness, [&]() -> void {
            harness.listener->events()
                .onListening([&]() -> void { listening = true; })
                .onConnection([&](el::network::TcpConnectionRequestPtr request) -> void {
                    requests.emplace_back(std::move(request));
                });
            harness.listener->start(IpEndpoint{IpAddress::loopbackV4(), Port{}}, options);
        });
        REQUIRE(listening);
        REQUIRE_EQUAL(harness.device->backlog, unit::ItemCount{23U});
        REQUIRE_EQUAL(harness.listener->localEndpoint()->port(), Port{49000U});
        REQUIRE(harness.device->acceptingChanges.back());

        const auto destroyed = std::make_shared<int>(0);
        run(harness, [&]() -> void {
            harness.device->callbacks.accepted(makeSocket(41000U, destroyed));
            harness.device->callbacks.accepted(makeSocket(41001U));
        });
        REQUIRE_EQUAL(filterEndpoints.size(), std::size_t{2U});
        REQUIRE_EQUAL(*destroyed, 1);
        REQUIRE_EQUAL(requests.size(), std::size_t{1U});
        REQUIRE_FALSE(harness.device->acceptingChanges.back());

        auto rejectThread = std::thread{[request = requests.front()]() -> void {
            request->reject();
            request->reject();
        }};
        rejectThread.join();
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(requests.front()->state(), TcpConnectionRequestState::Rejected);
        REQUIRE(harness.device->acceptingChanges.back());
    }

    void testFilterExceptionClosesSocketAndReachesLoopErrorHandling() {
        auto harness = makeHarness();
        const auto destroyed = std::make_shared<int>(0);
        run(harness, [&]() -> void {
            harness.listener->start(
                IpEndpoint{IpAddress::loopbackV4(), Port{}},
                TcpListenerOptions{}.setConnectionFilter([](const IpEndpoint &) -> TcpConnectionFilterResult {
                    throw std::runtime_error{"filter failure"};
                }));
        });
        harness.loop->invoke([&]() -> void { harness.device->callbacks.accepted(makeSocket(42000U, destroyed)); });
        REQUIRE(harness.loop->runOnce());
        REQUIRE(harness.loop->hasError());
        REQUIRE_EQUAL(*destroyed, 1);
        REQUIRE_THROWS_AS(std::runtime_error, std::rethrow_exception(harness.loop->takeError()));
    }

    void testClosingKeepsEmittedRequestValidAndOrdersFinal() {
        auto harness = makeHarness();
        auto request = el::network::TcpConnectionRequestPtr{};
        auto events = std::vector<int>{};
        run(harness, [&]() -> void {
            harness.listener->events()
                .onConnection([&](el::network::TcpConnectionRequestPtr value) -> void { request = std::move(value); })
                .onClosed([&]() -> void { events.push_back(1); })
                .onFinal([&]() -> void { events.push_back(2); });
            harness.listener->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });
        run(harness, [&]() -> void { harness.device->callbacks.accepted(makeSocket(43000U)); });
        REQUIRE(request != nullptr);
        run(harness, [&]() -> void { harness.listener->close(); });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_EQUAL(events, std::vector<int>({1, 2}));
        REQUIRE(harness.device->closed);
        REQUIRE_EQUAL(request->state(), TcpConnectionRequestState::Pending);
        request->reject();
        REQUIRE_EQUAL(request->state(), TcpConnectionRequestState::Rejected);
    }

    void testSharedQuotaRejectsAndWakesMultipleListeners() {
        auto first = makeHarness();
        auto second = makeHarness();
        const auto quota = ConnectionQuota::create(unit::ItemCount{1U});
        auto firstRequest = el::network::TcpConnectionRequestPtr{};
        auto secondRequest = el::network::TcpConnectionRequestPtr{};
        auto rejectedSocketDestructions = std::make_shared<int>(0);
        run(first, [&]() -> void {
            first.listener->events().onConnection(
                [&](el::network::TcpConnectionRequestPtr request) -> void { firstRequest = std::move(request); });
            first.listener->start(
                IpEndpoint{IpAddress::loopbackV4(), Port{}}, TcpListenerOptions{}.setConnectionQuota(quota));
        });
        run(second, [&]() -> void {
            second.listener->events().onConnection(
                [&](el::network::TcpConnectionRequestPtr request) -> void { secondRequest = std::move(request); });
            second.listener->start(
                IpEndpoint{IpAddress::loopbackV4(), Port{}}, TcpListenerOptions{}.setConnectionQuota(quota));
        });

        run(first, [&]() -> void { first.device->callbacks.accepted(makeSocket(43100U)); });
        REQUIRE(firstRequest != nullptr);
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{1U});
        REQUIRE_FALSE(first.device->acceptingChanges.back());

        run(second,
            [&]() -> void { second.device->callbacks.accepted(makeSocket(43101U, rejectedSocketDestructions)); });
        REQUIRE(secondRequest == nullptr);
        REQUIRE_EQUAL(*rejectedSocketDestructions, 1);
        REQUIRE_FALSE(second.device->acceptingChanges.back());

        firstRequest->reject();
        static_cast<void>(first.loop->runUntilIdle());
        static_cast<void>(second.loop->runUntilIdle());
        REQUIRE_EQUAL(quota->current(), unit::ItemCount{});
        REQUIRE(first.device->acceptingChanges.back());
        REQUIRE(second.device->acceptingChanges.back());

        const auto firstDefaults = TcpListenerOptions{};
        const auto secondDefaults = TcpListenerOptions{};
        REQUIRE(firstDefaults.connectionQuota() != secondDefaults.connectionQuota());
        REQUIRE_EQUAL(firstDefaults.connectionQuota()->maximum(), unit::ItemCount{1024U});
    }

    void testAbortPostsOnlyFinal() {
        auto harness = makeHarness();
        auto closed = false;
        auto error = false;
        auto final = false;
        run(harness, [&]() -> void {
            harness.listener->events()
                .onClosed([&]() -> void { closed = true; })
                .onError([&](const NetworkErrorContext &) -> void { error = true; })
                .onFinal([&]() -> void { final = true; });
            harness.listener->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
            harness.listener->abort();
        });
        static_cast<void>(harness.loop->runUntilIdle());
        REQUIRE_FALSE(closed);
        REQUIRE_FALSE(error);
        REQUIRE(final);
        REQUIRE(harness.device->aborted);
    }

private:
    [[nodiscard]] static auto makeHarness() -> Harness {
        auto result = Harness{};
        result.driver = EventLoopDriver::createDefault();
        result.loop = EventLoop::create(result.driver);
        result.device = std::make_shared<FakeState>();
        const auto state = result.device;
        result.implementation = std::make_shared<el::network::impl::TcpListener>(
            result.loop,
            result.driver,
            [state](EventLoopDriverPtr, TcpListenerDeviceCallbacks callbacks) -> TcpListenerDevicePtr {
                state->callbacks = std::move(callbacks);
                return std::make_unique<FakeDevice>(state);
            });
        result.listener = result.implementation;
        return result;
    }

    [[nodiscard]] static auto makeSocket(const std::uint16_t port, std::shared_ptr<int> destructionCount = {})
        -> TcpAcceptedSocketPtr {
        return std::make_unique<FakeSocket>(
            IpEndpoint{IpAddress::loopbackV4(), Port{49000U}},
            IpEndpoint{IpAddress::loopbackV4(), Port{port}},
            std::move(destructionCount));
    }

    static void run(Harness &harness, std::function<void()> callback) {
        harness.loop->invoke(std::move(callback));
        static_cast<void>(harness.loop->runUntilIdle());
    }
};
