// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/cterm/TerminalEmulation.hpp>
#include <tool/pre_commit/impl/TaskDisplay.hpp>

TESTED_TARGETS(TaskDisplay)
class PreCommitDisplayTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testInteractiveRowsRetainHistoryAndClearOldProgress() {
        runInsideApplication([&]() -> void {
            const auto output = el::stream::AnyStringBuilderStream::create();
            auto terminal = std::make_shared<el::cterm::Terminal>(
                std::make_shared<CapturedTerminalBackend>(output, true), el::block::Size{100, 12});
            terminal->initializeScreen();
            const auto progress = tool::frame::TaskProgress::create();
            {
                auto display = tool::pre_commit::impl::TaskDisplay{terminal};
                display.begin("first task"_el, progress);
                progress.beginPhase("files"_el, el::ItemCount{10});
                progress.advance(el::ItemCount{4});
                display.refresh();
                progress.beginPhase("empty"_el, el::ItemCount{0});
                display.refresh();
                display.finish(true, false, el::time::TimeDelta::milliseconds(10));
                display.begin("second task"_el, progress);
                progress.beginPhase("discovering"_el);
                display.refresh();
                display.finish(false, true, el::time::TimeDelta::milliseconds(20));
                display.summary(1, 1, el::time::TimeDelta::milliseconds(30));
            }
            terminal->restoreScreen();
            auto emulator = el::cterm::TerminalEmulation{el::cterm::TerminalEmulationOptions{}.setSize({100, 12})};
            emulator.write(output->toString());
            emulator.finish();
            const auto text = emulator.regularBuffer().toString();
            REQUIRE(text.contains("✔ first task"_el));
            REQUIRE(text.contains("✖ second task"_el));
            REQUIRE(text.contains("1 passed, 1 failed"_el));
            REQUIRE_FALSE(text.contains("discovering"_el));
            REQUIRE_FALSE(text.contains("4/10"_el));
            REQUIRE_FALSE(emulator.isAlternateScreenActive());
            REQUIRE(output->toString().contains("4/10"_el));
            REQUIRE(output->toString().contains("100% 0/0"_el));
            REQUIRE(output->toString().contains("discovering"_el));
        });
    }

    void testNarrowAndResizedTerminal() {
        runInsideApplication([&]() -> void {
            const auto output = el::stream::AnyStringBuilderStream::create();
            auto terminal = std::make_shared<el::cterm::Terminal>(
                std::make_shared<CapturedTerminalBackend>(output, true), el::block::Size{24, 8});
            terminal->initializeScreen();
            const auto progress = tool::frame::TaskProgress::create();
            auto resizeOffset = el::ByteCount{};
            {
                auto display = tool::pre_commit::impl::TaskDisplay{terminal};
                display.begin("界界 Wide characters and a very long task title"_el, progress);
                progress.beginPhase("phase"_el, el::ItemCount{1});
                display.refresh();
                resizeOffset = output->toString().length();
                terminal->setSize({12, 8});
                progress.advance();
                display.refresh();
                display.finish(true, false, el::time::TimeDelta{});
            }
            terminal->restoreScreen();
            const auto afterResize =
                output->toString().slice(el::StringSide::Back, output->toString().length() - resizeOffset);
            REQUIRE(afterResize.contains("100%"_el));
            auto emulator = el::cterm::TerminalEmulation{el::cterm::TerminalEmulationOptions{}.setSize({12, 8})};
            emulator.write(afterResize);
            emulator.finish();
            const auto text = emulator.regularBuffer().toString();
            REQUIRE(text.contains("✔"_el));
            REQUIRE(text.contains("0.0s"_el));
            REQUIRE_FALSE(text.contains("100%"_el));
            REQUIRE_FALSE(text.contains("phase"_el));
            REQUIRE_FALSE(terminal->isAlternateScreenActive());
        });
    }
};
