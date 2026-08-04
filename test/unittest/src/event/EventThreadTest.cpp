// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/event/CurrentEvents.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/Events.hpp>
#include <erbsland/event/UnmanagedEventThread.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <thread>

using namespace el::event;

TESTED_TARGETS(UnmanagedEventThread EventLoop Events)
class EventThreadTest final : public el::UnitTest {
public:
    void testCurrentEventsOutsideLoopThrows() { REQUIRE_THROWS_AS(el::err::LogicError, currentEvents()); }

    void testStartInvokeAndJoin() {
        const auto thread = UnmanagedEventThread::create();
        auto called = std::atomic<bool>{false};

        thread->start();
        thread->events()->invoke([thread, &called]() -> void {
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
        thread->events()->invoke([thread, &callbackThreadId]() -> void {
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

        REQUIRE_THROWS_AS(el::err::LogicError, thread->start());
    }

    void testQuitBeforeStart() {
        const auto thread = UnmanagedEventThread::create();

        thread->quit();
        REQUIRE(thread->eventLoop().isQuitRequested());
        thread->start();
        thread->join();

        REQUIRE(thread->isStarted());
        REQUIRE_FALSE(thread->isRunning());
    }

    void testDestructorQuitsAndJoins() {
        auto called = std::atomic<bool>{false};

        {
            const auto thread = UnmanagedEventThread::create();
            thread->start();
            thread->events()->invoke([&called]() -> void { called = true; });
        }

        REQUIRE(called.load());
    }

    void testCurrentEventsInUnmanagedThread() {
        const auto thread = UnmanagedEventThread::create();
        auto matched = std::atomic<bool>{false};

        thread->start();
        thread->events()->invoke([thread, &matched]() -> void {
            matched = currentEvents() == thread->events();
            thread->quit();
        });
        thread->join();

        REQUIRE(matched.load());
    }
};
