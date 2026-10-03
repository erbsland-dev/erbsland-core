// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/cleanup/CleanupPolicy.hpp>
#include <tool/cleanup/CodeCleanupTool.hpp>
#include <tool/cleanup/IncludeIndex.hpp>

#include <filesystem>

TESTED_TARGETS(tool::cleanup::CleanupPolicy tool::cleanup::IncludeIndex)
class CodeCleanupSourceGlobTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public: // tests
    void testDirectoryGlobsAreSortedAndExcludeSiblingTemplates() {
        prepare();
        write("apps/z/src/Main.cpp"_el, ""_el);
        write("apps/a/src/Main.cpp"_el, ""_el);
        write("apps/a/data/template.cpp"_el, ""_el);
        write("apps/a/resource/template.hpp"_el, ""_el);
        const auto policy = makePolicy("\"apps/*/src\""_el);
        REQUIRE_EQUAL(policy.trees.size(), std::size_t{2});
        REQUIRE_EQUAL(policy.trees[0].directory, _root / "apps/a/src"_el);
        REQUIRE_EQUAL(policy.trees[1].directory, _root / "apps/z/src"_el);
        REQUIRE_EQUAL(policy.trees[0].includeRoots.count(), el::ItemCount{1});
        REQUIRE_EQUAL(policy.trees[0].includeRoots.first(), policy.trees[0].directory);
        REQUIRE_EQUAL(policy.trees[1].includeRoots.first(), policy.trees[1].directory);
    }

    void testDuplicateAndNestedSelectionsCollapseWithinOnePolicy() {
        prepare();
        write("apps/a/src/Main.cpp"_el, ""_el);
        write("apps/a/src/nested/src/Helper.hpp"_el, ""_el);
        write("apps/b/src/Main.cpp"_el, ""_el);
        const auto policy = makePolicy("\"apps/**/src\", \"apps/a/src\", \"apps/a/src\""_el);
        REQUIRE_EQUAL(policy.trees.size(), std::size_t{2});
        REQUIRE_EQUAL(policy.trees[0].directory, _root / "apps/a/src"_el);
        REQUIRE_EQUAL(policy.trees[1].directory, _root / "apps/b/src"_el);
    }

    void testOverlappingDifferentPoliciesAreRejected() {
        prepare();
        write("apps/a/src/Main.cpp"_el, ""_el);
        for (const auto &other : {"apps/a/src"_el, "apps/a"_el}) {
            _failureDiagnostic = other;
            REQUIRE_THROWS_AS(
                el::conf::ConfError,
                makePolicy(
                    "\"apps/*/src\""_el,
                    el::join("[code_cleanup.sources.other]\ndirectories: \""_el, other, "\"\n"_el)));
        }
    }

    void testUnsafeInvalidAndUnmatchedPatternsAreRejected() {
        prepare();
        write("apps/a/src/Main.cpp"_el, ""_el);
        for (
            const auto &patterns : el::StringList{
                "\"\""_el,
                "\"   \""_el,
                "\"/apps\""_el,
                "\"../apps\""_el,
                "\"apps/../apps\""_el,
                "\"apps/[\""_el,
                "\"missing/*/src\""_el,
                "\"apps/a/src/*.cpp\""_el,
                "\"apps/*/src\", \"missing\""_el}) {
            _failureDiagnostic = patterns;
            REQUIRE_THROWS_AS(el::conf::ConfError, makePolicy(patterns));
        }
    }

    void testReconfigurationResolvesNewDirectoriesWithoutRetainingOldTrees() {
        prepare();
        write("apps/a/src/Main.cpp"_el, ""_el);
        auto policy = makePolicy("\"apps/*/src\""_el);
        REQUIRE_EQUAL(policy.trees.size(), std::size_t{1});
        write("apps/b/src/Main.cpp"_el, ""_el);
        policy.read(configuration("\"apps/*/src\""_el), files());
        REQUIRE_EQUAL(policy.trees.size(), std::size_t{2});
        policy.read(configuration("\"apps/b/src\""_el), files());
        REQUIRE_EQUAL(policy.trees.size(), std::size_t{1});
        REQUIRE_EQUAL(policy.trees[0].directory, _root / "apps/b/src"_el);
    }

    void testExplicitIncludeRootsRecognizeRelativeHeadersWithoutLeakingAcrossPolicies() {
        prepare();
        write("apps/a/src/Main.cpp"_el, ""_el);
        write("apps/b/src/Main.cpp"_el, ""_el);
        write("shared/src/Shared.hpp"_el, ""_el);
        const auto policy = makePolicy(
            "\"apps/a/src\""_el,
            "include_roots: \"shared/src\"\n[code_cleanup.sources.other]\ndirectories: \"apps/b/src\"\n"_el);
        auto index = tool::cleanup::IncludeIndex{policy, _root};
        index.add(_root / "shared/src/Shared.hpp"_el);
        auto global = false;
        REQUIRE_EQUAL(
            index.resolve(
                _root / "apps/a/src/Main.cpp"_el, "../../../shared/src/Shared.hpp"_el, global, policy.trees[0]),
            "../../../shared/src/Shared.hpp"_el);
        REQUIRE_FALSE(global);
        REQUIRE_EQUAL(
            index.resolve(_root / "apps/a/src/Main.cpp"_el, "Shared.hpp"_el, global, policy.trees[0]), "Shared.hpp"_el);
        REQUIRE_THROWS_AS(
            el::ApplicationError,
            index.resolve(_root / "apps/b/src/Main.cpp"_el, "Shared.hpp"_el, global, policy.trees[1]));
        REQUIRE_THROWS_AS(
            el::ApplicationError,
            index.resolve(
                _root / "apps/b/src/Main.cpp"_el, "../../../shared/src/Shared.hpp"_el, global, policy.trees[1]));
    }

    void testLinkedDirectoriesAreSkipped() {
        prepare();
        write("apps/real/src/Main.cpp"_el, ""_el);
        auto error = std::error_code{};
        std::filesystem::create_directory_symlink(
            (_root / "apps/real"_el).toStdPath(), (_root / "apps/alias"_el).toStdPath(), error);
        if (error) {
            return; // Windows may lack symbolic-link privileges.
        }
        const auto policy = makePolicy("\"apps/*/src\""_el);
        REQUIRE_EQUAL(policy.trees.size(), std::size_t{1});
        REQUIRE_EQUAL(policy.trees[0].directory, _root / "apps/real/src"_el);
        REQUIRE_THROWS_AS(el::conf::ConfError, makePolicy("\"apps/alias/src\""_el));
    }

private:
    void prepare() {
        write("src/erbsland/Widget.hpp"_el, ""_el);
        write("tool/data/public_headers/header_map.txt"_el, "Widget.hpp;Widget.hpp\n"_el);
        write(".clang-format"_el, "BasedOnStyle: LLVM\n"_el);
    }

    auto configuration(const el::String &patterns, const el::String &extra = {}) -> el::conf::ValuePtr {
        const auto parsed = el::conf::Parser{}.parseTextOrThrow(
            el::join(
                "[code_cleanup]\nfile_suffixes: \".cpp\", \".hpp\"\n"
                "[code_cleanup.sources.selected]\ndirectories: "_el,
                patterns,
                "\n"_el,
                extra,
                "[code_cleanup.cmake.trees]\n[code_cleanup.format]\n"_el));
        const auto section = parsed->valueOrThrow("code_cleanup"_el);
        tool::cleanup::CodeCleanupTool{}.configurationRules()->validate(section, 1);
        return section;
    }

    auto makePolicy(const el::String &patterns, const el::String &extra = {}) -> tool::cleanup::CleanupPolicy {
        auto policy = tool::cleanup::CleanupPolicy{};
        policy.read(configuration(patterns, extra), files());
        return policy;
    }
};
