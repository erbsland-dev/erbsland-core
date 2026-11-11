// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/doc/CreateDemoTool.hpp>
#include <tool/doc/impl/ApiGuidelines.hpp>
#include <tool/doc/impl/ApiTestStatus.hpp>
#include <tool/doc/impl/DemoFingerprint.hpp>
#include <tool/doc/impl/DemoSynchronizer.hpp>
#include <tool/doc/impl/HeaderScanner.hpp>
#include <tool/doc/impl/RstCleaner.hpp>
#include <tool/doc/impl/SourcePreprocessor.hpp>
#include <tool/doc/impl/SourceSelectors.hpp>
#include <tool/doc/ProcessRstTool.hpp>

using namespace el::text::literals;
using namespace tool::doc;

TESTED_TARGETS(
    ApiGuidelines ApiTestStatus SourcePreprocessor SourceSelectors HeaderScanner RstCleaner DemoSynchronizer
        ProcessRstTool)
class DocumentationProcessingTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testSelectorIndentationUsesAsciiWhitespace() {
        const auto source = "    // alpha\n\t// beta\n // gamma\n"_el;
        const auto selectors = impl::SourceSelectors{source};
        REQUIRE_EQUAL(selectors.select("c:alpha"_el, ""_el).join("\n"_el), "// alpha"_el);
        REQUIRE_EQUAL(selectors.select("c:beta"_el, ""_el).join("\n"_el), "// beta"_el);
        REQUIRE_EQUAL(selectors.select("c:gamma"_el, ""_el).join("\n"_el), " // gamma"_el);
        REQUIRE_THROWS(impl::SourceSelectors{" \t\n\n"_el}.select(""_el, ""_el));
    }

    void testVerbosePatternsPreserveHashesAndQuotedSelectors() {
        const auto source = "// first # marker, with (offset)\n// second marker\n"_el;
        const auto selectors = impl::SourceSelectors{source};
        REQUIRE_EQUAL(
            selectors.select(R"sel(c:"first # marker, with (offset)")sel"_el, ""_el).join("\n"_el),
            "// first # marker, with (offset)"_el);
        REQUIRE_THROWS(selectors.select(R"(c:"unclosed)"_el, ""_el));
        const auto transformed =
            impl::SourcePreprocessor::transform("#define ERBSLAND_CORE_SAMPLE 1\nvoid after();\n"_el);
        REQUIRE_FALSE(transformed.contains("#define"_el));
        REQUIRE(transformed.contains("void after();"_el));
        const auto index = impl::DoxygenIndex{};
        const auto cleaner = impl::RstCleaner{index, 80};
        auto warnings = el::StringList{};
        const auto numbered = "#. first item\n#. second item\n"_el;
        REQUIRE_EQUAL(cleaner.process(numbered, warnings), numbered);
    }

    void testSelectorCapturesPreserveEscapesAndUnicode() {
        const auto selectors = impl::SourceSelectors{R"src(// Grüße, "Welt" (detail)
// l'été, ok
// path\,tail)src"_el};
        REQUIRE_EQUAL(
            selectors.select(R"sel(c:"Grüße, \"Welt\" (detail)", c:'l\'été, ok', c:"path\\,tail")sel"_el, ""_el)
                .join("\n"_el),
            R"src(// Grüße, "Welt" (detail)

// l'été, ok

// path\,tail)src"_el);
    }

    void testSelectorRejectsUnbalancedDelimiters() {
        const auto selectors = impl::SourceSelectors{"// first\n// last"_el};
        REQUIRE_THROWS_AS(el::ApplicationError, selectors.select(R"sel(c:"first\")sel"_el, ""_el));
        REQUIRE_THROWS_AS(el::ApplicationError, selectors.select(R"sel(c:"first\)sel"_el, ""_el));
        REQUIRE_THROWS_AS(el::ApplicationError, selectors.select("c:first)"_el, ""_el));
        REQUIRE_THROWS_AS(el::ApplicationError, selectors.select("c:first(+0,+0"_el, ""_el));
        REQUIRE_THROWS_AS(el::ApplicationError, selectors.select(",c:first"_el, ""_el));
        REQUIRE_THROWS_AS(el::ApplicationError, selectors.select("c:first,,c:last"_el, ""_el));
    }

    void testSelectorRangesIncludeBoundaryLines() {
        const auto source = "// first\n// middle\n// last"_el;
        const auto selectors = impl::SourceSelectors{source};
        REQUIRE_EQUAL(selectors.select("c:first"_el, ""_el).join("\n"_el), "// first"_el);
        REQUIRE_EQUAL(selectors.select("c:last"_el, ""_el).join("\n"_el), "// last"_el);
        REQUIRE_EQUAL(selectors.select("c:first-c:last"_el, ""_el).join("\n"_el), source);
        REQUIRE_EQUAL(selectors.select("c:middle(+1)"_el, ""_el).join("\n"_el), source);
        REQUIRE_EQUAL(selectors.select("c:first-c:last(-1,-1)"_el, ""_el).join("\n"_el), "// middle"_el);
        REQUIRE_EQUAL(selectors.select("c:first-c:last"_el, "c:first,c:last"_el).join("\n"_el), "// middle"_el);
        REQUIRE_THROWS_AS(el::ApplicationError, selectors.select("c:first(+1,+0)"_el, ""_el));
        REQUIRE_THROWS_AS(el::ApplicationError, selectors.select("c:last(+0,+1)"_el, ""_el));
        REQUIRE_THROWS_AS(el::ApplicationError, selectors.select("c:middle(-1,+0)"_el, ""_el));
    }

    void testDefaultSelectorRangeEndsBeforeNamespaceBrace() {
        const auto selectors =
            impl::SourceSelectors{"namespace demo {\n    /// Sample\n    void sample() {}\n}\n// outside"_el};
        REQUIRE_EQUAL(selectors.select(""_el, ""_el).join("\n"_el), "/// Sample\nvoid sample() {}"_el);
        const auto unwrapped = impl::SourceSelectors{"// before\n/// Sample\n// last"_el};
        REQUIRE_EQUAL(unwrapped.select(""_el, ""_el).join("\n"_el), "/// Sample\n// last"_el);
    }

    void testExecutionNumbersAreSortedOnceWithEquivalentKeysPreserved() {
        write("demos/example/Sample.cpp"_el, "/// Sample\nvoid sample() {}\n"_el);
        write("cmake-build-debug/demo-apps/.keep"_el, ""_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto synchronizer = impl::DemoSynchronizer{files};
        auto issues = el::StringList{};
        const auto result = synchronizer.process(
            _root / "page.rst"_el,
            ".. erbsland-demo::\n    :source: example/Sample.cpp\n    :exec-2: missing-two\n    :exec-1: missing-one\n    :exec-01: missing-zero-one\n    :exec: missing-default\n\n.. erbsland-demo-end::\n"_el,
            true,
            issues);
        REQUIRE(result.contains(".. note::"_el));
        REQUIRE_EQUAL(issues.count(), el::ItemCount{4});
        REQUIRE(issues.getRef(el::ItemIndex{0}).contains("missing-default"_el));
        REQUIRE(issues.getRef(el::ItemIndex{1}).contains("missing-zero-one"_el));
        REQUIRE(issues.getRef(el::ItemIndex{2}).contains("missing-one"_el));
        REQUIRE(issues.getRef(el::ItemIndex{3}).contains("missing-two"_el));
    }

    void testPreprocessorPreservesConstraintsAndQualifiers() {
        const auto input =
            "template <typename T>\n    requires Small<T>\nauto value() const noexcept -> int;\nvirtual auto other() const noexcept -> bool override = 0;\n"_el;
        const auto text = impl::SourcePreprocessor::transform(input);
        REQUIRE(text.contains("requires Small<T>"_el));
        REQUIRE_FALSE(
            impl::SourcePreprocessor::transform(
                "#define ERBSLAND_CORE_SAMPLE(x) \\\n    auto body() -> int;\nvoid after();\n"_el)
                .contains("body"_el));
        REQUIRE(text.contains("int value() const noexcept"_el));
        REQUIRE(text.contains("bool other() const noexcept override = 0;"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testPreprocessingIsCachedAndRemovesDeletedInputs() {
        write("src/erbsland/Sample.hpp"_el, "auto value() -> int;\n"_el);
        write("src/erbsland/all_test.hpp"_el, "ignored\n"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto processor = impl::SourcePreprocessor{files};
        REQUIRE(processor.run());
        REQUIRE_FALSE(processor.run());
        REQUIRE_FALSE((_root / "_doxygen_input/erbsland/all_test.hpp"_el).info().exists());
        REQUIRE(processor.run(true));
        (_root / "src/erbsland/Sample.hpp"_el).operations().removeOrThrow();
        REQUIRE(processor.run());
        REQUIRE_FALSE((_root / "_doxygen_input/erbsland/Sample.hpp"_el).info().exists());
    }

    void testCoverageMarkersNormalizeAndValidate() {
        const auto text = "/// Example.\n/// @tested{OneTest::testOne, TwoTest}\nclass Value {};\n"_el;
        const auto normalized = impl::test_status::normalize(text);
        REQUIRE(normalized.contains("@tested{OneTest TwoTest}"_el));
        REQUIRE(impl::test_status::validate(normalized).isEmpty());
        REQUIRE_FALSE(impl::test_status::validate("/// @tested{Bad}\nclass Value {};\n"_el).isEmpty());
        REQUIRE_FALSE(impl::test_status::validate("    /// @tested{OneTest}\n    void member();\n"_el).isEmpty());
    }

    void testGuidelineStructureAndNestedReturns() {
        const auto source =
            "*******\nExample\n*******\n\nPrimary Types\n=============\n\n.. code-block:: text\n\n    Value // a value\n\nValue Patterns\n==============\n\n.. code-block:: text\n\n    o.get() -> List<Pair<A, B>> // get values\n\n"_el;
        REQUIRE(impl::ApiGuidelines{}.validate(source).isEmpty());
        REQUIRE_FALSE(
            impl::ApiGuidelines{}.validate(el::String{source}.replacedAll("o.get()"_el, "bad syntax"_el)).isEmpty());
    }

    void testSelectorsUseOriginalLinesAndHideWithoutChangingIndent() {
        const auto source =
            "#include <DemoCommon.hpp>\nnamespace demo {\n/// Example\nvoid example() {\n    auto value = 1;\n    // Keep this comment\n    use(value);\n}\n}\n"_el;
        const auto selection = impl::SourceSelectors{source}.select("f:example"_el, "a:value"_el);
        REQUIRE(selection.join("\n"_el).contains("    use(value);"_el));
        REQUIRE_FALSE(selection.join("\n"_el).contains("auto value"_el));
        REQUIRE_THROWS(impl::SourceSelectors{source}.select("f:missing"_el, ""_el));
    }

    void testSelectorsFindQualifiedMethods() {
        const auto source =
            "void ExampleApp::requestResult() {\n    send();\n}\nauto demo::Widget::render() -> int {\n    return 42;\n}\n"_el;
        REQUIRE_EQUAL(
            impl::SourceSelectors{source}.select("f:ExampleApp::requestResult"_el, ""_el).join("\n"_el),
            "void ExampleApp::requestResult() {\n    send();\n}"_el);
        REQUIRE_EQUAL(
            impl::SourceSelectors{source}.select("f:demo::Widget::render"_el, ""_el).join("\n"_el),
            "auto demo::Widget::render() -> int {\n    return 42;\n}"_el);
    }

    void testHeaderScannerHandlesDocumentedDeclarationsAndForwardSignatures() {
        const auto source =
            "namespace erbsland::example {\n/// Value.\nclass Value {};\n/// Alias.\nusing Alias = Value;\n/// Function.\n[[nodiscard]] auto\nfunction(\n    int value) -> bool;\n}\n"_el;
        const auto entries = impl::HeaderScanner{}.scan(source).join("\n"_el);
        REQUIRE(entries.contains(".. doxygenclass:: erbsland::example::Value\n    :members:"_el));
        REQUIRE(entries.contains(".. doxygentypedef:: erbsland::example::Alias"_el));
        REQUIRE(entries.contains(".. doxygenfunction:: erbsland::example::function(int value) -> bool"_el));
    }

    void testCleanerPreservesCodeAndIsIdempotent() {
        auto symbols = impl::DoxygenIndex{};
        const auto cleaner = impl::RstCleaner{symbols, 40};
        auto warnings = el::StringList{};
        const auto source =
            "Title\n===\n\nA first sentence. Another sentence that wraps across the requested width.\n\n.. code-block:: cpp\n\n    `untouched`\n"_el;
        const auto result = cleaner.process(source, warnings);
        REQUIRE(result.startsWith("Title\n=====\n"_el));
        REQUIRE(result.contains("    `untouched`"_el));
        REQUIRE_EQUAL(cleaner.process(result, warnings), result);
    }

    void testCleanerProtectsNestedCodeAndAnsiBlocks() {
        const auto index = impl::DoxygenIndex{};
        const auto cleaner = impl::RstCleaner{index, 40};
        auto warnings = el::StringList{};
        const auto source =
            "* Example\n\n    .. code-block:: cpp\n\n        `unresolved`\n\n    .. erbsland-ansi::\n\n        `also-unchanged`\n\nAfter `unresolved`.\n"_el;
        const auto result = cleaner.process(source, warnings);
        REQUIRE(result.contains("        `unresolved`"_el));
        REQUIRE(result.contains("        `also-unchanged`"_el));
        REQUIRE(result.contains("After ``unresolved``."_el));
        REQUIRE_EQUAL(warnings.count(), el::ItemCount{1});
    }

    void testFingerprintFramesOrderedRawOptions() {
        const auto files = tool::frame::ProjectFiles{_root};
        auto first = el::StringMap<el::String>{};
        constexpr auto lastKey = "z"_el;
        constexpr auto firstKey = "a"_el;
        first.set(lastKey, "quote\" slash\\ newline\n 😀"_el);
        first.set(firstKey, "value"_el);
        auto second = el::StringMap<el::String>{};
        second.set("sha256"_el, "ignored"_el);
        second.set(firstKey, "value"_el);
        second.set(lastKey, "quote\" slash\\ newline\n 😀"_el);
        auto left = impl::DemoFingerprint{files};
        auto right = impl::DemoFingerprint{files};
        left.addOptions(first);
        right.addOptions(second);
        REQUIRE_EQUAL(left.finish(), right.finish());
        first.clear().set("ab"_el, "c"_el);
        second.clear().set(firstKey, "bc"_el);
        auto splitLeft = impl::DemoFingerprint{files};
        auto splitRight = impl::DemoFingerprint{files};
        splitLeft.addOptions(first);
        splitRight.addOptions(second);
        REQUIRE(splitLeft.finish() != splitRight.finish());
    }

    void testFingerprintTracksFileContentsNamesAndKinds() {
        write("source.txt"_el, "first"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto first = impl::DemoFingerprint{files};
        first.addFile("source"_el, _root / "source.txt"_el);
        auto otherKind = impl::DemoFingerprint{files};
        otherKind.addFile("fixture"_el, _root / "source.txt"_el);
        auto otherName = impl::DemoFingerprint{files};
        otherName.addFile("source"_el, _root / "source.txt"_el, "alias"_el);
        const auto original = first.finish();
        REQUIRE(original != otherKind.finish());
        REQUIRE(original != otherName.finish());
        write("source.txt"_el, "second"_el);
        auto changed = impl::DemoFingerprint{files};
        changed.addFile("source"_el, _root / "source.txt"_el);
        REQUIRE(original != changed.finish());
    }

    void testSourceOnlyDemoSynchronizationIsIdempotent() {
        write("demos/example/Sample.cpp"_el, "namespace demo {\n/// Example\nvoid sample() {}\n}\n"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto synchronizer = impl::DemoSynchronizer{files};
        auto issues = el::StringList{};
        const auto input = ".. erbsland-demo::\n    :source: example/Sample.cpp\n\n.. erbsland-demo-end::\n"_el;
        const auto result = synchronizer.process(_root / "page.rst"_el, input, false, issues);
        REQUIRE(issues.isEmpty());
        REQUIRE(result.contains(":sha256:"_el));
        REQUIRE(result.contains(".. code-block:: cpp"_el));
        REQUIRE_EQUAL(synchronizer.process(_root / "page.rst"_el, result, false, issues), result);
    }

    void testTemplateSourceOnlySynchronizationAndExecutionRejection() {
        const auto name = "event/templates/Startup/main.cpp"_el;
        write(el::join("demos/"_el, name), "/// Startup template\nauto main() -> int { return 0; }\n"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto synchronizer = impl::DemoSynchronizer{files};
        auto issues = el::StringList{};
        const auto document = _root / "page.rst"_el;
        const auto input = el::join(".. erbsland-demo::\n    :source: "_el, name, "\n\n.. erbsland-demo-end::\n"_el);
        const auto result = synchronizer.process(document, input, false, issues);
        REQUIRE(issues.isEmpty());
        REQUIRE(result.contains("auto main() -> int"_el));
        REQUIRE_EQUAL(synchronizer.process(document, result, false, issues), result);
        // Compute a matching fingerprint as if an older tool had cached an execution block.
        for (
            const auto key :
            {"exec"_el, "exec-2"_el, "exec-exit-code"_el, "exec-2-exit-code"_el, "files"_el, "show-cmd-line"_el}) {
            auto options = el::StringMap<el::String>{};
            options.set("source"_el, name);
            options.set(key, ""_el);
            auto fingerprint = impl::DemoFingerprint{files};
            fingerprint.addOptions(options);
            fingerprint.addFile("source"_el, _root / "demos"_el / name);
            const auto cached = el::join(
                ".. erbsland-demo::\n    :source: "_el,
                name,
                "\n    :"_el,
                key,
                ":\n    :sha256: "_el,
                fingerprint.finish(),
                "\n\nCached output must not be reused.\n\n.. erbsland-demo-end::\n"_el);
            REQUIRE_THROWS_AS(el::ApplicationError, synchronizer.process(document, cached, false, issues));
            REQUIRE_THROWS_AS(el::ApplicationError, synchronizer.process(document, cached, true, issues));
        }
    }

    void testLegacyMetadataMigrationDoesNotChangeCode() {
        const auto input =
            ".. erbsland-demo::\n    :function-blocks: first second\n    :source-sha256: obsolete\n\n.. code-block:: cpp\n\n    :function-blocks: untouched\n\n.. erbsland-demo-end::\n"_el;
        const auto result = impl::DemoSynchronizer::migrate(input);
        REQUIRE(result.contains(":show: f:first, f:second"_el));
        REQUIRE_FALSE(result.contains("source-sha256"_el));
        REQUIRE(result.contains(":function-blocks: untouched"_el));
    }

    void testDemoReplacementAndStableFingerprintLight() {
        write("demos/example/Sample.cpp"_el, "/// Sample\nvoid sample() { DemoTerminal::enable(app); }\n"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        const auto root = el::Path{"cmake-build-debug/demo-apps"_el};
        const auto rules = impl::DemoSynchronizer::TextReplacements{
            {el::RegEx::compile(R"(DemoTerminal::enable\(app\);)"_el), "app.enableTerminal();"_el}};
        auto synchronizer = impl::DemoSynchronizer{files, root, rules};
        auto issues = el::StringList{};
        const auto document = _root / "page.rst"_el;
        const auto result = synchronizer.process(
            document,
            ".. erbsland-demo::\n    :source: example/Sample.cpp\n\n.. erbsland-demo-end::\n"_el,
            false,
            issues);
        REQUIRE(result.contains("app.enableTerminal();"_el));
        REQUIRE_EQUAL(impl::DemoSynchronizer{files}.process(document, result, false, issues), result);
        REQUIRE(issues.isEmpty());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testDemoReplacementRulesAreOrderedAndPreserveFingerprints() {
        write("demos/example/Sample.cpp"_el, "/// Sample\nvoid sample() { demo::DemoTerminal::enable(app); }\n"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        const auto root = el::Path{"cmake-build-debug/demo-apps"_el};
        const auto pattern = el::RegEx::compile(R"(demo::DemoTerminal::enable\((?<app>\w+)\);)"_el);
        auto rules = impl::DemoSynchronizer::TextReplacements{
            {pattern, "{app}.enableTerminal();"_el},
            {el::RegEx::compile("enableTerminal"_el), "initializeTerminal"_el}};
        auto synchronizer = impl::DemoSynchronizer{files, root, rules};
        auto issues = el::StringList{};
        const auto input = ".. erbsland-demo::\n    :source: example/Sample.cpp\n\n.. erbsland-demo-end::\n"_el;
        const auto document = _root / "page.rst"_el;
        const auto result = synchronizer.process(document, input, false, issues);
        REQUIRE(issues.isEmpty());
        REQUIRE(result.contains("app.initializeTerminal();"_el));
        REQUIRE_EQUAL(synchronizer.process(document, result, false, issues), result);
        auto changed = impl::DemoSynchronizer{
            files, root, impl::DemoSynchronizer::TextReplacements{{pattern, "{app}.enableTerminal();"_el}}};
        REQUIRE_EQUAL(changed.process(document, result, false, issues), result);
        const auto updated = changed.process(document, result, true, issues);
        REQUIRE(updated != result);
        REQUIRE(updated.contains("app.enableTerminal();"_el));
        auto removed = impl::DemoSynchronizer{files};
        REQUIRE_EQUAL(removed.process(document, updated, false, issues), updated);
        REQUIRE(removed.process(document, updated, true, issues).contains("demo::DemoTerminal::enable(app);"_el));
        REQUIRE(issues.isEmpty());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testConfiguredDemoReplacementsOnlyChangeExtractedSource() {
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            R"([rendering]
[doc_process_rst]
*[.demo_text_replacement]
pattern: /(?:demo::)?DemoTerminal::enable\((?<app>\w+)\);/
replacement: "{app}.enableTerminal();"
*[.demo_text_replacement]
pattern: /(?:demo::)?DemoTerminal::enable\(\*this\);/
replacement: "enableTerminal();"
)"_el);
        const auto source =
            "void sample() {\n    demo::DemoTerminal::enable(app);\n    DemoTerminal::enable(other);\n    DemoTerminal::enable(*this);\n}\n"_el;
        write("demos/example/Sample.cpp"_el, source);
        write(
            "page.rst"_el,
            "Manual: demo::DemoTerminal::enable(app);\n\n.. erbsland-demo::\n    :source: example/Sample.cpp\n    :show: f:sample\n\n.. erbsland-demo-end::\n"_el);
        REQUIRE_EQUAL(
            runTool<ProcessRstTool>(
                {},
                {},
                "doc-process-rst"_el,
                el::StringList{"page.rst"_el, "--demos"_el, "sync"_el, "--no-cleanup"_el}),
            0);
        const auto result = (_root / "page.rst"_el).content().readTextOrThrow();
        REQUIRE(result.contains("Manual: demo::DemoTerminal::enable(app);"_el));
        REQUIRE(result.contains("    app.enableTerminal();"_el));
        REQUIRE(result.contains("    other.enableTerminal();"_el));
        REQUIRE(result.contains("    enableTerminal();"_el));
        REQUIRE_EQUAL((_root / "demos/example/Sample.cpp"_el).content().readTextOrThrow(), source);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testInvalidDemoReplacementLeavesDocumentsUntouched() {
        const auto original = "Title\n===\n"_el;
        write("page.rst"_el, original);
        for (
            const auto &rule :
            {"pattern: /[/\nreplacement: \"text\"\n"_el,
                "pattern: /word/\nreplacement: \"{missing}\"\n"_el,
                "pattern: /word/\n"_el}) {
            write(
                "tool/conf/erbsland-core-tool.elcl"_el,
                el::join("[rendering]\n[doc_process_rst]\n*[.demo_text_replacement]\n"_el, rule));
            REQUIRE(
                runTool<ProcessRstTool>(
                    {},
                    {},
                    "doc-process-rst"_el,
                    el::StringList{"page.rst"_el, "--demos"_el, "sync"_el, "--no-cleanup"_el}) != 0);
            REQUIRE_EQUAL((_root / "page.rst"_el).content().readTextOrThrow(), original);
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRstCheckAndPreviewDoNotWrite() {
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[doc_process_rst]\n"_el);
        write("index.xml"_el, "<doxygenindex/>"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[doc_process_rst]\ndoxygen_index: \"index.xml\"\n"_el);
        write("page.rst"_el, "Title\n===\n"_el);
        REQUIRE_EQUAL(
            runTool<ProcessRstTool>({}, {}, "doc-process-rst"_el, el::StringList{"page.rst"_el, "--dry-run"_el}), 0);
        REQUIRE_EQUAL((_root / "page.rst"_el).content().readTextOrThrow(), "Title\n===\n"_el);
        REQUIRE(
            runTool<ProcessRstTool>({}, {}, "doc-process-rst"_el, el::StringList{"page.rst"_el, "--check"_el}) != 0);
        REQUIRE_EQUAL(runTool<ProcessRstTool>({}, {}, "doc-process-rst"_el, el::StringList{"page.rst"_el}), 0);
        REQUIRE_EQUAL((_root / "page.rst"_el).content().readTextOrThrow(), "Title\n=====\n"_el);
    }
};
