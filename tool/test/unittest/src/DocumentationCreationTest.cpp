// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/PathInfo.hpp>
#include <tool/doc/CreateDemoTool.hpp>
#include <tool/doc/UpdateReferenceTool.hpp>

using namespace el::text::literals;

TESTED_TARGETS(CreateDemoTool DemoCreation UpdateReferenceTool ReferenceDocumentation)
class DocumentationCreationTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCreateAndExtendDemo() {
        REQUIRE_EQUAL(
            runTool<tool::doc::CreateDemoTool>(
                {}, {}, "create-demo"_el, el::StringList{"alpha/Sample/First"_el, "--yes"_el}),
            0);
        REQUIRE((_root / "demos/alpha/Sample/First.cpp"_el).info().isRegularFile());
        REQUIRE(
            (_root / "demos/alpha/Sample/SampleDemos.hpp"_el).content().readTextOrThrow().contains("void first();"_el));
        REQUIRE_EQUAL(
            runTool<tool::doc::CreateDemoTool>(
                {}, {}, "create-demo"_el, el::StringList{"alpha/Sample/Second"_el, "--yes"_el}),
            0);
        const auto main = (_root / "demos/alpha/Sample/main.cpp"_el).content().readTextOrThrow();
        REQUIRE(main.contains("app.registerDemo(\"First\"_el, first);"_el));
        REQUIRE(main.contains("app.registerDemo(\"Second\"_el, second);"_el));
        const auto before = (_root / "demos/alpha/Sample/CMakeLists.txt"_el).content().readTextOrThrow();
        REQUIRE(
            runTool<tool::doc::CreateDemoTool>(
                {}, {}, "create-demo"_el, el::StringList{"alpha/Sample/First"_el, "--yes"_el}) != 0);
        REQUIRE_EQUAL((_root / "demos/alpha/Sample/CMakeLists.txt"_el).content().readTextOrThrow(), before);
    }

    void testDemoTargetUsesAsciiWordBoundaries() {
        REQUIRE_EQUAL(
            runTool<tool::doc::CreateDemoTool>(
                {}, {}, "create-demo"_el, el::StringList{"alpha/HTTPServer2Value/First"_el, "--yes"_el}),
            0);
        const auto cmake = (_root / "demos/alpha/HTTPServer2Value/CMakeLists.txt"_el).content().readTextOrThrow();
        REQUIRE(cmake.contains("erbsland_core_add_demo(http_server2_value)"_el));
    }

    void testReferenceHeadingUsesUnicodeWords() {
        write(
            "src/erbsland/example/Value.hpp"_el,
            "namespace erbsland::example {\n/// Example.\nclass Value {};\n}\n"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            u8"[rendering]\n[doc_update_reference]\n*[doc_update_reference.reference_groups]\npage: \"example/__éValue__andHTTPServer__.rst\"\nheaders: \"example/Value.hpp\"\n"_el);
        REQUIRE_EQUAL(runTool<tool::doc::UpdateReferenceTool>({}, {}, "doc-update-reference"_el), 0);
        const auto text =
            (_root / u8"doc/reference/example/__éValue__andHTTPServer__.rst"_el).content().readTextOrThrow();
        REQUIRE(text.contains(u8"É Value And Http Server"_el));
    }

    void testInvalidDemoPathWritesNothing() {
        REQUIRE(
            runTool<tool::doc::CreateDemoTool>(
                {}, {}, "create-demo"_el, el::StringList{"../Sample/First"_el, "--yes"_el}) != 0);
        REQUIRE_FALSE((_root / "demos"_el).info().exists());
    }

    void testReferenceRejectsMissingOrDuplicateInterfaceHeadings() {
        write(
            "src/erbsland/example/Value.hpp"_el,
            "namespace erbsland::example {\n/// Example.\nclass Value {};\n}\n"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[doc_update_reference]\n*[doc_update_reference.reference_groups]\npage: \"example/value.rst\"\nheaders: \"example/Value.hpp\"\n"_el);
        for (
            const auto text :
            {"Keep this introduction.\nInterface=========\n"_el,
                "Interface\n=========\nfirst\nInterface\n=========\nsecond\n"_el,
                "Interface\n========= trailing\n"_el}) {
            write("doc/reference/example/value.rst"_el, text);
            REQUIRE(runTool<tool::doc::UpdateReferenceTool>({}, {}, "doc-update-reference"_el) != 0);
            REQUIRE_EQUAL((_root / "doc/reference/example/value.rst"_el).content().readTextOrThrow(), text);
        }
    }

    void testReferenceGenerationPreservesProseAndRemovesOrphans() {
        write(
            "src/erbsland/example/Value.hpp"_el,
            "namespace erbsland::example {\n/// Example.\nclass Value {};\n}\n"_el);
        write(
            "doc/reference/example/value.rst"_el,
            "*****\nValue\n*****\n\nKeep this introduction.\n\nInterface\n=========\n\nobsolete\n"_el);
        write("doc/reference/example/orphan.rst"_el, "obsolete\n"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[doc_update_reference]\n*[doc_update_reference.reference_groups]\npage: \"example/value.rst\"\nheaders: \"example/Value.hpp\"\n"_el);
        REQUIRE_EQUAL(runTool<tool::doc::UpdateReferenceTool>({}, {}, "doc-update-reference"_el), 0);
        const auto text = (_root / "doc/reference/example/value.rst"_el).content().readTextOrThrow();
        REQUIRE(text.contains("Keep this introduction."_el));
        REQUIRE(text.contains(".. doxygenclass:: erbsland::example::Value"_el));
        REQUIRE_FALSE((_root / "doc/reference/example/orphan.rst"_el).info().exists());
        REQUIRE_EQUAL(runTool<tool::doc::UpdateReferenceTool>({}, {}, "doc-update-reference"_el), 0);
        REQUIRE_EQUAL((_root / "doc/reference/example/value.rst"_el).content().readTextOrThrow(), text);
    }
};
