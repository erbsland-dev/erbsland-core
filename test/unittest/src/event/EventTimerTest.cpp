// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/EventTimer.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/time/TimeUnitTags.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <thread>

using namespace el::event;
using namespace el::time;
using namespace std::chrono_literals;

TESTED_TARGETS(EventTimer EventTimerMode)
class EventTimerTest final : public el::UnitTest {
public:
    void testCreateTimerIsInactive() {
        const auto loop = EventLoop::create();
        const auto timer = loop->createTimer([]() -> void {});

        REQUIRE_FALSE(timer->isActive());
        REQUIRE_EQUAL(timer->mode(), EventTimerMode::Inactive);
    }

    void testStartOnceFiresOnce() {
        const auto loop = EventLoop::create();
        auto count = 0;

        const auto timer = loop->createTimer([&count]() -> void { count += 1; });
        timer->startOnce(TimeDelta{Milliseconds{5}});

        REQUIRE(timer->isActive());
        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE_EQUAL(count, 1);
        REQUIRE_FALSE(timer->isActive());
        REQUIRE_FALSE(loop->runOnce(TimeDelta{Milliseconds{20}}));
        REQUIRE_EQUAL(count, 1);
    }

    void testFixedDelayRepeatsAndStops() {
        const auto loop = EventLoop::create();
        auto count = 0;
        auto timer = EventTimerPtr{};
        timer = loop->createTimer([&count, &timer]() -> void {
            count += 1;
            if (count == 3) {
                timer->stop();
            }
        });
        timer->startFixedDelay(TimeDelta{Milliseconds{5}});

        while (count < 3) {
            REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        }
        REQUIRE_EQUAL(count, 3);
        REQUIRE_FALSE(timer->isActive());
        REQUIRE_FALSE(loop->runOnce(TimeDelta{Milliseconds{20}}));
        REQUIRE_EQUAL(count, 3);
    }

    void testFixedRateDoesNotQueueCatchUpBursts() {
        const auto loop = EventLoop::create();
        auto count = 0;
        const auto timer = loop->createTimer([&count]() -> void {
            count += 1;
            std::this_thread::sleep_for(20ms);
        });
        timer->startFixedRate(TimeDelta{Milliseconds{5}});

        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE_EQUAL(count, 1);
        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE_EQUAL(count, 2);
        timer->stop();
    }

    void testStopPreventsFutureCallbacks() {
        const auto loop = EventLoop::create();
        auto count = 0;
        const auto timer = loop->createTimer([&count]() -> void { count += 1; });
        timer->startFixedDelay(TimeDelta{Milliseconds{5}});

        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE_EQUAL(count, 1);
        timer->stop();
        REQUIRE_FALSE(loop->runOnce(TimeDelta{Milliseconds{20}}));
        REQUIRE_EQUAL(count, 1);
    }

    void testRestartReplacesModeAndInterval() {
        const auto loop = EventLoop::create();
        auto count = 0;
        const auto timer = loop->createTimer([&count]() -> void { count += 1; });

        timer->startFixedDelay(TimeDelta{Milliseconds{50}});
        timer->startOnce(TimeDelta::zero());

        REQUIRE_EQUAL(timer->mode(), EventTimerMode::Once);
        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE_EQUAL(count, 1);
        REQUIRE_FALSE(timer->isActive());
        REQUIRE_FALSE(loop->runOnce(TimeDelta{Milliseconds{70}}));
        REQUIRE_EQUAL(count, 1);
    }

    void testExpiredTimerPointerStopsCallbacks() {
        const auto loop = EventLoop::create();
        auto count = 0;

        {
            const auto timer = loop->createTimer([&count]() -> void { count += 1; });
            timer->startOnce(TimeDelta{Milliseconds{5}});
        }

        REQUIRE_FALSE(loop->runOnce(TimeDelta{Milliseconds{20}}));
        REQUIRE_EQUAL(count, 0);
    }

    void testInvalidRepeatedIntervalThrows() {
        const auto loop = EventLoop::create();
        const auto timer = loop->createTimer([]() -> void {});

        REQUIRE_THROWS_AS(el::err::ParameterError, timer->startFixedDelay(TimeDelta::zero()));
        REQUIRE_THROWS_AS(el::err::ParameterError, timer->startFixedRate(TimeDelta{Milliseconds{-1}}));
    }

    void testTimerExceptionIsCapturedAndTimerRepeats() {
        const auto loop = EventLoop::create();
        auto count = 0;
        auto timer = EventTimerPtr{};
        timer = loop->createTimer([&count, &timer]() -> void {
            count += 1;
            if (count == 1) {
                throw std::runtime_error{"timer"};
            }
            timer->stop();
        });
        timer->startFixedDelay(TimeDelta{Milliseconds{5}});

        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE(loop->hasError());
        REQUIRE(loop->takeError() != nullptr);
        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE_EQUAL(count, 2);
        REQUIRE_FALSE(timer->isActive());
    }
};
