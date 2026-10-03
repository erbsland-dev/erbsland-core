// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/Subprocess.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/impl/worker_executor/WorkerServices.hpp>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <new>
#include <system_error>
#include <thread>

using namespace el::text::literals;
using namespace el::event;

TESTED_TARGETS(EventLoop WorkerServices WorkerExecutor)
class EventFatalFailureTest final : public el::UnitTest {
private:
    template <typename Failure>
    void verifyCallbackFailure(const Failure &failure) {
        for (
            const auto policy :
            {EventLoopErrorPolicy::Ignore, EventLoopErrorPolicy::Store, EventLoopErrorPolicy::Quit}) {
            const auto loop = EventLoop::create(EventLoopOptions{}.setErrorPolicy(policy));
            loop->invoke([failure]() -> void { throw failure; });
            REQUIRE_THROWS_AS(Failure, loop->runOnce());
            REQUIRE_FALSE(loop->hasError());
            REQUIRE_FALSE(loop->isRunning());
        }
        const auto loop = EventLoop::create();
        auto handlerCalled = false;
        loop->setErrorHandler([&](std::exception_ptr) -> EventLoopErrorAction {
            handlerCalled = true;
            return EventLoopErrorAction::Continue;
        });
        loop->invoke([failure]() -> void { throw failure; });
        REQUIRE_THROWS_AS(Failure, loop->runOnce());
        REQUIRE_FALSE(handlerCalled);
    }

public:
    void testFatalCallbacksBypassRecoveryPolicies() {
        WITH_CONTEXT(verifyCallbackFailure(std::bad_alloc{}));
        WITH_CONTEXT(
            verifyCallbackFailure(std::system_error{std::make_error_code(std::errc::resource_unavailable_try_again)}));
        WITH_CONTEXT(verifyCallbackFailure(el::err::LogicFailure{"Programming error."_el}));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testFatalWorkerFailuresTerminate() {
        for (const auto mode : {"io-memory"_el, "co-thread"_el, "handler-memory"_el}) {
            const auto child = el::system::Subprocess::create(
                el::path::Path{el::unittest::fh::unitTestExecutablePath()},
                el::text::StringList{"name:FatalWorkerChild"_el, "+name:FatalWorkerChild"_el, "--no-color"_el},
                el::system::SubprocessOptions{}.setEnvironmentChanges(
                    el::system::EnvironmentChanges{}.setVariable("ERBSLAND_FATAL_WORKER_TEST"_el, mode)));
            child->start();
            const auto status = child->wait(el::time::TimeDelta::seconds(10));
            REQUIRE(status.has_value());
            REQUIRE(status->exitCode().has_value());
            REQUIRE_EQUAL(*status->exitCode(), std::int32_t{73});
        }
    }

    SKIP_BY_DEFAULT()
    void testFatalWorkerChild() {
        const auto mode = el::system::EnvironmentVariables{}.get("ERBSLAND_FATAL_WORKER_TEST"_el);
        if (!mode.has_value()) {
            return;
        }
        // Verify termination itself; the synchronous tests separately check the original exception type.
        // Active exception inspection during nested noexcept termination differs between C++ runtimes.
        const std::terminate_handler terminationHandler = []() -> void { std::_Exit(73); };
        std::set_terminate(terminationHandler);
        if (*mode == "io-memory"_el) {
            el::util::impl::worker_services::io().submit([terminationHandler]() -> void {
                // Install on the throwing thread as well for C++ runtimes with per-thread handlers.
                std::set_terminate(terminationHandler);
                throw std::bad_alloc{};
            });
        } else if (*mode == "handler-memory"_el) {
            const auto loop = EventLoop::create();
            loop->setErrorHandler([](std::exception_ptr) -> EventLoopErrorAction { throw std::bad_alloc{}; });
            loop->invoke([]() -> void { throw el::err::RuntimeError{"Operational failure."_el}; });
            loop->run();
        } else {
            el::util::impl::worker_services::coroutine().submit([terminationHandler]() -> void {
                std::set_terminate(terminationHandler);
                throw std::system_error{std::make_error_code(std::errc::resource_unavailable_try_again)};
            });
        }
        // A swallowed exception must fail deterministically rather than leave the child waiting forever.
        std::this_thread::sleep_for(std::chrono::seconds{2});
        std::_Exit(75);
    }
};
