// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/cleanup/CodeCleanupTool.hpp>

SKIP_BY_DEFAULT()
TAGS(FullRun)
TESTED_TARGETS(tool::cleanup::CodeCleanupTool tool::cleanup::ClangFormat tool::cleanup::CodeCleanup)
class CodeCleanupToolTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testGlobSelectionExcludesSiblingTemplatesAndCheckDoesNotWrite() {
        prepare();
        const auto configurationPath = "tool/conf/erbsland-core-tool.elcl"_el;
        const auto configuration = read(configurationPath);
        write(
            configurationPath,
            el::join(configuration, "[code_cleanup.sources.apps]\ndirectories: \"apps/*/src\"\n"_el));
        const auto input = "// Header\nint value=1;\n"_el;
        write("apps/a/src/Main.cpp"_el, input);
        write("apps/b/src/Main.cpp"_el, input);
        write("apps/a/data/template.cpp"_el, "// FORMAT_FAILURE\n"_el);
        REQUIRE_NOT_EQUAL(runCleanup(el::StringList{"--check"_el}), 0);
        REQUIRE_EQUAL(read("apps/a/src/Main.cpp"_el), input);
        REQUIRE_FALSE((_root / ".cache/code_cleanup/fingerprints.txt"_el).info().exists());
        REQUIRE_EQUAL(runCleanup(), 0);
        REQUIRE(_outputText.contains("clang-format processed 4"_el));
        REQUIRE(read("apps/a/src/Main.cpp"_el).contains("int value = 1;"_el));
        REQUIRE_EQUAL(read("apps/a/data/template.cpp"_el), "// FORMAT_FAILURE\n"_el);
    }

    void testFullBaselineThenCachedAndIncludeOnlyRuns() {
        prepare();
        REQUIRE_EQUAL(runCleanup(), 0);
        REQUIRE(_outputText.contains("clang-format processed 2"_el));
        REQUIRE(read("src/erbsland/Widget.cpp"_el).contains("int value = 1;"_el));
        const auto first = read("src/erbsland/Widget.cpp"_el);
        REQUIRE_EQUAL(runCleanup(), 0);
        REQUIRE(_outputText.contains("clang-format processed 0"_el));
        write(
            "src/erbsland/Widget.cpp"_el,
            first.replacedAll("#include \"Widget.hpp\""_el, "#include \"src/erbsland/Widget.hpp\""_el));
        REQUIRE_EQUAL(runCleanup(), 0);
        REQUIRE(_outputText.contains("clang-format processed 0"_el));
        REQUIRE_EQUAL(read("src/erbsland/Widget.cpp"_el), first);
        write("src/erbsland/Widget.cpp"_el, first.replacedAll("value = 1"_el, "value = 2"_el));
        REQUIRE_EQUAL(runCleanup(), 0);
        REQUIRE(_outputText.contains("clang-format processed 1"_el));
    }

    void testCheckAndFailureDoNotWriteSourcesOrCache() {
        prepare();
        const auto original = read("src/erbsland/Widget.cpp"_el);
        REQUIRE_NOT_EQUAL(runCleanup(el::StringList{"--check"_el}), 0);
        REQUIRE_EQUAL(read("src/erbsland/Widget.cpp"_el), original);
        REQUIRE(!(_root / ".cache/code_cleanup/fingerprints.txt"_el).info().exists());
        write("src/erbsland/Widget.hpp"_el, "// Header\n#pragma once\n// FORMAT_EMPTY_OUTPUT\n"_el);
        REQUIRE_NOT_EQUAL(runCleanup(), 0);
        REQUIRE_EQUAL(read("src/erbsland/Widget.cpp"_el), original);
        REQUIRE(!(_root / ".cache/code_cleanup/fingerprints.txt"_el).info().exists());
        write("src/erbsland/Widget.hpp"_el, "// Header\n#pragma once\n// FORMAT_FAILURE\n"_el);
        REQUIRE_NOT_EQUAL(runCleanup(), 0);
        REQUIRE_EQUAL(read("src/erbsland/Widget.cpp"_el), original);
        REQUIRE(!(_root / ".cache/code_cleanup/fingerprints.txt"_el).info().exists());
    }

    void testSerialParallelAndStyleInvalidation() {
        prepare();
        REQUIRE_EQUAL(runCleanup(el::StringList{"--jobs"_el, "1"_el}), 0);
        const auto first = read("src/erbsland/Widget.cpp"_el);
        const auto cache = read(".cache/code_cleanup/fingerprints.txt"_el);
        REQUIRE_EQUAL(runCleanup(el::StringList{"--jobs"_el, "4"_el, "--full-format"_el}), 0);
        REQUIRE_EQUAL(read("src/erbsland/Widget.cpp"_el), first);
        REQUIRE_EQUAL(read(".cache/code_cleanup/fingerprints.txt"_el), cache);
        write(".clang-format"_el, "BasedOnStyle: LLVM\nIndentWidth: 4\n"_el);
        REQUIRE_EQUAL(runCleanup(), 0);
        REQUIRE(_outputText.contains("clang-format processed 2"_el));
    }

    void testFormatterBomDoesNotInvalidateBaseline() {
        prepare();
        write("src/erbsland/Widget.hpp"_el, "// Header\n#pragma once\n\n// FORMAT_BOM\n"_el);
        REQUIRE_EQUAL(runCleanup(), 0);
        REQUIRE_EQUAL(runCleanup(), 0);
        REQUIRE(_outputText.contains("clang-format processed 0"_el));
        REQUIRE(!read("src/erbsland/Widget.hpp"_el).startsWith("\uFEFF"_el));
    }

    void testCacheLineEndingsAndInvalidRecords() {
        prepare();
        REQUIRE_EQUAL(runCleanup(), 0);
        const auto cachePath = ".cache/code_cleanup/fingerprints.txt"_el;
        const auto cache = read(cachePath);
        write(cachePath, cache.replacedAll("\n"_el, "\r\n"_el));
        REQUIRE_EQUAL(runCleanup(), 0);
        REQUIRE(_outputText.contains("clang-format processed 0"_el));
        write(cachePath, el::join(cache, "invalid-record\n"_el));
        REQUIRE_EQUAL(runCleanup(), 0);
        REQUIRE(_outputText.contains("clang-format processed 2"_el));
    }

    void testInvalidConfigurationAndJobs() {
        prepare();
        REQUIRE_NOT_EQUAL(runCleanup(el::StringList{"--jobs"_el, "0"_el}), 0);
        const auto configuration = read("tool/conf/erbsland-core-tool.elcl"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            configuration.replacedAll("directories: \"src/erbsland\""_el, "directories: \"../outside\""_el));
        REQUIRE_NOT_EQUAL(runCleanup(), 0);
    }

    void testPublicIncludePrefixValidation() {
        prepare();
        const auto configuration = read("tool/conf/erbsland-core-tool.elcl"_el);
        for (const auto &prefix : {""_el, "/absolute"_el, "../outside"_el, "vendor/./core"_el, "vendor//core"_el}) {
            write(
                "tool/conf/erbsland-core-tool.elcl"_el,
                configuration.replacedAll(
                    "[code_cleanup]\n"_el,
                    el::join("[code_cleanup]\npublic_include_prefix: \""_el, prefix, "\"\n"_el)));
            REQUIRE_NOT_EQUAL(runCleanup(), 0);
        }
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            configuration.replacedAll(
                "[code_cleanup]\n"_el, "[code_cleanup]\npublic_include_prefix: \"vendor/core\"\n"_el));
        REQUIRE_EQUAL(runCleanup(), 0);
    }

private:
    void prepare() {
        write("src/erbsland/Widget.hpp"_el, "// Header\n\n#pragma once\n"_el);
        write(
            "src/erbsland/Widget.cpp"_el, "// Header\n#include \"Widget.hpp\"\nnamespace demo {\nint value=1;\n}\n"_el);
        write("tool/data/public_headers/header_map.txt"_el, "Widget.hpp;Widget.hpp\n"_el);
        write(".clang-format"_el, "BasedOnStyle: LLVM\n"_el);
        const auto executable =
            el::Path{std::filesystem::path{ERBSLAND_CORE_TOOL_FAKE_CLANG_FORMAT}}.toString().replacedAll(
                "\\"_el, "\\\\"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            el::join(
                "[rendering]\n[code_cleanup]\nfile_suffixes: \".hpp\", \".cpp\"\n"
                "[code_cleanup.sources.library]\ndirectories: \"src/erbsland\"\ninclude_style: \"relative\"\n"
                "[code_cleanup.cmake]\n[code_cleanup.cmake.trees]\n"
                "[code_cleanup.format]\nexecutable: \""_el,
                executable,
                "\"\n"_el));
    }

    auto read(const el::String &name) const -> el::String { return (_root / name).content().readTextOrThrow(); }
    auto runCleanup(const el::StringList &options = {}) -> int {
        return runTool<tool::cleanup::CodeCleanupTool>({}, {}, "code-cleanup"_el, options);
    }
};
