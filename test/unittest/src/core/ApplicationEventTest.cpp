// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/AnyStringBuilderStream.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/CurrentLoop.hpp>
#include <erbsland/ElapsedTimer.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/EventQueueFullError.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ManagedEventThread.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/TimeAmounts.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/UnmanagedEventThread.hpp>

#include <atomic>
#include <future>
#include <latch>
#include <stdexcept>
#include <thread>

using el::core::Application;
using el::event::ManagedEventThreadPtr;
using el::event::UnmanagedEventThread;
using el::unit::ExitCode;
using namespace el::text::literals;

class ManagedFailureCleanupApplication final : public Application {
public:
    using Application::mainEventLoop;
    bool cleanupCalled{false};

protected:
    void cleanup() noexcept override { cleanupCalled = true; }
};

TESTED_TARGETS(Application EventLoop ManagedEventThread UnmanagedEventThread)
class ApplicationEventTest final : public el::UnitTest {
public:
    void testQuitWithExitCode() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();

        app.loop()->invoke([&app]() -> void { app.quit(ExitCode{17}); });

        REQUIRE_EQUAL(app.run(), 17);
    }

    void testFirstQuitExitCodeWins() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();

        app.loop()->invoke([&app]() -> void {
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

        app.loop()->invoke([]() -> void { throw el::err::RuntimeError{"event callback failed"_el}; });

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

        app.loop()->invoke(
            []() -> void { throw el::core::ApplicationError{"event application failure"_el, ExitCode{17}}; });

        REQUIRE_EQUAL(app.run(), 17);
        const auto text = el::text::StringConverter{errorOutput->toU8String()}.toStdString();
        const auto errorPosition = text.find("event application failure");
        REQUIRE_NOT_EQUAL(errorPosition, std::string::npos);
    }

    void testMainLoopForeignExceptionEscapesRun() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();

        app.loop()->invoke([]() -> void { throw std::runtime_error{"boom"}; });

        REQUIRE_THROWS_AS(std::runtime_error, app.run());
    }

    void testManagedThreadQuitsWithApplication() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto thread = ManagedEventThreadPtr{app.createEventThread()};
        auto workerCalled = std::atomic<bool>{false};

        thread->start();
        thread->loop()->invoke([&workerCalled]() -> void { workerCalled = true; });
        app.loop()->invoke([&app]() -> void { app.quit(); });

        REQUIRE_EQUAL(app.run(), ExitCode::success().toRawValue());
        REQUIRE(workerCalled.load());
        REQUIRE(thread->workerEventLoop().isQuitRequested());
        REQUIRE(thread->stopToken().isStopRequested());
        REQUIRE_FALSE(thread->isRunning());
    }

    void testManagedCancellationReachesExecutingCallback() {
        for (const auto failMain : {false, true}) {
            const auto errorOutput = el::stream::AnyStringBuilderStream::create();
            auto errorRedirect = el::stream::redirectStdErr(errorOutput);
            auto scope = ApplicationTestScope<ManagedFailureCleanupApplication>{};
            auto &app = scope.app();
            const auto worker = app.createEventThread();
            const auto token = worker->stopToken();
            auto entered = std::latch{1};
            auto observed = false;
            worker->loop()->invoke([token, &entered, &observed]() -> void {
                entered.count_down();
                const auto timeout = el::ElapsedTimer{};
                // The latch establishes order; the deadline only prevents a regression from hanging the suite.
                while (!token.isStopRequested() && timeout.elapsed() < el::Seconds{2}) {
                    std::this_thread::yield();
                }
                observed = token.isStopRequested();
            });
            worker->start();
            entered.wait();
            app.loop()->invoke([&app, failMain]() -> void {
                if (failMain) {
                    throw el::RuntimeError{"main callback failed"};
                }
                app.quit();
            });
            const auto exitCode = app.run();
            REQUIRE_EQUAL(exitCode != 0, failMain);
            REQUIRE(observed);
            REQUIRE(app.cleanupCalled);
            REQUIRE_FALSE(worker->isRunning());
        }
    }

    void testCurrentLoopInMainLoop() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        auto matched = false;
        const auto expectedTarget = app.loop();

        app.loop()->invoke([&app, &matched, expectedTarget]() -> void {
            matched = el::event::currentLoop() == expectedTarget;
            app.quit();
        });

        REQUIRE_EQUAL(app.run(), ExitCode::success().toRawValue());
        REQUIRE(matched);
    }

    void testCurrentLoopInManagedThread() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto thread = app.createEventThread();
        auto matched = std::atomic<bool>{false};

        thread->start();
        thread->loop()->invoke([&app, thread, &matched]() -> void {
            matched = el::event::currentLoop() == thread->loop();
            app.quit();
        });

        REQUIRE_EQUAL(app.run(), ExitCode::success().toRawValue());
        REQUIRE(matched.load());
    }

    void testManagedFailureBeforeMainLoopStartup() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto worker = app.createEventThread(
            el::event::EventLoopOptions{}.setErrorPolicy(el::event::EventLoopErrorPolicy::FailApplication));
        worker->loop()->invoke([]() -> void { throw el::err::RuntimeError{"worker failure"}; });
        worker->start();
        worker->join();
        auto dispatched = false;
        app.loop()->invoke([&]() -> void {
            dispatched = true;
            app.quit();
        });
        REQUIRE_EQUAL(app.run(), ExitCode::failure().toRawValue());
        REQUIRE_FALSE(dispatched);
        REQUIRE_FALSE(worker->isRunning());
        REQUIRE(worker->workerEventLoop().hasError());
        REQUIRE(worker->stopToken().isStopRequested());
    }

    void testManagedFailureWakesRunningMainLoop() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto worker = app.createEventThread(
            el::event::EventLoopOptions{}.setErrorPolicy(el::event::EventLoopErrorPolicy::FailApplication));
        app.loop()->invoke([worker]() -> void {
            worker->loop()->invoke([]() -> void { throw el::err::RuntimeError{"worker failure"}; });
            worker->start();
        });
        REQUIRE_EQUAL(app.run(), ExitCode::failure().toRawValue());
        REQUIRE_FALSE(worker->isRunning());
    }

    void testManagedFailureDuringWorkerJoinsRunsCleanup() {
        auto scope = ApplicationTestScope<ManagedFailureCleanupApplication>{};
        auto &app = scope.app();
        const auto worker = app.createEventThread(
            el::event::EventLoopOptions{}.setErrorPolicy(el::event::EventLoopErrorPolicy::FailApplication));
        auto started = std::promise<void>{};
        worker->loop()->invoke([worker, &started]() -> void {
            started.set_value();
            while (!worker->workerEventLoop().isQuitRequested()) {
                std::this_thread::yield();
            }
            throw el::err::RuntimeError{"failure during join"};
        });
        worker->start();
        started.get_future().wait();
        // Stop only the main loop. finishRun() requests worker quit after snapshotting its thread list.
        app.loop()->invoke([&app]() -> void { app.mainEventLoop().stop(); });
        REQUIRE_EQUAL(app.run(), ExitCode::failure().toRawValue());
        REQUIRE(app.cleanupCalled);
        REQUIRE_FALSE(worker->isRunning());
    }

    void testProceduralMainForwardsManagedFailureAfterCleanup() {
        auto scope = ApplicationTestScope<ManagedFailureCleanupApplication>{};
        auto &app = scope.app();
        const auto worker = app.createEventThread(
            el::event::EventLoopOptions{}.setErrorPolicy(el::event::EventLoopErrorPolicy::FailApplication));
        worker->loop()->invoke([]() -> void { throw el::err::RuntimeError{"procedural worker failure"}; });
        app.setMainFn([worker]() -> ExitCode {
            worker->start();
            worker->join();
            return ExitCode::success();
        });
        REQUIRE_EQUAL(app.run(), ExitCode::failure().toRawValue());
        REQUIRE(app.cleanupCalled);
        REQUIRE_FALSE(worker->isRunning());
    }

    void testManagedFailureQuitsReceiverDuringProceduralMain() {
        auto scope = ApplicationTestScope<ManagedFailureCleanupApplication>{};
        auto &app = scope.app();
        const auto receiver = app.createEventThread();
        const auto worker = app.createEventThread(
            el::event::EventLoopOptions{}.setErrorPolicy(el::event::EventLoopErrorPolicy::FailApplication));
        auto receiverWasQuit = false;
        worker->loop()->invoke([]() -> void { throw el::err::RuntimeError{"worker failure with waiting receiver"}; });
        app.setMainFn([receiver, worker, &receiverWasQuit]() -> ExitCode {
            receiver->start();
            worker->start();
            worker->join();
            receiverWasQuit = receiver->workerEventLoop().isQuitRequested();
            // Ensure a regression fails the assertion rather than hanging in join().
            receiver->quit();
            receiver->join();
            return ExitCode::success();
        });

        REQUIRE_EQUAL(app.run(), ExitCode::failure().toRawValue());
        REQUIRE(receiverWasQuit);
        REQUIRE(receiver->stopToken().isStopRequested());
        REQUIRE(worker->stopToken().isStopRequested());
        REQUIRE(app.cleanupCalled);
        REQUIRE_FALSE(receiver->isRunning());
        REQUIRE_FALSE(worker->isRunning());
    }

    void testManagedOverflowFailsApplicationAndKeepsFirstFailure() {
        const auto errorOutput = el::stream::AnyStringBuilderStream::create();
        auto errorRedirect = el::stream::redirectStdErr(errorOutput);
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto worker = app.createEventThread(
            el::event::EventLoopOptions{}
                .setMaximumQueuedEvents(1)
                .setQueueOverflowAction(el::event::EventQueueOverflowAction::FailApplication)
                .setErrorPolicy(el::event::EventLoopErrorPolicy::FailApplication));
        worker->loop()->invoke([]() -> void { throw el::core::ApplicationError{"later failure"_el, ExitCode{17}}; });
        worker->loop()->invoke([]() -> void {});
        REQUIRE(worker->workerEventLoop().isQuitRequested());
        REQUIRE(worker->stopToken().isStopRequested());
        REQUIRE_EQUAL(worker->workerEventLoop().queuedEventCount(), std::size_t{1});
        worker->start();
        worker->join();
        REQUIRE_EQUAL(app.run(), ExitCode::failure().toRawValue());
        const auto error = worker->workerEventLoop().takeError();
        REQUIRE(error);
        REQUIRE_THROWS_AS(el::event::EventQueueFullError, std::rethrow_exception(error));
    }

    void testManagedApplicationErrorKeepsExitCode() {
        const auto errorOutput = el::stream::AnyStringBuilderStream::create();
        auto errorRedirect = el::stream::redirectStdErr(errorOutput);
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto worker = app.createEventThread(
            el::event::EventLoopOptions{}.setErrorPolicy(el::event::EventLoopErrorPolicy::FailApplication));
        worker->loop()->invoke([]() -> void { throw el::core::ApplicationError{"worker"_el, ExitCode{17}}; });
        worker->start();
        worker->join();
        REQUIRE_EQUAL(app.run(), 17);
    }

    void testUnmanagedThreadIsNotQuitByApplication() {
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        const auto thread = UnmanagedEventThread::create();

        thread->start();
        app.quit();

        REQUIRE_FALSE(thread->workerEventLoop().isQuitRequested());
        REQUIRE_FALSE(thread->stopToken().isStopRequested());
        thread->quit();
        thread->join();
    }
};
