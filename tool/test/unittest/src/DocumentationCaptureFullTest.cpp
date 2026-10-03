// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/cterm/TerminalEmulation.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <tool/doc/impl/DemoCapture.hpp>

TESTED_TARGETS(DemoCapture VirtualBackend)
SKIP_BY_DEFAULT()
TAGS(FullRun)
class DocumentationCaptureFullTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testVirtualTerminalOutputAndRestoration() {
        const auto executable = el::Path{std::filesystem::path{ERBSLAND_CORE_TOOL_CAPTURE_FIXTURE}};
        const auto output =
            tool::doc::impl::DemoCapture::create()->run(executable, el::StringList{"interactive"_el}, _root, 0, 120);
        REQUIRE(output.contains("terminal hé界"_el));
        REQUIRE(output.contains("size=120x40"_el));
        REQUIRE(output.contains("merged error"_el));
        REQUIRE(output.contains("\x1b[31m"_el));
        REQUIRE(output.contains("\x1b[?1049h"_el));
        REQUIRE(output.contains("\x1b[?1049l"_el));
        REQUIRE(output.endsWith("\x1b[?25h"_el));
        auto emulator = el::cterm::TerminalEmulation{el::cterm::TerminalEmulationOptions{}.setSize({120, 40})};
        emulator.write(output);
        emulator.finish();
        REQUIRE_FALSE(emulator.isAlternateScreenActive());
        REQUIRE(emulator.regularBuffer().toString().contains("size=120x40"_el));
        REQUIRE(emulator.alternateBuffer().toString().contains("alternate"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testDrainsLargeOutputAndChecksExitStatus() {
        const auto executable = el::Path{std::filesystem::path{ERBSLAND_CORE_TOOL_CAPTURE_FIXTURE}};
        auto capture = tool::doc::impl::DemoCapture::create();
        const auto output = capture->run(executable, el::StringList{"drain"_el}, _root, 7);
        REQUIRE_EQUAL(output.length(), el::ByteCount{256U * 1024U});
        REQUIRE_THROWS(capture->run(executable, el::StringList{"drain"_el}, _root, 99));
        REQUIRE_THROWS(capture->run(executable, el::StringList{"limit"_el}, _root, 7));
        REQUIRE(capture->run(executable, el::StringList{"interactive"_el}, _root, 0).contains("size=90x40"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testTimeoutTerminatesChildAndAllowsReuse() {
        const auto executable = el::Path{std::filesystem::path{ERBSLAND_CORE_TOOL_CAPTURE_FIXTURE}};
        auto capture = tool::doc::impl::DemoCapture::create();
        REQUIRE_THROWS(capture->run(executable, el::StringList{"timeout"_el}, _root, 0));
        REQUIRE(capture->run(executable, el::StringList{"interactive"_el}, _root, 0).contains("terminal"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCaptureDoesNotInheritUnrelatedEnvironmentVariables() {
        auto environment = el::EnvironmentVariables{};
        const auto previous = environment.get("ERBSLAND_CAPTURE_SECRET"_el);
        environment.setOrThrow("ERBSLAND_CAPTURE_SECRET"_el, "must-not-reach-the-child"_el);
        auto output = el::String{};
        try {
            const auto executable = el::Path{std::filesystem::path{ERBSLAND_CORE_TOOL_CAPTURE_FIXTURE}};
            output =
                tool::doc::impl::DemoCapture::create()->run(executable, el::StringList{"interactive"_el}, _root, 0);
        } catch (...) {
            restoreEnvironment(previous);
            throw;
        }
        restoreEnvironment(previous);
        REQUIRE(output.contains("terminal"_el));
    }

private:
    static void restoreEnvironment(const std::optional<el::String> &value) {
        auto environment = el::EnvironmentVariables{};
        if (value) {
            environment.setOrThrow("ERBSLAND_CAPTURE_SECRET"_el, *value);
        } else {
            environment.removeOrThrow("ERBSLAND_CAPTURE_SECRET"_el);
        }
    }
};
