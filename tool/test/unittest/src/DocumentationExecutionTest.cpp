// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"
#include "RecordingDemoCapture.hpp"

#include <erbsland/EnvironmentVariables.hpp>
#include <tool/doc/impl/DemoExecutor.hpp>

using namespace el::text::literals;

TESTED_TARGETS(DemoExecutor)
class DocumentationExecutionTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testArgumentsDoNotExpandShellSyntax() {
        REQUIRE_EQUAL(
            tool::doc::impl::DemoExecutor::arguments("demo 'one value' \"two values\""_el).count(), el::ItemCount{3});
        REQUIRE_THROWS(tool::doc::impl::DemoExecutor::arguments("demo 'unfinished"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testTemporaryFilePathsAreRedactedBeforeAnsiRendering() {
        writeExecutable("cmake-build-debug/demo-apps/demo"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto executor = tool::doc::impl::DemoExecutor{files, std::make_unique<RecordingDemoCapture>()};
        const auto result = executor.run("demo {file:text}"_el, {}, 0);
        REQUIRE(result.text.contains("sample-text.txt"_el));
        REQUIRE_FALSE(result.text.contains(_root.toString()));
        REQUIRE_FALSE(result.text.contains(_root.toNativeString()));
        REQUIRE(result.text.contains("\x1b["_el));
        REQUIRE_EQUAL(result.command, "demo sample-text.txt"_el);
        REQUIRE_THROWS(executor.run("demo ;bad"_el, {}, 0));
        REQUIRE_THROWS(executor.run("../escape"_el, {}, 0));
    }

    void testCaptureCombinesScrollbackWithTheFixedTerminalFrame() {
        writeExecutable("cmake-build-debug/demo-apps/demo"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto raw = el::StringEditor{};
        for (auto row = 0; row < 50; ++row) {
            raw.append("line"_el).append(el::String::fromInteger(row)).append(U'\n');
        }
        raw.append("\x1b[HFRAME"_el);
        auto executor = tool::doc::impl::DemoExecutor{files, std::make_unique<RecordingDemoCapture>(el::String{raw})};
        const auto result = executor.run("demo"_el, {}, 0);
        REQUIRE(result.text.contains("line0"_el));
        REQUIRE(result.text.contains("line10"_el));
        REQUIRE(result.text.contains("FRAME"_el));
        REQUIRE(result.text.contains("line49"_el));
    }

    void testDocumentRelativeFilesRequireUniqueBasenamesAndConfinement() {
        write("doc/a/input.txt"_el, "first"_el);
        write("doc/b/input.txt"_el, "second"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto executor = tool::doc::impl::DemoExecutor{files, std::make_unique<RecordingDemoCapture>()};
        REQUIRE_THROWS(executor.resolveFiles(_root / "doc/page.rst"_el, "a/input.txt b/input.txt"_el));
        REQUIRE_THROWS(executor.resolveFiles(_root / "doc/page.rst"_el, "../../../escape.txt"_el));
        REQUIRE_EQUAL(executor.resolveFiles(_root / "doc/page.rst"_el, "a/input.txt"_el).count(), el::ItemCount{1});
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCustomRelativeAndAbsoluteExecutableRoots() {
        writeExecutable("special-build/demo-apps/demo"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto relative = tool::doc::impl::DemoExecutor{
            files, std::make_unique<RecordingDemoCapture>("relative"_el), el::Path{"special-build/demo-apps"_el}};
        REQUIRE(relative.run("demo"_el, {}, 0).text.contains("relative"_el));
        const auto outsideRoot = _temporary->path() / "separate-build"_el;
        const auto executable =
            el::EnvironmentVariables{}.executableCandidatesOrThrow((outsideRoot / "demo"_el).toNativeString()).first();
        executable.content().writeTextOrThrow("fixture"_el, el::PathWriteTextOptions{}.setCreateParents(true));
        auto absolute =
            tool::doc::impl::DemoExecutor{files, std::make_unique<RecordingDemoCapture>("absolute"_el), outsideRoot};
        REQUIRE(absolute.run("demo"_el, {}, 0).text.contains("absolute"_el));
        REQUIRE_THROWS(absolute.run("../demo"_el, {}, 0));
        REQUIRE_THROWS(absolute.run("missing"_el, {}, 0));
    }

private:
    void writeExecutable(const el::String &relative) {
        const auto executable =
            el::EnvironmentVariables{}.executableCandidatesOrThrow((_root / relative).toNativeString()).first();
        write(executable.toRelativeOrThrow(_root).toString(), "fixture"_el);
    }
};
