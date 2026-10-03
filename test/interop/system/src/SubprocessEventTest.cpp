// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/DetachedSubprocess.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/PlatformError.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/Subprocess.hpp>
#include <erbsland/System.hpp>
#include <erbsland/TimePoint.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <thread>

using namespace el::text::literals;

TESTED_TARGETS(Subprocess SubprocessEventEditor System)
class SubprocessEventTest final : public el::UnitTest {
    using Process = el::system::Subprocess;
    using Options = el::system::SubprocessOptions;
    using Mode = el::system::SubprocessOutputMode;
    using Loop = el::event::EventLoop;
    using Delta = el::time::TimeDelta;

private:
    static auto executable() -> el::path::Path { return el::path::Path{el::unittest::fh::unitTestExecutablePath()}; }
    static auto options() -> Options {
        return Options{}
            .setInheritStandardInput(false)
            .setStandardOutputMode(Mode::Capture)
            .setStandardErrorMode(Mode::Capture);
    }
    void drain(const el::event::EventLoopPtr &loop, const bool &finished) {
        const auto deadline = el::time::TimePoint::inFuture(Delta::seconds(5));
        while (!finished && el::time::TimePoint::now() < deadline) {
            static_cast<void>(loop->runOnce(Delta::milliseconds(20)));
        }
        if (loop->hasError()) {
            std::rethrow_exception(loop->takeError());
        }
        REQUIRE(finished);
    }

public:
    void testLazyBindingAndValidation() {
        const auto process = Process::create(executable(), {}, options());
        REQUIRE_FALSE(process->processId().isValid());
        REQUIRE_FALSE(process->isRunning());
        REQUIRE_THROWS_AS(el::err::LogicFailure, process->events());
        REQUIRE_THROWS_AS(el::err::LogicFailure, process->wait());
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            Process::create(executable(), {}, options().setMaximumOutputChunkSize(el::unit::ByteCount{255})));
        const auto loop = Loop::create();
        loop->invoke([&]() -> void {
            REQUIRE_EQUAL(process->events().source().get(), process.get());
            REQUIRE_EQUAL(process->events().target(), loop);
            REQUIRE_EQUAL(&process->events(), &process->events());
        });
        REQUIRE(loop->runOnce());
        const auto otherLoop = Loop::create();
        otherLoop->invoke([&]() -> void { REQUIRE_THROWS_AS(el::err::LogicFailure, process->events()); });
        REQUIRE(otherLoop->runOnce());
    }

    void testNormalAndNonzeroExitOrdering() {
        const auto loop = Loop::create();
        auto process = el::system::SubprocessPtr{};
        auto order = el::text::StringEditor{};
        auto output = el::text::StringEditor{};
        auto errorOutput = el::text::StringEditor{};
        auto finished = false;
        loop->invoke([&]() -> void {
            process = loop->get<el::system::System>().createSubprocess(
                executable(),
                el::text::StringList{"--subprocess-helper"_el, "output"_el, "hello"_el, "error"_el, "7"_el},
                options());
            process->events()
                .onStarted([&]() -> void { order.append("start;"_el); })
                .onStandardOutput([&](const el::text::String &block) -> void { output.append(block); })
                .onStandardError([&](const el::text::String &block) -> void { errorOutput.append(block); })
                .onError([&](const el::system::PlatformErrorContext &) -> void { order.append("error;"_el); })
                .onExit([&](el::unit::ExitCode code) -> void {
                    REQUIRE_EQUAL(code.toRawValue(), 7);
                    REQUIRE_EQUAL(el::text::String{output}, "hello"_el);
                    REQUIRE_EQUAL(el::text::String{errorOutput}, "error"_el);
                    order.append("exit;"_el);
                })
                .onFinal([&]() -> void {
                    order.append("final;"_el);
                    finished = true;
                });
            process->start();
            REQUIRE_THROWS_AS(el::err::LogicFailure, process->start());
            // Procedural wait does not run owner callbacks or duplicate terminal delivery.
            REQUIRE_EQUAL(process->wait().exitCode(), std::optional<std::int32_t>{7});
            REQUIRE(order.isEmpty());
        });
        drain(loop, finished);
        REQUIRE_EQUAL(el::text::String{order}, "start;exit;final;"_el);
        REQUIRE(process->standardOutput().isEmpty());
        REQUIRE(process->standardError().isEmpty());
    }

    void testLaunchFailureAndCrashHaveExclusiveOutcome() {
        for (const auto crash : {false, true}) {
            const auto loop = Loop::create();
            auto process = el::system::SubprocessPtr{};
            auto failed = 0;
            auto exited = 0;
            auto started = 0;
            auto finished = false;
            loop->invoke([&]() -> void {
                auto path = executable();
                if (!crash) {
                    path /= "missing-executable"_el;
                }
                process = loop->get<el::system::System>().createSubprocess(
                    path, el::text::StringList{"--subprocess-helper"_el, "crash"_el}, options());
                process->events()
                    .onStarted([&]() -> void { ++started; })
                    .onError([&](const el::system::PlatformErrorContext &context) -> void {
                        REQUIRE_FALSE(context.toString().isEmpty());
                        ++failed;
                    })
                    .onExit([&](el::unit::ExitCode) -> void { ++exited; })
                    .onFinal([&]() -> void { finished = true; });
                if (crash) {
                    process->start();
                } else {
                    REQUIRE_THROWS_AS(el::system::PlatformError, process->start());
                }
            });
            drain(loop, finished);
            REQUIRE_EQUAL(failed, 1);
            REQUIRE_EQUAL(exited, 0);
            REQUIRE_EQUAL(started, static_cast<int>(crash));
        }
    }

    void testUtf8RawBytesAndMergedOutput() {
        const auto loop = Loop::create();
        auto finished = false;
        auto process = el::system::SubprocessPtr{};
        auto output = el::text::StringEditor{};
        loop->invoke([&]() -> void {
            process = loop->get<el::system::System>().createSubprocess(
                executable(),
                el::text::StringList{"--subprocess-helper"_el, "utf8-output"_el},
                options().setMaximumOutputChunkSize(el::unit::ByteCount{256}).setMergeStandardError(true));
            process->events()
                .onStandardOutput([&](const el::text::String &block) -> void {
                    REQUIRE(block.length() <= el::unit::ByteCount{256});
                    output.append(block);
                })
                .onFinal([&]() -> void { finished = true; });
            process->start();
        });
        drain(loop, finished);
        const auto expected =
            el::text::StringConverter{el::unittest::th::stdStringFromHex("61f09f92a1ff0062e282ace2")}.toString();
        REQUIRE_EQUAL(el::text::String{output}, expected);
        REQUIRE(process->standardError().isEmpty());
    }

    void testChunkLimitsNeverSplitValidUtf8() {
        const auto loop = Loop::create();
        auto finished = false;
        auto process = el::system::SubprocessPtr{};
        auto length = el::unit::ByteCount{};
        loop->invoke([&]() -> void {
            process = loop->get<el::system::System>().createSubprocess(
                executable(),
                el::text::StringList{"--subprocess-helper"_el, "utf8-boundaries"_el},
                options().setMaximumOutputChunkSize(el::unit::ByteCount{256}));
            process->events()
                .onStandardOutput([&](const el::text::String &block) -> void {
                    REQUIRE(block.length() <= el::unit::ByteCount{256});
                    REQUIRE(block.isEncodingValid());
                    length += block.length();
                })
                .onFinal([&]() -> void { finished = true; });
            process->start();
        });
        drain(loop, finished);
        REQUIRE_EQUAL(length, el::unit::ByteCount{259U * 128U});
    }

    void testOverflowIsBoundedAndNotifiesOnce() {
        const auto loop = Loop::create();
        auto finished = false;
        auto overflows = 0;
        auto process = el::system::SubprocessPtr{};
        auto output = el::text::StringEditor{};
        loop->invoke([&]() -> void {
            process = loop->get<el::system::System>().createSubprocess(
                executable(),
                el::text::StringList{"--subprocess-helper"_el, "large-output"_el, "1048576"_el},
                options().setCaptureLimit(el::unit::ByteCount{1024}));
            process->events()
                .onStandardOutput([&](const el::text::String &block) -> void { output.append(block); })
                .onOutputTruncated([&]() -> void { ++overflows; })
                .onFinal([&]() -> void { finished = true; });
            process->start();
            REQUIRE(process->wait().isSuccess());
        });
        drain(loop, finished);
        REQUIRE_EQUAL(output.length(), el::unit::ByteCount{1024});
        REQUIRE_EQUAL(overflows, 1);
        REQUIRE(process->wasStandardOutputTruncated());
        REQUIRE_FALSE(process->wasStandardErrorTruncated());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testClearingOutputHandlerResumesCapture() {
        const auto loop = Loop::create();
        auto finished = false;
        auto process = el::system::SubprocessPtr{};
        auto output = el::text::StringEditor{};
        loop->invoke([&]() -> void {
            process = loop->get<el::system::System>().createSubprocess(
                executable(), el::text::StringList{"--subprocess-helper"_el, "phased-output"_el}, options());
            process->events()
                .onStandardOutput([&](const el::text::String &block) -> void {
                    output.append(block);
                    process->events().onStandardOutput({});
                })
                .onFinal([&]() -> void { finished = true; });
            process->start();
        });
        drain(loop, finished);
        REQUIRE_EQUAL(el::text::String{output}, "first"_el);
        REQUIRE_EQUAL(process->standardOutput(), "second"_el);
    }

    void testSourceReleaseSuppressesPendingAndSubsequentCallbacks() {
        const auto loop = Loop::create();
        auto process = el::system::SubprocessPtr{};
        auto exited = false;
        auto final = false;
        loop->invoke([&]() -> void {
            process = loop->get<el::system::System>().createSubprocess(
                executable(), el::text::StringList{"--subprocess-helper"_el, "output"_el}, options());
            process->events()
                .onExit([&](el::unit::ExitCode) -> void {
                    exited = true;
                    process.reset();
                })
                .onFinal([&]() -> void { final = true; });
            process->start();
            static_cast<void>(process->wait());
        });
        static_cast<void>(loop->runUntilIdle());
        REQUIRE(exited);
        REQUIRE_FALSE(final);
    }

    void testSharedOwnershipKeepsRunningChildAlive() {
        auto process = Process::create(
            executable(), el::text::StringList{"--subprocess-helper"_el, "sleep"_el, "5000"_el}, options());
        process->start();
        auto retained = process;
        process.reset();
        REQUIRE(retained->isRunning());
        retained->kill();
        REQUIRE(retained->wait(Delta::seconds(2)).has_value());
    }

    void testCallbackExceptionsUseLoopPolicy() {
        const auto loop = Loop::create();
        auto finished = false;
        auto process = el::system::SubprocessPtr{};
        loop->invoke([&]() -> void {
            process = loop->get<el::system::System>().createSubprocess(
                executable(), el::text::StringList{"--subprocess-helper"_el, "output"_el}, options());
            process->events()
                .onExit([](el::unit::ExitCode) -> void { throw el::err::RuntimeError{"callback failure"_el}; })
                .onFinal([&]() -> void { finished = true; });
            process->start();
        });
        const auto deadline = el::time::TimePoint::inFuture(Delta::seconds(5));
        while (!finished && el::time::TimePoint::now() < deadline) {
            static_cast<void>(loop->runOnce(Delta::milliseconds(20)));
        }
        REQUIRE(finished);
        REQUIRE(loop->hasError());
    }
};
