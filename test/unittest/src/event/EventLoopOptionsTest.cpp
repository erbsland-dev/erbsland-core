// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopDriver.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/UnmanagedEventThread.hpp>

#include <atomic>
#include <future>
#include <thread>
#include <vector>

using namespace el::event;
using namespace el::text::literals;

TESTED_TARGETS(EventLoopOptions EventLoop EventQueueOverflowAction EventLoopErrorPolicy)
class EventLoopOptionsTest final : public el::UnitTest {
private:
    auto bounded(EventQueueOverflowAction action) -> EventLoopOptions {
        return EventLoopOptions{}.setMaximumQueuedEvents(1).setQueueOverflowAction(action);
    }

public:
    void testDefaultsAndChaining() {
        auto options = EventLoopOptions{};
        REQUIRE_EQUAL(options.maximumQueuedEvents(), std::size_t{0});
        REQUIRE_EQUAL(options.queueOverflowAction(), EventQueueOverflowAction::Terminate);
        REQUIRE_EQUAL(options.errorPolicy(), EventLoopErrorPolicy::Store);
        REQUIRE_EQUAL(
            &options.setMaximumQueuedEvents(2)
                .setQueueOverflowAction(EventQueueOverflowAction::Ignore)
                .setErrorPolicy(EventLoopErrorPolicy::Ignore),
            &options);
        const auto loop = EventLoop::create(options);
        options.setMaximumQueuedEvents(1);
        loop->invoke([]() -> void {});
        loop->invoke([]() -> void {});
        loop->invoke([]() -> void {});
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{2});
        REQUIRE_EQUAL(loop->runUntilIdle(), std::size_t{2});
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{0});
    }

    void testCountExcludesRunningDelayedAndControl() {
        const auto loop = EventLoop::create();
        auto insideCount = std::size_t{99};
        loop->invoke([&]() -> void { insideCount = loop->queuedEventCount(); });
        loop->invokeAfter(el::time::TimeDelta::seconds(60), []() -> void {});
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{1});
        loop->quit();
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{1});
        loop->run();
        REQUIRE_EQUAL(insideCount, std::size_t{0});
    }

    void testOverflowIgnoreAndQuit() {
        for (const auto action : {EventQueueOverflowAction::Ignore, EventQueueOverflowAction::Quit}) {
            const auto loop = EventLoop::create(bounded(action));
            auto first = false;
            auto second = false;
            loop->invoke([&]() -> void { first = true; });
            loop->invoke([&]() -> void { second = true; });
            REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{1});
            static_cast<void>(loop->runUntilIdle());
            REQUIRE(first);
            REQUIRE_FALSE(second);
            REQUIRE_EQUAL(loop->isQuitRequested(), action == EventQueueOverflowAction::Quit);
        }
    }

    void testBlockResumesWhenSpaceBecomesAvailable() {
        const auto loop = EventLoop::create(bounded(EventQueueOverflowAction::Block));
        auto started = std::promise<void>{};
        auto completed = std::promise<void>{};
        auto executed = false;
        loop->invoke([]() -> void {});
        auto producer = std::jthread{[&]() -> void {
            started.set_value();
            loop->invoke([&]() -> void { executed = true; });
            completed.set_value();
        }};
        started.get_future().wait();
        const auto processed = loop->runOnce(el::time::TimeDelta::zero());
        completed.get_future().wait();
        producer.join();
        REQUIRE(processed);
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{1});
        REQUIRE_EQUAL(loop->runUntilIdle(), std::size_t{1});
        REQUIRE(executed);
    }

    void testShutdownReleasesBlockedProducers() {
        for (const auto quit : {false, true}) {
            const auto loop = EventLoop::create(bounded(EventQueueOverflowAction::Block));
            auto started = std::promise<void>{};
            loop->invoke([]() -> void {});
            auto producer = std::jthread{[&]() -> void {
                started.set_value();
                loop->invoke([]() -> void {});
            }};
            started.get_future().wait();
            if (quit) {
                loop->quit();
            } else {
                loop->stop();
            }
            producer.join();
            REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{1});
        }
    }

    void testConcurrentAdmissionHonorsCapacity() {
        const auto loop = EventLoop::create(
            EventLoopOptions{}.setMaximumQueuedEvents(16).setQueueOverflowAction(EventQueueOverflowAction::Ignore));
        auto producers = std::vector<std::jthread>{};
        for (auto i = 0; i < 4; ++i) {
            producers.emplace_back([&]() -> void {
                for (auto j = 0; j < 100; ++j) {
                    loop->invoke([]() -> void {});
                }
            });
        }
        producers.clear();
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{16});
    }

    void testDefaultErrorPoliciesAndHandlerOverride() {
        for (
            const auto policy :
            {EventLoopErrorPolicy::Ignore, EventLoopErrorPolicy::Store, EventLoopErrorPolicy::Quit}) {
            const auto loop = EventLoop::create(EventLoopOptions{}.setErrorPolicy(policy));
            auto continued = false;
            loop->invoke([]() -> void { throw el::err::RuntimeError{"failure"_el}; });
            loop->invoke([&]() -> void { continued = true; });
            static_cast<void>(loop->runUntilIdle());
            REQUIRE(continued);
            REQUIRE_EQUAL(loop->hasError(), policy != EventLoopErrorPolicy::Ignore);
            REQUIRE_EQUAL(loop->isQuitRequested(), policy == EventLoopErrorPolicy::Quit);
        }
        const auto loop = EventLoop::create(EventLoopOptions{}.setErrorPolicy(EventLoopErrorPolicy::Ignore));
        loop->setErrorHandler(
            [](std::exception_ptr) -> EventLoopErrorAction { return EventLoopErrorAction::Continue; });
        loop->invoke([]() -> void { throw el::err::RuntimeError{"saved"_el}; });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE(loop->takeError());
        loop->setErrorHandler({});
        loop->invoke([]() -> void { throw el::err::RuntimeError{"ignored"_el}; });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_FALSE(loop->hasError());
    }

    void testRejectedRepeatingTimerReschedules() {
        const auto loop = EventLoop::create(bounded(EventQueueOverflowAction::Ignore));
        auto fired = false;
        const auto timer = loop->createTimer([&]() -> void { fired = true; });
        timer->startFixedDelay(el::time::TimeDelta::milliseconds(1));
        // An already due timer is rejected while the only queue slot is occupied.
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        loop->invoke([]() -> void {});
        REQUIRE(loop->runOnce(el::time::TimeDelta::zero()));
        REQUIRE_FALSE(fired);
        REQUIRE(timer->isActive());
        REQUIRE(loop->runOnce(el::time::TimeDelta::milliseconds(50)));
        REQUIRE(fired);
        timer->stop();
    }

    void testRejectedTimerOccurrencesFinishOnQuit() {
        for (const auto action : {EventQueueOverflowAction::Ignore, EventQueueOverflowAction::Quit}) {
            const auto loop = EventLoop::create(bounded(action));
            auto fired = false;
            const auto timer = loop->createTimer([&]() -> void { fired = true; });
            timer->startOnce(el::time::TimeDelta::zero());
            loop->invoke([]() -> void {});
            REQUIRE(loop->runOnce(el::time::TimeDelta::zero()));
            REQUIRE_FALSE(fired);
            REQUIRE_FALSE(timer->isActive());
        }
    }

    void testDriverFactoryPropagatesOptions() {
        const auto loop =
            EventLoop::create(EventLoopDriver::createDefault(), bounded(EventQueueOverflowAction::Ignore));
        loop->invoke([]() -> void {});
        loop->invoke([]() -> void {});
        REQUIRE_EQUAL(loop->queuedEventCount(), std::size_t{1});
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            EventLoop::create(EventLoopOptions{}.setErrorPolicy(static_cast<EventLoopErrorPolicy>(255))));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            EventLoop::create(EventLoopOptions{}.setQueueOverflowAction(static_cast<EventQueueOverflowAction>(255))));
    }

    void testUnsupportedApplicationPoliciesAndUnmanagedPropagation() {
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            EventLoop::create(EventLoopOptions{}.setErrorPolicy(EventLoopErrorPolicy::FailApplication)));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            UnmanagedEventThread::create(bounded(EventQueueOverflowAction::FailApplication)));
        const auto worker = UnmanagedEventThread::create(bounded(EventQueueOverflowAction::Ignore));
        worker->loop()->invoke([]() -> void {});
        worker->loop()->invoke([]() -> void {});
        REQUIRE_EQUAL(worker->workerEventLoop().queuedEventCount(), std::size_t{1});
    }
};
