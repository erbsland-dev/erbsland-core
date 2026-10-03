// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../src/PiSession.hpp"

#include <erbsland/BindEvent.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/TimeAmounts.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <memory>
#include <stdexcept>

using namespace el::text::literals;

TESTED_TARGETS(PiSession)
class PiSessionTest final : public el::UnitTest {
    struct OptionalView {
        int calls{0};
        void updated() { ++calls; }
    };

    void wait(const el::EventLoopPtr &loop, const std::shared_ptr<demo::PiSession> &session) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{1};
        while (!session->isFinished() && std::chrono::steady_clock::now() < deadline) {
            (void)loop->runOnce(el::Milliseconds{10});
            if (const auto error = loop->takeError()) {
                std::rethrow_exception(error);
            }
        }
        REQUIRE(session->isFinished());
        (void)loop->runUntilIdle();
        REQUIRE_FALSE(loop->hasError());
    }

public:
    void testFiniteSessionRetainsDataAfterOptionalViewExpires() {
        const auto loop = el::EventLoop::create();
        const auto history = std::make_shared<demo::DigitHistory>();
        auto view = std::make_shared<OptionalView>();
        const auto weakView = std::weak_ptr<OptionalView>{view};
        auto completions = 0;
        auto error = std::exception_ptr{};
        const auto session = std::make_shared<demo::PiSession>(
            loop,
            history,
            el::bindWeakEvent(loop, weakView, &OptionalView::updated),
            [&](std::exception_ptr result) -> void {
                ++completions;
                error = result;
            });
        session->start(el::CpCount{100});
        view.reset();
        REQUIRE(weakView.expired());
        wait(loop, session);
        REQUIRE(error == nullptr);
        REQUIRE_EQUAL(completions, 1);
        REQUIRE_EQUAL(history->count(), el::CpCount{100});
        REQUIRE_EQUAL(
            history->range({el::CpIndex{}, el::CpCount{100}}),
            "1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679"_el);
    }
    void testZeroLimitAndOneShotLifecycle() {
        const auto loop = el::EventLoop::create();
        const auto history = std::make_shared<demo::DigitHistory>();
        auto completions = 0;
        const auto session = std::make_shared<demo::PiSession>(
            loop, history, demo::PiSession::OnUpdated{}, [&](std::exception_ptr error) -> void {
                REQUIRE(error == nullptr);
                ++completions;
            });
        session->start(el::CpCount{0});
        wait(loop, session);
        REQUIRE_EQUAL(history->count(), el::CpCount{});
        REQUIRE_EQUAL(completions, 1);
        REQUIRE_THROWS(session->start(el::CpCount{1}));
        session->requestStop();
        session->shutdown();
    }
    void testWorkerErrorHasRequiredCompletion() {
        const auto loop = el::EventLoop::create();
        auto error = std::exception_ptr{};
        const auto session = std::make_shared<demo::PiSession>(
            loop,
            std::make_shared<demo::DigitHistory>(),
            demo::PiSession::OnUpdated{},
            [&](std::exception_ptr result) -> void { error = result; },
            []() -> std::optional<el::Char> { throw el::RuntimeError{"arithmetic"}; });
        session->start(std::nullopt);
        wait(loop, session);
        REQUIRE(error != nullptr);
        REQUIRE_THROWS_AS(el::RuntimeError, std::rethrow_exception(error));
    }
    void testCancellationAndCleanupWithoutDispatch() {
        auto loop = el::EventLoop::create();
        const auto weakLoop = std::weak_ptr<el::EventLoop>{loop};
        auto completed = false;
        auto session = std::make_shared<demo::PiSession>(
            loop,
            std::make_shared<demo::DigitHistory>(),
            demo::PiSession::OnUpdated{},
            [&](std::exception_ptr) -> void { completed = true; });
        session->start(std::nullopt);
        session->shutdown();
        REQUIRE_FALSE(completed);
        session.reset();
        loop.reset();
        REQUIRE(weakLoop.expired());
    }
    void testStopKeepsOwnerLoopOpenUntilCompletion() {
        const auto loop = el::EventLoop::create();
        auto completed = false;
        const auto session = std::make_shared<demo::PiSession>(
            loop,
            std::make_shared<demo::DigitHistory>(),
            demo::PiSession::OnUpdated{},
            [&](std::exception_ptr error) -> void {
                REQUIRE(error == nullptr);
                completed = true;
            });
        session->start(std::nullopt);
        session->requestStop();
        session->requestStop();
        REQUIRE_FALSE(loop->isQuitRequested());
        wait(loop, session);
        REQUIRE(completed);
        REQUIRE(session->isStopping());
    }
};
