// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/event/EventBackend.hpp>
#include <erbsland/event/EventBackendTarget.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/EventRegistry.hpp>
#include <erbsland/event/EventScheduler.hpp>
#include <erbsland/event/EventTimer.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/time/TimeUnitTags.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <optional>
#include <vector>

using namespace el::event;
using namespace el::text::literals;
using namespace el::time;

TESTED_TARGETS(EventBackend EventLoop)
class EventBackendTest final : public el::UnitTest {
    class TestBackend final : public EventBackend {
    public:
        TestBackend(
            std::vector<int> &log, EventBackendId backendId, int identifier, std::optional<TimePoint> nextWakeTime) :
            _log{log}, _backendId{backendId}, _identifier{identifier}, _nextWakeTime{nextWakeTime} {}

    public: // implement EventBackend
        [[nodiscard]] auto backendId() const noexcept -> EventBackendId override { return _backendId; }
        void attach(EventBackendTargetWeakPtr target) override {
            attached = true;
            _target = std::move(target);
        }
        void wake() noexcept override { wakeCount += 1; }
        void poll(const TimePoint now) override {
            pollCount += 1;
            if (!_nextWakeTime.has_value() || now < *_nextWakeTime) {
                return;
            }
            _log.push_back(_identifier);
            _nextWakeTime = std::nullopt;
            if (const auto target = _target.lock(); target != nullptr) {
                target->postFromBackend(Event{id::TimerEvent});
            }
        }
        [[nodiscard]] auto handleEvent(const Event &event) -> bool override {
            if (event.identifier() != id::TimerEvent) {
                return false;
            }
            _log.push_back(_identifier * 100);
            return true;
        }
        [[nodiscard]] auto nextWakeTime() const -> std::optional<TimePoint> override { return _nextWakeTime; }

    public:
        bool attached{false};
        std::size_t pollCount{0};
        std::size_t wakeCount{0};
        void requestTargetWake() {
            if (const auto target = _target.lock(); target != nullptr) {
                target->wakeFromBackend();
            }
        }

    private:
        std::vector<int> &_log;
        EventBackendId _backendId;
        int _identifier{0};
        std::optional<TimePoint> _nextWakeTime;
        EventBackendTargetWeakPtr _target;
    };

    class DueAfterFirstPollBackend final : public EventBackend {
    public:
        DueAfterFirstPollBackend(std::vector<int> &log, EventBackendId backendId, int identifier) :
            _log{log}, _backendId{backendId}, _identifier{identifier} {}

    public: // implement EventBackend
        [[nodiscard]] auto backendId() const noexcept -> EventBackendId override { return _backendId; }
        void attach(EventBackendTargetWeakPtr target) override { _target = std::move(target); }
        void wake() noexcept override {}
        void poll(const TimePoint now) override {
            pollCount += 1;
            if (!_armed) {
                _armed = true;
                _nextWakeTime = now;
                return;
            }
            if (!_nextWakeTime.has_value() || now < *_nextWakeTime) {
                return;
            }
            _log.push_back(_identifier);
            _nextWakeTime = std::nullopt;
            if (const auto target = _target.lock(); target != nullptr) {
                target->postFromBackend(Event{id::TimerEvent});
            }
        }
        [[nodiscard]] auto handleEvent(const Event &event) -> bool override {
            if (event.identifier() != id::TimerEvent) {
                return false;
            }
            _log.push_back(_identifier * 100);
            return true;
        }
        [[nodiscard]] auto nextWakeTime() const -> std::optional<TimePoint> override { return _nextWakeTime; }

    public:
        std::size_t pollCount{0};

    private:
        std::vector<int> &_log;
        EventBackendId _backendId;
        int _identifier{0};
        std::optional<TimePoint> _nextWakeTime;
        EventBackendTargetWeakPtr _target;
        bool _armed{false};
    };

public:
    void testBackendAttachPollAndPost() {
        const auto loop = EventLoop::create();
        auto log = std::vector<int>{};
        auto backend = std::make_unique<TestBackend>(log, id::SchedulerBackend, 1, TimePoint::now());
        const auto backendPtr = backend.get();

        loop->registerBackend(std::move(backend));
        REQUIRE(backendPtr->attached);
        REQUIRE(loop->runOnce(TimeDelta::zero()));
        REQUIRE_EQUAL(log, std::vector<int>({1, 100}));
        REQUIRE_GREATER_EQUAL(backendPtr->pollCount, std::size_t{1});
    }

    void testBackendPollingOrder() {
        ApplicationTestScope appScope;
        const auto backendIdA = appScope.app().eventRegistry().registerBackend("com.example.Backend.A"_el);
        const auto backendIdB = appScope.app().eventRegistry().registerBackend("com.example.Backend.B"_el);
        const auto loop = EventLoop::create();
        auto log = std::vector<int>{};

        loop->registerBackend(std::make_unique<TestBackend>(log, backendIdA, 1, TimePoint::now()));
        loop->registerBackend(std::make_unique<TestBackend>(log, backendIdB, 2, TimePoint::now()));

        REQUIRE(loop->runOnce(TimeDelta::zero()));
        REQUIRE_GREATER_EQUAL(log.size(), std::size_t{2});
        REQUIRE_EQUAL(log[0], 1);
        REQUIRE_EQUAL(log[1], 2);
    }

    void testNextWakeTimeParticipatesInLoopWait() {
        ApplicationTestScope appScope;
        const auto backendId = appScope.app().eventRegistry().registerBackend("com.example.Backend.Wait"_el);
        const auto loop = EventLoop::create();
        auto log = std::vector<int>{};

        loop->registerBackend(
            std::make_unique<TestBackend>(log, backendId, 3, TimePoint::inFuture(TimeDelta{Milliseconds{10}})));

        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE_EQUAL(log, std::vector<int>({3, 300}));
    }

    void testDueBackendWakeTimeRepollsInsteadOfEndingRunOnce() {
        ApplicationTestScope appScope;
        const auto backendId = appScope.app().eventRegistry().registerBackend("com.example.Backend.DueAfterPoll"_el);
        const auto loop = EventLoop::create();
        auto log = std::vector<int>{};
        auto backend = std::make_unique<DueAfterFirstPollBackend>(log, backendId, 9);
        const auto backendPtr = backend.get();

        loop->registerBackend(std::move(backend));

        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE_GREATER_EQUAL(backendPtr->pollCount, std::size_t{2});
        REQUIRE_EQUAL(log, std::vector<int>({9, 900}));
    }

    void testWakeFromBackendWakesRegisteredBackends() {
        const auto loop = EventLoop::create();
        auto log = std::vector<int>{};
        auto backend = std::make_unique<TestBackend>(log, id::SchedulerBackend, 3, std::nullopt);
        const auto backendPtr = backend.get();

        loop->registerBackend(std::move(backend));
        backendPtr->requestTargetWake();

        REQUIRE_GREATER_EQUAL(backendPtr->wakeCount, std::size_t{1});
    }

    void testHandleEventDispatch() {
        const auto loop = EventLoop::create();
        auto log = std::vector<int>{};

        loop->registerBackend(std::make_unique<TestBackend>(log, id::SchedulerBackend, 4, std::nullopt));
        loop->post(Event{id::TimerEvent});

        REQUIRE(loop->runOnce(TimeDelta::zero()));
        REQUIRE_EQUAL(log, std::vector<int>({400}));
    }

    void testCreateDoesNotRegisterSchedulerBackend() {
        const auto loop = EventLoop::create();
        auto log = std::vector<int>{};

        loop->registerBackend(std::make_unique<TestBackend>(log, id::SchedulerBackend, 5, std::nullopt));

        loop->post(Event{id::TimerEvent});
        REQUIRE(loop->runOnce(TimeDelta::zero()));
        REQUIRE_EQUAL(log, std::vector<int>({500}));
    }

    void testRejectNullBackend() {
        const auto loop = EventLoop::create();

        REQUIRE_THROWS_AS(el::err::ParameterError, loop->registerBackend(nullptr));
    }

    void testRejectNoBackendIdentifier() {
        const auto loop = EventLoop::create();
        auto log = std::vector<int>{};

        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            loop->registerBackend(std::make_unique<TestBackend>(log, id::NoBackend, 6, std::nullopt)));
    }

    void testRejectDuplicateBackendIdentifier() {
        const auto loop = EventLoop::create();
        auto log = std::vector<int>{};

        loop->registerBackend(std::make_unique<TestBackend>(log, id::SchedulerBackend, 7, std::nullopt));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            loop->registerBackend(std::make_unique<TestBackend>(log, id::SchedulerBackend, 8, std::nullopt)));
    }

    void testSchedulerFrontendAutoRegistersBackend() {
        const auto loop = EventLoop::create();

        auto &scheduler = loop->get<EventScheduler>();
        const auto timer = scheduler.createTimer([]() -> void {});

        REQUIRE(timer != nullptr);
        REQUIRE_FALSE(timer->isActive());
    }
};
