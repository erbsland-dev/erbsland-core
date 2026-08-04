// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/event/CurrentEvents.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/Events.hpp>
#include <erbsland/event/ManagedEventThread.hpp>
#include <erbsland/event/UnmanagedEventThread.hpp>
#include <erbsland/stream/AnyStringBuilderStream.hpp>
#include <erbsland/stream/StandardStreams.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unit/ExitCode.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <stdexcept>

using el::core::Application;
using el::event::ManagedEventThreadPtr;
using el::event::UnmanagedEventThread;
using el::unit::ExitCode;
using namespace el::text::literals;

TESTED_TARGETS(Application EventLoop ManagedEventThread UnmanagedEventThread)
class ApplicationEventTest final : public el::UnitTest {
public:
    void testQuitWithExitCode() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();

        app.events()->invoke([&app]() -> void { app.quit(ExitCode{17}); });

        REQUIRE_EQUAL(app.run(), 17);
    }

    void testFirstQuitExitCodeWins() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();

        app.events()->invoke([&app]() -> void {
            app.quit(ExitCode{17});
            app.quit(ExitCode{23});
        });

        REQUIRE_EQUAL(app.run(), 17);
    }

    void testMainLoopErbslandExceptionIsReported() {
        const auto errorOutput = el::stream::AnyStringBuilderStream::create();
        auto errorRedirect = el::stream::redirectStdErr(errorOutput);
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();

        app.events()->invoke([]() -> void { throw el::err::RuntimeError{"event callback failed"_el}; });

        REQUIRE_EQUAL(app.run(), ExitCode::failure().toRawValue());
        const auto text = el::text::StringConverter{errorOutput->toU8String()}.toStdString();
        const auto errorPosition = text.find("event callback failed");
        REQUIRE_NOT_EQUAL(errorPosition, std::string::npos);
    }

    void testMainLoopApplicationErrorControlsExitCode() {
        const auto errorOutput = el::stream::AnyStringBuilderStream::create();
        auto errorRedirect = el::stream::redirectStdErr(errorOutput);
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();

        app.events()->invoke(
            []() -> void { throw el::core::ApplicationError{"event application failure"_el, ExitCode{17}}; });

        REQUIRE_EQUAL(app.run(), 17);
        const auto text = el::text::StringConverter{errorOutput->toU8String()}.toStdString();
        const auto errorPosition = text.find("event application failure");
        REQUIRE_NOT_EQUAL(errorPosition, std::string::npos);
    }

    void testMainLoopForeignExceptionEscapesRun() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();

        app.events()->invoke([]() -> void { throw std::runtime_error{"boom"}; });

        REQUIRE_THROWS_AS(std::runtime_error, app.run());
    }

    void testManagedThreadQuitsWithApplication() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto thread = ManagedEventThreadPtr{app.createEventThread()};
        auto workerCalled = std::atomic<bool>{false};

        thread->start();
        thread->events()->invoke([&workerCalled]() -> void { workerCalled = true; });
        app.events()->invoke([&app]() -> void { app.quit(); });

        REQUIRE_EQUAL(app.run(), ExitCode::success().toRawValue());
        REQUIRE(workerCalled.load());
        REQUIRE(thread->eventLoop().isQuitRequested());
        REQUIRE_FALSE(thread->isRunning());
    }

    void testCurrentEventsInMainLoop() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        auto matched = false;
        const auto expectedEvents = app.events();

        app.events()->invoke([&app, &matched, expectedEvents]() -> void {
            matched = el::event::currentEvents() == expectedEvents;
            app.quit();
        });

        REQUIRE_EQUAL(app.run(), ExitCode::success().toRawValue());
        REQUIRE(matched);
    }

    void testCurrentEventsInManagedThread() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto thread = app.createEventThread();
        auto matched = std::atomic<bool>{false};

        thread->start();
        thread->events()->invoke([&app, thread, &matched]() -> void {
            matched = el::event::currentEvents() == thread->events();
            app.quit();
        });

        REQUIRE_EQUAL(app.run(), ExitCode::success().toRawValue());
        REQUIRE(matched.load());
    }

    void testUnmanagedThreadIsNotQuitByApplication() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto thread = UnmanagedEventThread::create();

        thread->start();
        app.quit();

        REQUIRE_FALSE(thread->eventLoop().isQuitRequested());
        thread->quit();
        thread->join();
    }
};
