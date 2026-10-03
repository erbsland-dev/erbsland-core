// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/anti_patterns/AntiPatternsTool.hpp>
#include <tool/anti_patterns/FindingCache.hpp>
#include <tool/anti_patterns/Policy.hpp>
#include <tool/anti_patterns/RuleRegistry.hpp>
#include <tool/anti_patterns/Scanner.hpp>

#include <filesystem>

TESTED_TARGETS(Policy Scanner)
class AntiPatternsSourceGlobTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public: // tests
    void testDirectoryPatternsExcludeSiblingTemplates() {
        for (
            const auto &path : el::StringList{
                "cmake/tools/compiler/src/Main.cpp"_el,
                "cmake/tools/compiler/src/impl/Helper.hpp"_el,
                "test/unittest/src/Example.cpp"_el,
                "test/unittest/src/data/Example.cpp"_el,
                "test/unittest/src/resource/Example.cpp"_el,
                "test/profiling/first/src/Main.cpp"_el,
                "test/profiling/second/src/Main.cpp"_el,
                "test/profiling/framework/test/src/Main.cpp"_el,
                "test/profiling/path/test/src/Main.cpp"_el,
                "test/fuzzing/http/src/Main.cpp"_el}) {
            write(path, "namespace { }\n"_el);
        }
        for (
            const auto &path : el::StringList{
                "cmake/tools/compiler/data/template.cpp"_el,
                "cmake/tools/compiler/resource/template.hpp"_el,
                "test/unittest/data/template.cpp"_el,
                "test/profiling/first/data/template.cpp"_el,
                "test/profiling/first/ressource/template.hpp"_el,
                "test/fuzzing/http/corpus/template.cpp"_el}) {
            write(path, "namespace { }\n"_el);
        }
        write("test/profiling/first/src/input.txt"_el, "namespace { }\n"_el);
        const auto policy = makePolicy(
            "\"cmake/tools/*/src\", \"test/unittest/src\", \"test/profiling/*/src\", "
            "\"test/profiling/*/test/src\", \"test/fuzzing/*/src\""_el);
        const auto registry = tool::anti_patterns::RuleRegistry{};
        const auto service = files();
        auto scanner = tool::anti_patterns::Scanner{policy, registry, service};
        const auto findings = scanner.scan({}, 4);
        REQUIRE_EQUAL(scanner.fileCount(), el::ItemCount{10});
        REQUIRE_EQUAL(findings.count(), el::ItemCount{10});
        for (const auto &finding : findings) {
            REQUIRE_FALSE(finding.path.toString().contains("template"_el));
        }
        auto explicitFile = tool::anti_patterns::Scanner{policy, registry, service};
        REQUIRE_EQUAL(
            explicitFile.scan(el::StringList{"cmake/tools/compiler/data/template.cpp"_el}, 1).count(),
            el::ItemCount{1});
    }

    void testRecursiveOverlappingPatternsAndFreshResolution() {
        write("test/profiling/a/src/A.cpp"_el, "namespace { }\n"_el);
        write("test/profiling/a/test/src/B.cpp"_el, "namespace { }\n"_el);
        auto policy = makePolicy("\"test/profiling/**/src\", \"test/profiling/a/src\", \"test/profiling/a/src\""_el);
        const auto registry = tool::anti_patterns::RuleRegistry{};
        const auto service = files();
        auto first = tool::anti_patterns::Scanner{policy, registry, service};
        const auto expected = first.scan({}, 1);
        REQUIRE_EQUAL(first.fileCount(), el::ItemCount{2});
        REQUIRE_EQUAL(expected.count(), el::ItemCount{2});
        auto second = tool::anti_patterns::Scanner{policy, registry, service};
        const auto actual = second.scan({}, 4);
        REQUIRE_EQUAL(second.cacheHits(), el::ItemCount{2});
        REQUIRE_EQUAL(actual.first().path, expected.first().path);
        REQUIRE_EQUAL(actual.last().path, expected.last().path);
        write("test/profiling/new/src/C.cpp"_el, "namespace { }\n"_el);
        auto fresh = tool::anti_patterns::Scanner{policy, registry, service};
        REQUIRE_EQUAL(fresh.scan({}, 1).count(), el::ItemCount{3});
        REQUIRE_EQUAL(fresh.cacheHits(), el::ItemCount{2});
        REQUIRE_EQUAL(fresh.cacheMisses(), el::ItemCount{1});
        policy = makePolicy("\"test/profiling/new/src\""_el);
        auto narrowed = tool::anti_patterns::Scanner{policy, registry, service};
        REQUIRE_EQUAL(narrowed.scan({}, 1).count(), el::ItemCount{1});
        REQUIRE_EQUAL(narrowed.cacheHits(), el::ItemCount{1});
        const auto cache = tool::anti_patterns::FindingCache{service, registry};
        REQUIRE(
            cache.get(
                el::Path{"test/profiling/a/src/A.cpp"_el},
                tool::anti_patterns::FindingCache::signature(_root / "test/profiling/a/src/A.cpp"_el)) == nullptr);
    }

    void testUnmatchedAndUnsafePatterns() {
        write("src/A.cpp"_el, "namespace { }\n"_el);
        const auto service = files();
        const auto registry = tool::anti_patterns::RuleRegistry{};
        for (
            const auto &pattern : el::StringList{
                "\"\""_el,
                "\"   \""_el,
                "\"../src\""_el,
                "\"src/../src\""_el,
                "\"/src\""_el,
                "\"src/[\""_el,
                "\"missing/*/src\""_el,
                "\"src/*.cpp\""_el,
                "\"src\", \"missing\""_el}) {
            _failureDiagnostic = pattern;
            REQUIRE_THROWS([&]() -> void {
                const auto policy = makePolicy(pattern);
                auto scanner = tool::anti_patterns::Scanner{policy, registry, service};
                const auto result = scanner.scan({}, 1);
                (void)result;
            }());
        }
        const auto policy = makePolicy("\"missing/*/src\""_el);
        auto explicitTarget = tool::anti_patterns::Scanner{policy, registry, service};
        REQUIRE_EQUAL(explicitTarget.scan(el::StringList{"src/A.cpp"_el}, 1).count(), el::ItemCount{1});
    }

    void testLinkedDirectoriesAndFilesAreSkipped() {
        write("tree/real/src/A.cpp"_el, "namespace { }\n"_el);
        write("outside/B.cpp"_el, "namespace { }\n"_el);
        auto error = std::error_code{};
        std::filesystem::create_directory_symlink(
            (_root / "tree/real"_el).toStdPath(), (_root / "tree/alias"_el).toStdPath(), error);
        if (error) {
            return; // Windows may lack symbolic-link privileges.
        }
        std::filesystem::create_symlink(
            (_root / "outside/B.cpp"_el).toStdPath(), (_root / "tree/real/src/linked.cpp"_el).toStdPath());
        const auto policy = makePolicy("\"tree/*/src\""_el);
        const auto registry = tool::anti_patterns::RuleRegistry{};
        const auto service = files();
        auto scanner = tool::anti_patterns::Scanner{policy, registry, service};
        REQUIRE_EQUAL(scanner.scan({}, 1).count(), el::ItemCount{1});
        REQUIRE_EQUAL(scanner.fileCount(), el::ItemCount{1});
        const auto linked = makePolicy("\"tree/alias/src\""_el);
        REQUIRE_THROWS(linked.roots(service));
    }

private:
    [[nodiscard]] auto makePolicy(const el::String &directories) -> tool::anti_patterns::Policy {
        const auto parsed = el::conf::Parser{}.parseTextOrThrow(
            el::join(
                "[anti_patterns]\nSource Directories: "_el, directories, "\nFile Suffixes: \".cpp\", \".hpp\"\n"_el));
        const auto section = parsed->valueOrThrow("anti_patterns"_el);
        tool::anti_patterns::AntiPatternsTool{}.configurationRules()->validate(section, 1);
        auto policy = tool::anti_patterns::Policy{};
        const auto registry = tool::anti_patterns::RuleRegistry{};
        policy.read(section, files(), registry);
        return policy;
    }
};
