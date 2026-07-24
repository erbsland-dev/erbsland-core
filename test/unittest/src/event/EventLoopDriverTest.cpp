// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/EventLoopDriver.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

using namespace el::event;
using namespace el::time;
using namespace std::chrono_literals;

TESTED_TARGETS(EventLoopDriver)
class EventLoopDriverTest final : public el::UnitTest {
    class Driver final : public EventLoopDriver {
    public:
        void wait() override {
            std::unique_lock lock{_mutex};
            infiniteWaits += 1;
            _condition.wait(lock, [this]() -> bool { return _wakePending; });
            _wakePending = false;
        }
        void wait(const TimeDelta maximumWait) override {
            std::unique_lock lock{_mutex};
            timedWaits += 1;
            lastWait = maximumWait;
            _condition.wait_for(lock, maximumWait.toStdNanoseconds(), [this]() -> bool { return _wakePending; });
            _wakePending = false;
        }
        void wake() noexcept override {
            {
                std::scoped_lock lock{_mutex};
                _wakePending = true;
                wakes += 1;
            }
            _condition.notify_one();
        }

        std::atomic<int> infiniteWaits{};
        std::atomic<int> timedWaits{};
        std::atomic<int> wakes{};
        TimeDelta lastWait;

    private:
        std::mutex _mutex;
        std::condition_variable _condition;
        bool _wakePending{false};
    };

public:
    void testDriverSupportsZeroTimedAndInfiniteWaits() {
        const auto driver = std::make_shared<Driver>();

        driver->wait(TimeDelta::zero());
        REQUIRE_EQUAL(driver->timedWaits.load(), 1);
        REQUIRE(driver->lastWait.isZero());

        driver->wait(TimeDelta{Milliseconds{1}});
        REQUIRE_EQUAL(driver->timedWaits.load(), 2);

        auto finished = std::atomic<bool>{false};
        auto worker = std::thread{[driver, &finished]() -> void {
            driver->wait();
            finished = true;
        }};
        std::this_thread::sleep_for(5ms);
        driver->wake();
        worker.join();
        REQUIRE(finished.load());
        REQUIRE_EQUAL(driver->infiniteWaits.load(), 1);
    }

    void testInjectedDriverControlsLoopWakePath() {
        const auto driver = std::make_shared<Driver>();
        const auto loop = EventLoop::create(driver);
        REQUIRE_FALSE(loop->runOnce(TimeDelta::zero()));
        REQUIRE_EQUAL(driver->timedWaits.load(), 1);
        REQUIRE(driver->lastWait.isZero());
        auto called = false;
        auto worker = std::thread{[loop, &called]() -> void {
            std::this_thread::sleep_for(5ms);
            loop->invoke([&called]() -> void { called = true; });
        }};

        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        worker.join();
        REQUIRE(called);
        REQUIRE(driver->wakes.load() >= 1);
        REQUIRE(driver->timedWaits.load() >= 1);
    }
};
