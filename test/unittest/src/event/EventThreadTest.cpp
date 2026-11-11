// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CurrentLoop.hpp>
#include <erbsland/Event.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/UnmanagedEventThread.hpp>

#include <atomic>
#include <exception>
#include <latch>
#include <thread>

using namespace el::event;

TESTED_TARGETS(EventThread UnmanagedEventThread EventLoop EventTarget)
class EventThreadTest final : public el::UnitTest {
public:
    void testCurrentLoopOutsideLoopThrows() { REQUIRE_THROWS_AS(el::err::LogicFailure, currentLoop()); }

    void testStartInvokeAndJoin() {
        const auto thread = UnmanagedEventThread::create();
        auto called = std::atomic<bool>{false};

        thread->start();
        thread->loop()->invoke([thread, &called]() -> void {
            called = true;
            thread->quit();
        });
        thread->join();

        REQUIRE(thread->isStarted());
        REQUIRE_FALSE(thread->isRunning());
        REQUIRE(called.load());
    }

    void testCallbackRunsOnWorkerThread() {
        const auto thread = UnmanagedEventThread::create();
        const auto mainThreadId = std::this_thread::get_id();
        auto callbackThreadId = std::thread::id{};

        thread->start();
        thread->loop()->invoke([thread, &callbackThreadId]() -> void {
            callbackThreadId = std::this_thread::get_id();
            thread->quit();
        });
        thread->join();

        REQUIRE_NOT_EQUAL(callbackThreadId, std::thread::id{});
        REQUIRE_NOT_EQUAL(callbackThreadId, mainThreadId);
    }

    void testStartIsOneShot() {
        const auto thread = UnmanagedEventThread::create();

        thread->start();
        thread->quit();
        thread->join();

        REQUIRE_THROWS_AS(el::err::LogicFailure, thread->start());
    }

    void testJoinBeforeStartAndAfterJoinIsNoOperation() {
        const auto thread = UnmanagedEventThread::create();

        thread->join();
        REQUIRE_FALSE(thread->isStarted());
        REQUIRE_FALSE(thread->stopToken().isStopRequested());
        thread->start();
        thread->quit();
        thread->join();
        thread->join();

        REQUIRE(thread->isStarted());
        REQUIRE_FALSE(thread->isRunning());
    }

    void testJoinFromEventThreadThrows() {
        const auto thread = UnmanagedEventThread::create();
        auto exception = std::exception_ptr{};
        auto callbackCompleted = std::atomic<bool>{false};

        thread->start();
        thread->loop()->invoke([thread, &exception, &callbackCompleted]() -> void {
            try {
                thread->join();
            } catch (...) {
                exception = std::current_exception();
            }
            thread->quit();
            callbackCompleted = true;
        });
        while (!callbackCompleted.load()) {
            std::this_thread::yield();
        }
        thread->join();

        REQUIRE(exception != nullptr);
        REQUIRE_THROWS_AS(el::err::LogicFailure, std::rethrow_exception(exception));
    }

    void testQuitBeforeStart() {
        const auto thread = UnmanagedEventThread::create();
        const auto token = thread->stopToken();
        REQUIRE_FALSE(token.isStopRequested());

        thread->quit();
        thread->quit();
        REQUIRE(token.isStopRequested());
        REQUIRE(thread->stopToken().isStopRequested());
        REQUIRE(thread->workerEventLoop().isQuitRequested());
        thread->start();
        thread->join();

        REQUIRE(thread->isStarted());
        REQUIRE_FALSE(thread->isRunning());
    }

    void testTokenOutlivesThreadWithoutRetainingIt() {
        auto thread = UnmanagedEventThread::create();
        const auto other = UnmanagedEventThread::create();
        const auto weak = UnmanagedEventThreadWeakPtr{thread};
        const auto token = thread->stopToken();
        const auto copy = token;
        thread.reset();
        REQUIRE(weak.expired());
        REQUIRE(token.isStopRequested());
        REQUIRE(copy.isStopRequested());
        REQUIRE_FALSE(other->stopToken().isStopRequested());
    }

    void testLoopQuitSignalsExecutingCallbackAndDrainsQueue() {
        const auto thread = UnmanagedEventThread::create();
        const auto token = thread->stopToken();
        auto entered = std::latch{1};
        auto release = std::latch{1};
        auto observed = false;
        auto drained = false;
        auto rejectedRan = false;
        thread->loop()->invoke([token, &entered, &release, &observed]() -> void {
            entered.count_down();
            release.wait();
            observed = token.isStopRequested();
        });
        thread->loop()->invoke([&drained]() -> void { drained = true; });
        thread->start();
        entered.wait();
        thread->workerEventLoop().quit();
        const auto signalledBeforeReturn = token.isStopRequested();
        thread->loop()->invoke([&rejectedRan]() -> void { rejectedRan = true; });
        release.count_down();
        thread->join();
        REQUIRE(signalledBeforeReturn);
        REQUIRE(observed);
        REQUIRE(drained);
        REQUIRE_FALSE(rejectedRan);
    }

    void testOverflowQuitSignalsTokenBeforeDispatch() {
        const auto thread = UnmanagedEventThread::create(
            EventLoopOptions{}.setMaximumQueuedEvents(1).setQueueOverflowAction(EventQueueOverflowAction::Quit));
        const auto token = thread->stopToken();
        auto observed = false;
        thread->loop()->invoke([token, &observed]() -> void { observed = token.isStopRequested(); });
        thread->loop()->invoke([]() -> void {});
        REQUIRE(token.isStopRequested());
        thread->start();
        thread->join();
        REQUIRE(observed);
    }

    void testPostedQuitSignalsBeforeDispatch() {
        const auto thread = UnmanagedEventThread::create();
        const auto token = thread->stopToken();
        thread->loop()->post(Event{id::QuitEvent});
        REQUIRE(token.isStopRequested());
        REQUIRE_FALSE(thread->isStarted());
    }

    void testErrorHandlerStopLeavesTokenUnset() {
        const auto thread = UnmanagedEventThread::create(EventLoopOptions{}.setErrorPolicy(EventLoopErrorPolicy::Quit));
        const auto token = thread->stopToken();
        thread->workerEventLoop().setErrorHandler(
            [](std::exception_ptr) -> EventLoopErrorAction { return EventLoopErrorAction::Stop; });
        thread->loop()->invoke([]() -> void { throw el::RuntimeError{"handled failure"}; });
        thread->start();
        thread->join();
        REQUIRE(thread->workerEventLoop().hasError());
        REQUIRE_FALSE(token.isStopRequested());
    }

    void testErrorPoliciesSignalOnlyForQuit() {
        for (
            const auto policy :
            {EventLoopErrorPolicy::Ignore, EventLoopErrorPolicy::Store, EventLoopErrorPolicy::Quit}) {
            const auto thread = UnmanagedEventThread::create(EventLoopOptions{}.setErrorPolicy(policy));
            const auto token = thread->stopToken();
            auto observed = false;
            thread->loop()->invoke([]() -> void { throw el::RuntimeError{"callback failed"}; });
            thread->loop()->invoke([token, &observed, &thread]() -> void {
                observed = token.isStopRequested();
                thread->quit();
            });
            thread->start();
            thread->join();
            REQUIRE_EQUAL(observed, policy == EventLoopErrorPolicy::Quit);
            REQUIRE(token.isStopRequested());
        }
    }

    void testResumableStopDoesNotSignalToken() {
        const auto thread = UnmanagedEventThread::create();
        const auto token = thread->stopToken();
        thread->loop()->invoke([&thread]() -> void { thread->workerEventLoop().stop(); });
        thread->start();
        thread->join();
        REQUIRE_FALSE(token.isStopRequested());
        auto resumed = false;
        thread->loop()->invoke([&resumed]() -> void { resumed = true; });
        (void)thread->workerEventLoop().runUntilIdle();
        REQUIRE(resumed);
        REQUIRE_FALSE(token.isStopRequested());
        thread->quit();
        REQUIRE(token.isStopRequested());
    }

    void testDestructorQuitsAndJoins() {
        auto called = std::atomic<bool>{false};

        {
            const auto thread = UnmanagedEventThread::create();
            thread->start();
            thread->loop()->invoke([&called]() -> void { called = true; });
        }

        REQUIRE(called.load());
    }

    void testCurrentLoopInUnmanagedThread() {
        const auto thread = UnmanagedEventThread::create();
        auto matched = std::atomic<bool>{false};

        thread->start();
        thread->loop()->invoke([thread, &matched]() -> void {
            matched = currentLoop() == thread->loop();
            thread->quit();
        });
        thread->join();

        REQUIRE(matched.load());
    }
};
