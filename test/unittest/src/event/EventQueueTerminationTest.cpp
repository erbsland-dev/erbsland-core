// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/Subprocess.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdlib>
#include <exception>

using namespace el::event;
using namespace el::text::literals;

TESTED_TARGETS(EventQueueOverflowAction EventLoop)
class EventQueueTerminationTest final : public el::UnitTest {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testQueueTerminationInChildProcesses() {
        for (const auto mode : {"overflow"_el, "self"_el, "backend"_el}) {
            auto options = el::system::SubprocessOptions{};
            options.environmentChanges().setVariable("ERBSLAND_EVENT_TERMINATION_TEST"_el, mode);
            auto child = el::system::Subprocess::create(
                el::path::Path{el::unittest::fh::unitTestExecutablePath()},
                el::text::StringList{"name:ChildTermination"_el, "+name:ChildTermination"_el, "--no-color"_el},
                options);
            child->start();
            const auto status = child->wait(el::time::TimeDelta::seconds(10));
            REQUIRE(status.has_value());
            REQUIRE_EQUAL(status->exitCode(), std::optional<std::int32_t>{73});
        }
    }

    SKIP_BY_DEFAULT()
    void testChildTermination() {
        const auto mode = el::system::EnvironmentVariables{}.get("ERBSLAND_EVENT_TERMINATION_TEST"_el);
        if (!mode.has_value()) {
            return;
        }
        std::set_terminate([]() -> void { std::_Exit(73); });
        auto options = EventLoopOptions{}.setMaximumQueuedEvents(1);
        if (*mode != "overflow"_el) {
            options.setQueueOverflowAction(EventQueueOverflowAction::Block);
        }
        const auto loop = EventLoop::create(options);
        if (*mode == "overflow"_el) {
            loop->invoke([]() -> void {});
            loop->invoke([]() -> void {});
        } else if (*mode == "self"_el) {
            loop->invoke([loop]() -> void {
                loop->invoke([]() -> void {});
                loop->invoke([]() -> void {});
            });
            loop->run();
        } else {
            loop->invoke([]() -> void {});
            const auto timer = loop->createTimer([]() -> void {});
            timer->startOnce(el::time::TimeDelta::zero());
            loop->run();
        }
        REQUIRE(false);
    }
};
