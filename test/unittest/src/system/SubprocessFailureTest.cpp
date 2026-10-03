// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/EventBackendTarget.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/PlatformError.hpp>
#include <erbsland/system/impl/ProcessIdAccess.hpp>
#include <erbsland/system/impl/subprocess/SubprocessErrorContext.hpp>
#include <erbsland/system/impl/subprocess/SubprocessState.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <new>
#include <system_error>

using namespace el::text::literals;

TESTED_TARGETS(SubprocessState SubprocessBackend)
class SubprocessFailureTest final : public el::UnitTest {
    using State = el::system::impl::SubprocessState;
    using Status = el::system::SubprocessExitStatus;
    /// Deterministic native wait failure and observable cleanup.
    class Backend final : public el::system::impl::SubprocessBackend {
    public:
        explicit Backend(bool failWait) : _failWait{failWait} {}
        auto processId() const noexcept -> el::system::ProcessId override {
            return el::system::impl::ProcessIdAccess::fromNative(123);
        }
        auto isRunning() -> bool override { return !_killed; }
        auto wait() -> Status override { return *nativeStatus(); }
        auto wait(el::time::TimeDelta) -> std::optional<Status> override { return nativeStatus(); }
        void terminate() override { _killed = true; }
        void kill() override { _killed = true; }

    private:
        auto nativeStatus() -> std::optional<Status> {
            if (_failWait) {
                _failWait = false;
                throw el::system::PlatformError{
                    "Injected wait failure."_el,
                    std::make_shared<el::system::impl::SubprocessErrorContext>("wait failed"_el)};
            }
            return Status::exited(0);
        }
        bool _failWait;
        bool _killed{};
    };

    template <typename Failure>
    void verifyFatalLaunch(const Failure &failure) {
        const auto loop = el::event::EventLoop::create();
        auto state = std::make_shared<State>(
            el::path::Path{"fixture"_el},
            el::text::StringList{},
            el::system::SubprocessOptions{},
            [failure](const auto &, const auto &, const auto &, const auto &)
                -> el::system::impl::SubprocessBackendPtr { throw failure; });
        auto notified = false;
        state->bind(std::dynamic_pointer_cast<el::event::EventBackendTarget>(loop));
        state->setStarted([&]() -> void { notified = true; });
        state->setError([&](const auto &) -> void { notified = true; });
        state->setFinal([&]() -> void { notified = true; });
        REQUIRE_THROWS_AS(Failure, state->start());
        REQUIRE_THROWS_AS(Failure, state->wait());
        state->dispatch();
        static_cast<void>(loop->runUntilIdle());
        REQUIRE_FALSE(notified);
        REQUIRE_FALSE(state->isRunning());
        state.reset();
    }

public:
    void testFatalLaunchFailuresKeepTheirType() {
        WITH_CONTEXT(verifyFatalLaunch(std::bad_alloc{}));
        WITH_CONTEXT(
            verifyFatalLaunch(std::system_error{std::make_error_code(std::errc::resource_unavailable_try_again)}));
        WITH_CONTEXT(verifyFatalLaunch(el::err::LogicFailure{"Injected programming error."_el}));
    }

public:
    void testReadAndWaitFailuresReachBothApiStyles() {
        for (const auto failWait : {false, true}) {
            const auto loop = el::event::EventLoop::create();
            auto state = std::make_shared<State>(
                el::path::Path{"fixture"_el},
                el::text::StringList{},
                el::system::SubprocessOptions{},
                [failWait](
                    const el::path::Path &,
                    const el::text::StringList &,
                    const el::system::SubprocessOptions &,
                    const el::system::impl::SubprocessOutputSink &sink) -> el::system::impl::SubprocessBackendPtr {
                    if (!failWait) {
                        sink.error(std::make_shared<el::system::impl::SubprocessErrorContext>("read failed"_el));
                    }
                    return std::make_unique<Backend>(failWait);
                });
            auto order = el::text::StringEditor{};
            loop->invoke([&]() -> void {
                state->bind(std::dynamic_pointer_cast<el::event::EventBackendTarget>(loop));
                state->setError(
                    el::system::SubprocessEventEditor::ErrorFn{
                        [&](const el::system::PlatformErrorContext &context) -> void {
                            REQUIRE(context.toString().contains("failed"_el));
                            order.append("error;"_el);
                        }});
                state->setExit(el::system::SubprocessEventEditor::ExitFn{[&](el::unit::ExitCode) -> void {
                    order.append("exit;"_el);
                }});
                state->setFinal(
                    el::system::SubprocessEventEditor::EventFn{[&]() -> void { order.append("final;"_el); }});
                state->start();
                REQUIRE_THROWS_AS(el::system::PlatformError, state->wait());
                state->dispatch();
                state->dispatch();
            });
            REQUIRE(loop->runOnce());
            if (loop->hasError()) {
                std::rethrow_exception(loop->takeError());
            }
            REQUIRE_EQUAL(el::text::String{order}, "error;final;"_el);
        }
    }
};
