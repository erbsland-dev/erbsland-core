// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/event/EventSubscription.hpp>
#include <erbsland/event/impl/EventCallbackList.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(EventSubscription EventCallbackList)
class EventSubscriptionTest final : public el::UnitTest {
private:
    using Callback = std::function<void(int)>;
    using CallbackList = el::event::impl::EventCallbackList<Callback>;

public:
    void testOrderedDeliveryAndCancellation() {
        auto callbacks = CallbackList{};
        auto values = std::vector<int>{};
        auto first = callbacks.add([&values](const int value) -> void { values.push_back(value); });
        auto second = callbacks.add([&values](const int value) -> void { values.push_back(value * 2); });

        callbacks.notify([](std::exception_ptr) -> void {}, 3);
        REQUIRE_EQUAL(values.size(), std::size_t{2});
        REQUIRE_EQUAL(values[0], 3);
        REQUIRE_EQUAL(values[1], 6);

        first.cancel();
        values.clear();
        callbacks.notify([](std::exception_ptr) -> void {}, 4);
        REQUIRE_EQUAL(values.size(), std::size_t{1});
        REQUIRE_EQUAL(values[0], 8);
        REQUIRE_FALSE(first.isActive());
        REQUIRE(second.isActive());
    }

    void testMoveAndPublisherLifetime() {
        auto callbacks = std::make_unique<CallbackList>();
        auto subscription = callbacks->add([](int) -> void {});
        REQUIRE(subscription.isActive());

        auto moved = std::move(subscription);
        REQUIRE_FALSE(subscription.isActive());
        REQUIRE(moved.isActive());

        callbacks.reset();
        REQUIRE_FALSE(moved.isActive());
    }

    void testSelfCancellationAndMutationDuringDispatch() {
        auto callbacks = CallbackList{};
        auto calls = std::vector<int>{};
        auto self = el::event::EventSubscription{};
        auto added = el::event::EventSubscription{};
        self = callbacks.add([&](int) -> void {
            calls.push_back(1);
            self.cancel();
            added = callbacks.add([&calls](int) -> void { calls.push_back(3); });
        });
        auto second = callbacks.add([&calls](int) -> void { calls.push_back(2); });

        callbacks.notify([](std::exception_ptr) -> void {}, 0);
        REQUIRE_EQUAL(calls.size(), std::size_t{2});
        callbacks.notify([](std::exception_ptr) -> void {}, 0);
        REQUIRE_EQUAL(calls.size(), std::size_t{4});
        REQUIRE_EQUAL(calls[2], 2);
        REQUIRE_EQUAL(calls[3], 3);
        REQUIRE(second.isActive());
    }

    void testExceptionsAreIsolatedAndReported() {
        auto callbacks = CallbackList{};
        auto errors = std::vector<std::exception_ptr>{};
        auto called = false;
        auto throwing = callbacks.add([](int) -> void { throw el::err::RuntimeError{"expected"_el}; });
        auto later = callbacks.add([&called](int) -> void { called = true; });

        callbacks.notify([&errors](std::exception_ptr error) -> void { errors.emplace_back(std::move(error)); }, 0);
        REQUIRE(called);
        REQUIRE_EQUAL(errors.size(), std::size_t{1});
        REQUIRE_THROWS_AS(el::err::RuntimeError, std::rethrow_exception(errors.front()));
        REQUIRE(throwing.isActive());
        REQUIRE(later.isActive());
    }

    void testConcurrentRegistrationAndCancellation() {
        auto callbacks = CallbackList{};
        auto calls = std::atomic<int>{0};
        auto subscriptions = std::vector<el::event::EventSubscription>{};
        subscriptions.reserve(32);
        auto registrationThread = std::thread{[&]() -> void {
            for (auto index = 0; index < 32; ++index) {
                subscriptions.emplace_back(callbacks.add([&calls](int) -> void { ++calls; }));
            }
        }};
        registrationThread.join();
        callbacks.notify([](std::exception_ptr) -> void {}, 0);
        REQUIRE_EQUAL(calls.load(), 32);
        for (auto &subscription : subscriptions) {
            subscription.cancel();
        }
        callbacks.notify([](std::exception_ptr) -> void {}, 0);
        REQUIRE_EQUAL(calls.load(), 32);
    }
};
