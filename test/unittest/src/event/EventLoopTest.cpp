// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CurrentEvents.hpp>
#include <erbsland/EventBackend.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/TimeUnitTags.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

using namespace el::event;
using namespace el::time;
using namespace std::chrono_literals;

TESTED_TARGETS(EventLoop Events EventCallback)
class EventLoopTest final : public el::UnitTest {
    class ThrowingBackend final : public EventBackend {
    public: // implement EventBackend
        [[nodiscard]] auto backendId() const noexcept -> EventBackendId override { return id::SchedulerBackend; }
        void attach(EventBackendTargetWeakPtr target, [[maybe_unused]] EventLoopDriverWeakPtr driver) override {
            _target = std::move(target);
        }
        void poll([[maybe_unused]] TimePoint now) override {}
        [[nodiscard]] auto handleEvent(const Event &event) -> bool override {
            if (event.identifier() != id::TimerEvent) {
                return false;
            }
            throw el::err::RuntimeError{"backend"};
        }
        [[nodiscard]] auto nextWakeTime() const -> std::optional<TimePoint> override { return std::nullopt; }

    private:
        EventBackendTargetWeakPtr _target;
    };

    class PollFailureBackend final : public EventBackend {
    public:
        void setFailPolling(bool value) noexcept { _failPolling = value; }

    public: // implement EventBackend
        [[nodiscard]] auto backendId() const noexcept -> EventBackendId override { return id::SchedulerBackend; }
        void attach(
            [[maybe_unused]] EventBackendTargetWeakPtr target,
            [[maybe_unused]] EventLoopDriverWeakPtr driver) override {}
        void poll([[maybe_unused]] TimePoint now) override {
            if (_failPolling) {
                throw el::err::RuntimeError{"poll failure"};
            }
        }
        [[nodiscard]] auto handleEvent([[maybe_unused]] const Event &event) -> bool override { return false; }
        [[nodiscard]] auto nextWakeTime() const -> std::optional<TimePoint> override { return std::nullopt; }

    private:
        bool _failPolling{true};
    };

    void runVariant(const EventLoopPtr &loop, int variant) {
        switch (variant) {
        case 0:
            loop->run();
            break;
        case 1:
            static_cast<void>(loop->runOnce());
            break;
        case 2:
            static_cast<void>(loop->runOnce(TimeDelta::zero()));
            break;
        case 3:
            static_cast<void>(loop->runUntilIdle());
            break;
        }
    }

public:
    void testInvokeCallbacksRunInFifoOrder() {
        const auto loop = EventLoop::create();
        auto values = std::vector<int>{};

        loop->invoke([&values]() -> void { values.push_back(1); });
        loop->invoke([&values]() -> void { values.push_back(2); });
        loop->invoke([&values]() -> void { values.push_back(3); });

        REQUIRE_EQUAL(loop->runUntilIdle(), std::size_t{3});
        REQUIRE_EQUAL(values, std::vector<int>({1, 2, 3}));
    }

    void testInvokeFromThreadWakesLoop() {
        const auto loop = EventLoop::create();
        auto called = std::atomic<bool>{false};

        auto worker = std::thread{[loop, &called]() -> void {
            std::this_thread::sleep_for(20ms);
            loop->invoke([&called]() -> void { called = true; });
        }};

        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        worker.join();
        REQUIRE(called.load());
    }

    void testQuitEventStopsRun() {
        const auto loop = EventLoop::create();
        auto values = std::vector<int>{};

        loop->invoke([&values]() -> void { values.push_back(1); });
        loop->post(Event{id::QuitEvent});
        loop->invoke([&values]() -> void { values.push_back(2); });

        loop->run();
        REQUIRE_EQUAL(values, std::vector<int>({1}));
        REQUIRE(loop->isQuitRequested());
    }

    void testLocalQuitDropsLaterInvocations() {
        const auto loop = EventLoop::create();
        auto called = false;

        loop->quit();
        loop->invoke([&called]() -> void { called = true; });

        REQUIRE_EQUAL(loop->runUntilIdle(), std::size_t{1});
        REQUIRE_FALSE(called);
        REQUIRE(loop->isQuitRequested());
    }

    void testQuitDrainsEventsBeforeQuitEvent() {
        const auto loop = EventLoop::create();
        auto values = std::vector<int>{};

        loop->invoke([&values]() -> void { values.push_back(1); });
        loop->quit();
        loop->invoke([&values]() -> void { values.push_back(2); });

        loop->run();
        REQUIRE_EQUAL(values, std::vector<int>({1}));
    }

    void testStopIsReusable() {
        const auto loop = EventLoop::create();
        auto called = false;

        loop->stop();
        loop->invoke([&called]() -> void { called = true; });

        REQUIRE_EQUAL(loop->runUntilIdle(), std::size_t{1});
        REQUIRE(called);
        REQUIRE_FALSE(loop->isQuitRequested());
    }

    void testCallbackExceptionIsCaptured() {
        const auto loop = EventLoop::create();

        loop->invoke([]() -> void { throw el::err::RuntimeError{"boom"}; });

        REQUIRE_EQUAL(loop->runUntilIdle(), std::size_t{1});
        REQUIRE(loop->hasError());
        const auto error = loop->takeError();
        REQUIRE(error);
        REQUIRE_FALSE(loop->hasError());

        auto caught = false;
        try {
            std::rethrow_exception(error);
        } catch (const el::err::RuntimeError &) {
            caught = true;
        }
        REQUIRE(caught);
    }

    void testDefaultErrorHandlingContinues() {
        const auto loop = EventLoop::create();
        auto calledAfterError = false;

        loop->invoke([]() -> void { throw el::err::RuntimeError{"boom"}; });
        loop->invoke([&calledAfterError]() -> void { calledAfterError = true; });

        REQUIRE_EQUAL(loop->runUntilIdle(), std::size_t{2});
        REQUIRE(calledAfterError);
        REQUIRE(loop->hasError());
    }

    void testErrorHandlerCanStopLoop() {
        const auto loop = EventLoop::create();
        auto handlerCalled = false;
        auto calledAfterError = false;

        loop->setErrorHandler([&handlerCalled](std::exception_ptr error) -> EventLoopErrorAction {
            handlerCalled = error != nullptr;
            return EventLoopErrorAction::Stop;
        });
        loop->invoke([]() -> void { throw el::err::RuntimeError{"boom"}; });
        loop->invoke([&calledAfterError]() -> void { calledAfterError = true; });

        REQUIRE(loop->runOnce(TimeDelta::zero()));
        REQUIRE(handlerCalled);
        REQUIRE_FALSE(calledAfterError);
        REQUIRE(loop->hasError());
    }

    void testThrowingErrorHandlerIsCapturedAndStopsLoop() {
        const auto loop = EventLoop::create();

        loop->setErrorHandler([]([[maybe_unused]] std::exception_ptr error) -> EventLoopErrorAction {
            throw el::err::RuntimeError{"handler"};
        });
        loop->invoke([]() -> void { throw el::err::RuntimeError{"boom"}; });

        REQUIRE_EQUAL(loop->runUntilIdle(), std::size_t{1});
        REQUIRE(loop->takeError());
        REQUIRE(loop->takeError());
        REQUIRE_FALSE(loop->hasError());
    }

    void testTimerCallbackExceptionUsesErrorHandler() {
        const auto loop = EventLoop::create();
        auto handlerCalled = false;

        loop->setErrorHandler([&handlerCalled](std::exception_ptr error) -> EventLoopErrorAction {
            handlerCalled = error != nullptr;
            return EventLoopErrorAction::Stop;
        });
        const auto timer = loop->createTimer([]() -> void { throw el::err::RuntimeError{"timer"}; });
        timer->startOnce(TimeDelta::zero());

        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE(timer);
        REQUIRE(handlerCalled);
        REQUIRE(loop->hasError());
    }

    void testInvokeAfterFiresWithoutHandle() {
        const auto loop = EventLoop::create();
        auto count = 0;

        loop->invokeAfter(TimeDelta{Milliseconds{5}}, [&count]() -> void { count += 1; });

        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE_EQUAL(count, 1);
        REQUIRE_FALSE(loop->runOnce(TimeDelta{Milliseconds{20}}));
        REQUIRE_EQUAL(count, 1);
    }

    void testInvokeAfterZeroKeepsInvocationOrder() {
        const auto loop = EventLoop::create();
        auto values = std::vector<int>{};

        loop->invoke([&values]() -> void { values.push_back(1); });
        loop->invokeAfter(TimeDelta::zero(), [&values]() -> void { values.push_back(2); });
        loop->invoke([&values]() -> void { values.push_back(3); });

        REQUIRE_EQUAL(loop->runUntilIdle(), std::size_t{3});
        REQUIRE_EQUAL(values, std::vector<int>({1, 2, 3}));
    }

    void testDelayedInvocationExceptionUsesErrorHandler() {
        const auto loop = EventLoop::create();
        auto handlerCalled = false;

        loop->setErrorHandler([&handlerCalled](std::exception_ptr error) -> EventLoopErrorAction {
            handlerCalled = error != nullptr;
            return EventLoopErrorAction::Stop;
        });
        loop->invokeAfter(TimeDelta{Milliseconds{5}}, []() -> void { throw el::err::RuntimeError{"delayed"}; });

        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE(handlerCalled);
        REQUIRE(loop->hasError());
    }

    void testBackendExceptionUsesErrorHandler() {
        const auto loop = EventLoop::create();
        auto handlerCalled = false;

        loop->registerBackend(std::make_unique<ThrowingBackend>());
        loop->setErrorHandler([&handlerCalled](std::exception_ptr error) -> EventLoopErrorAction {
            handlerCalled = error != nullptr;
            return EventLoopErrorAction::Stop;
        });
        loop->post(Event{id::TimerEvent});

        REQUIRE(loop->runOnce(TimeDelta::zero()));
        REQUIRE(handlerCalled);
        REQUIRE(loop->hasError());
    }

    void testUnhandledBackendEventIsAccepted() {
        const auto loop = EventLoop::create();

        loop->post(Event{id::TimerEvent});

        REQUIRE_EQUAL(loop->runUntilIdle(), std::size_t{1});
        REQUIRE_FALSE(loop->hasError());
    }

    void testSchedulerAutoRegistersWhileRunning() {
        const auto loop = EventLoop::create();
        auto count = 0;
        auto timer = EventTimerPtr{};

        loop->invoke([loop, &count, &timer]() -> void {
            timer = loop->createTimer([&count]() -> void { count += 1; });
            timer->startOnce(TimeDelta::zero());
        });

        REQUIRE(loop->runOnce(TimeDelta::zero()));
        REQUIRE_EQUAL(count, 0);
        REQUIRE(timer);
        REQUIRE(loop->runOnce(TimeDelta{Seconds{1}}));
        REQUIRE_EQUAL(count, 1);
    }

    void testRunOnceRejectsNestedRun() {
        const auto loop = EventLoop::create();
        auto rejected = false;

        loop->invoke([loop, &rejected]() -> void {
            try {
                static_cast<void>(loop->runOnce(TimeDelta::zero()));
            } catch (const el::err::LogicFailure &) {
                rejected = true;
            }
        });

        REQUIRE(loop->runOnce(TimeDelta::zero()));
        REQUIRE(rejected);
    }

    void testRunVariantsRestoreStateAfterException() {
        for (auto variant = 0; variant < 4; ++variant) {
            auto backend = std::make_unique<PollFailureBackend>();
            const auto backendPtr = backend.get();
            const auto loop = EventLoop::create(std::move(backend));
            REQUIRE_THROWS_AS(el::err::RuntimeError, runVariant(loop, variant));
            REQUIRE_FALSE(loop->isRunning());
            REQUIRE_THROWS_AS(el::err::LogicFailure, currentEvents());
            backendPtr->setFailPolling(false);
            loop->invoke([loop]() -> void { loop->stop(); });
            runVariant(loop, variant);
            REQUIRE_FALSE(loop->isRunning());
            REQUIRE_THROWS_AS(el::err::LogicFailure, currentEvents());
        }
    }

    void testNestedLoopFailureRestoresOuterEventContext() {
        const auto outer = EventLoop::create();
        const auto inner = EventLoop::create(std::make_unique<PollFailureBackend>());
        auto restored = false;
        outer->invoke([&]() -> void {
            try {
                static_cast<void>(inner->runOnce(TimeDelta::zero()));
            } catch (const el::err::RuntimeError &) {
                restored = currentEvents() == outer;
            }
        });
        REQUIRE_EQUAL(outer->runUntilIdle(), std::size_t{1});
        REQUIRE(restored);
        REQUIRE_FALSE(outer->hasError());
    }
};
