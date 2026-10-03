// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathGlobPattern.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringPattern.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <vector>

using namespace el::text::literals;
TESTED_TARGETS(PathGlobPattern PathGlobPatternFlag PathGlobPatternFlags PathGlobMatchMode)
class PathGlobPatternTest final : public el::UnitTest {
    using Glob = el::path::PathGlobPattern;
    using Flag = el::path::PathGlobPatternFlag;
    using Flags = el::path::PathGlobPatternFlags;
    using Mode = el::path::PathGlobMatchMode;

public:
    void testFullSuffixAndRoots() {
        const auto pattern = Glob::compile("*.cpp"_el);
        REQUIRE(pattern->fullMatch("main.cpp"_el));
        REQUIRE_FALSE(pattern->fullMatch("src/main.cpp"_el));
        REQUIRE(pattern->suffixMatch("src/main.cpp"_el));
        REQUIRE_FALSE(pattern->suffixMatch("main.cpp/child"_el));
        REQUIRE(pattern->fullMatch("/main.cpp"_el));
        REQUIRE(pattern->fullMatch(el::Path::fromWindows("C:/main.cpp"_el)));
        REQUIRE(pattern->fullMatch(el::Path::fromWindows("//server/share/main.cpp"_el)));
        REQUIRE_FALSE(pattern->fullMatch(el::Path{}));
        REQUIRE_FALSE(pattern->fullMatch(""_el));
        REQUIRE_FALSE(pattern->fullMatch("bad\0.cpp"_el));
        for (const auto mode : {Mode::Full, Mode::Suffix}) {
            for (const auto &text : {"main.cpp"_el, "src/main.cpp"_el, "/"_el}) {
                REQUIRE_EQUAL(pattern->match(text, mode), pattern->match(el::Path{text}, mode));
                REQUIRE_EQUAL(
                    pattern->match(text, mode),
                    mode == Mode::Full ? pattern->fullMatch(text) : pattern->suffixMatch(text));
                REQUIRE_EQUAL(
                    pattern->mayMatchDescendant(text, mode), pattern->mayMatchDescendant(el::Path{text}, mode));
            }
        }
        const auto anchored = Glob::compile("/*.cpp"_el);
        REQUIRE(anchored->suffixMatch("main.cpp"_el));
        REQUIRE_FALSE(anchored->suffixMatch("src/main.cpp"_el));
        REQUIRE(Glob::compile("C:/src/*.cpp"_el)->fullMatch("D:/src/a.cpp"_el));
        REQUIRE(Glob::compile("//server/share/src/*.cpp"_el)->fullMatch("src/a.cpp"_el));
        REQUIRE_FALSE(Glob::compile("C:/src/*.cpp"_el)->suffixMatch("else/src/a.cpp"_el));
        REQUIRE(Glob::compile("/"_el)->fullMatch("c:/"_el));
        REQUIRE_FALSE(Glob::compile("/"_el)->mayMatchDescendant("/"_el));
        REQUIRE_THROWS_AS(el::ParameterFailure, pattern->match("a.cpp"_el, static_cast<Mode>(99)));
    }
    void testComponentSyntaxAndUnicode() {
        REQUIRE(Glob::compile("a/**/b"_el)->fullMatch("a/b"_el));
        REQUIRE(Glob::compile("a/**/b"_el)->fullMatch("a/x/y/b"_el));
        REQUIRE(Glob::compile("a/**"_el)->fullMatch("a"_el));
        REQUIRE(Glob::compile("**/**/b"_el)->fullMatch("b"_el));
        REQUIRE(Glob::compile("**/foo/**"_el)->fullMatch("a/foo/b"_el));
        REQUIRE_FALSE(Glob::compile("foo"_el)->suffixMatch("a/foo/b"_el));
        REQUIRE(Glob::compile("*"_el)->fullMatch(".hidden"_el));
        REQUIRE(Glob::compile("?/[ä-ö][^0-9].cpp"_el)->fullMatch("☃/ö雪.cpp"_el));
        REQUIRE_FALSE(Glob::compile("?/[ä-ö][^0-9].cpp"_el)->fullMatch("☃/ö2.cpp"_el));
        REQUIRE(Glob::compile("a*b*c"_el)->fullMatch("abbc"_el));
        REQUIRE_FALSE(Glob::compile("ab*bc"_el)->fullMatch("abc"_el));
        REQUIRE(Glob::compile(R"(literal\*\?\[\])"_el)->fullMatch("literal*?[]"_el));
        REQUIRE(Glob::compile(R"(a\\b)"_el)->fullMatch(el::Path::fromPosix(R"(a\b)"_el)));
        REQUIRE_FALSE(Glob::compile(R"(a\\b)"_el)->fullMatch(R"(a\b)"_el));
        REQUIRE(Glob::compile(u"☃/**/*.cpp"_el)->fullMatch("☃/a.cpp"_el));
        REQUIRE(Glob::compile(U"☃/**/*.cpp"_el)->fullMatch("☃/a.cpp"_el));
        REQUIRE_FALSE(Glob::compile("a?b"_el)->fullMatch("a/b"_el));
        REQUIRE_FALSE(Glob::compile("a[^]b"_el)->fullMatch("a/b"_el));
        REQUIRE(Glob::compile("a//b"_el)->fullMatch("a/b"_el));
    }
    void testFlagsListsAndTrimming() {
        REQUIRE(Glob::compile(" \t **/*.cpp \t"_el)->fullMatch("a.cpp"_el));
        REQUIRE(Glob::compile("a / b "_el)->fullMatch("a / b"_el));
        REQUIRE_FALSE(Glob::compile("a / b "_el)->fullMatch("a/b"_el));
        REQUIRE(Glob::compile(" a "_el, Flags{})->fullMatch(" a "_el));
        REQUIRE_FALSE(Glob::compile(" a "_el, Flags{})->fullMatch("a"_el));
        REQUIRE(Glob::compile("[ ]a[\t]"_el)->fullMatch(" a\t"_el));
        const auto lists = Flags{Flag::Default, Flag::MultiplePatterns, Flag::SemicolonSeparator};
        const auto pattern = Glob::compile(" *.cpp ; \n\t*.hpp\r\n;"_el, lists);
        REQUIRE(pattern->fullMatch("a.cpp"_el));
        REQUIRE(pattern->fullMatch("a.hpp"_el));
        REQUIRE_FALSE(pattern->fullMatch("a.txt"_el));
        REQUIRE(Glob::compile("[;]"_el)->fullMatch(";"_el));
        REQUIRE(Glob::compile(R"([\;\#])"_el)->fullMatch("#"_el));
        REQUIRE(Glob::compile(R"(a\;b)"_el)->fullMatch("a;b"_el));
        REQUIRE(Glob::compile("a[\n]b"_el)->fullMatch("a\nb"_el));
        REQUIRE_THROWS_AS(el::ParseError, Glob::compile("a;b"_el));
        REQUIRE_THROWS_AS(el::ParseError, Glob::compile("a\nb"_el));
        REQUIRE_THROWS_AS(el::ParseError, Glob::compile("a;b"_el, Flags{Flag::MultiplePatterns}));
        REQUIRE_THROWS_AS(el::ParameterFailure, Glob::compile("a"_el, Flags{Flag::SemicolonSeparator}));
        REQUIRE_THROWS_AS(el::ParseError, Glob::compile("**/a"_el, Flags{Flag::TrimWhitespace}));
        REQUIRE(Glob::compile(R"(\*\*)"_el, Flags{})->fullMatch("**"_el));
        const auto untrimmed = Glob::compile(" a ; b "_el, Flags{Flag::MultiplePatterns, Flag::SemicolonSeparator});
        REQUIRE(untrimmed->fullMatch(" a "_el));
        REQUIRE(untrimmed->fullMatch(" b "_el));
        REQUIRE_FALSE(untrimmed->fullMatch("a"_el));
        REQUIRE_THROWS_AS(el::ParseError, Glob::compile("; \t\r\n"_el, lists));
        REQUIRE_THROWS_AS(el::ParseError, Glob::compile("a\rb"_el, lists));
    }
    void testComments() {
        const auto flags = Flags{Flag::Default, Flag::MultiplePatterns, Flag::IgnoreComments, Flag::SemicolonSeparator};
        const auto pattern =
            Glob::compile("# ignored [bad; ***\r\n  # ignored\n *.cpp ;#literal\n\\#escaped\nfile#name\n"_el, flags);
        REQUIRE(pattern->fullMatch("a.cpp"_el));
        REQUIRE(pattern->fullMatch("#literal"_el));
        REQUIRE(pattern->fullMatch("#escaped"_el));
        REQUIRE(pattern->fullMatch("file#name"_el));
        REQUIRE_FALSE(pattern->fullMatch("ignored"_el));
        REQUIRE(Glob::compile(" #literal"_el, Flags{Flag::IgnoreComments})->fullMatch(" #literal"_el));
        REQUIRE(Glob::compile("#literal"_el)->fullMatch("#literal"_el));
        REQUIRE(Glob::compile("# comment\nfile"_el, Flags{Flag::IgnoreComments})->fullMatch("file"_el));
        REQUIRE(Glob::compile("# comment\r\nfile"_el, Flags{Flag::IgnoreComments})->fullMatch("file"_el));
        REQUIRE_THROWS_AS(el::ParseError, Glob::compile("# comment\r\n"_el, flags));
        REQUIRE_THROWS_AS(el::ParseError, Glob::compile(" # comment\n\t"_el, flags));
    }
    void testInvalidSyntaxAndLocations() {
        for (
            const auto &text :
            {""_el,
                " \t"_el,
                "a**"_el,
                "**a"_el,
                "***"_el,
                "a/***/b"_el,
                "a/"_el,
                "a/  "_el,
                "bad\0*"_el,
                "C:/"_el,
                "//server/share/"_el,
                "a["_el,
                "a]"_el,
                "[]"_el,
                "[z-a]"_el,
                "\\q"_el,
                "a\\"_el,
                "[a-z]:/file"_el,
                "*:/file"_el,
                "//*/share/file"_el,
                "//server/*/file"_el}) {
            REQUIRE_THROWS_AS(el::ParseError, Glob::compile(text));
        }
        try {
            const auto unexpected =
                Glob::compile("ok; bad["_el, Flags{Flag::Default, Flag::MultiplePatterns, Flag::SemicolonSeparator});
            REQUIRE(false);
        } catch (const el::ParseError &error) {
            REQUIRE(error.hasPosition());
            REQUIRE(error.codePointIndex() >= el::CpIndex{7U});
        }
    }
    void testAlternativeStateIsolation() {
        const auto flags = Flags{Flag::Default, Flag::MultiplePatterns, Flag::SemicolonSeparator};
        const auto pattern = Glob::compile("missing/**/deep/**/x; a/**/b; **/c; fixed"_el, flags);
        REQUIRE(pattern->fullMatch("a/b"_el));
        REQUIRE(pattern->fullMatch("a/deep/b"_el));
        REQUIRE(pattern->fullMatch("c"_el));
        REQUIRE(pattern->fullMatch("deep/c"_el));
        REQUIRE(pattern->fullMatch("fixed"_el));
        REQUIRE_FALSE(pattern->fullMatch("a/deep/x"_el));
        REQUIRE_FALSE(pattern->fullMatch("other/fixed"_el));
        REQUIRE(pattern->suffixMatch("other/a/b"_el));
        REQUIRE(pattern->suffixMatch("other/fixed"_el));
    }

    void testPruningAndBoundedCandidates() {
        const auto fixed = Glob::compile("src/*.cpp"_el);
        REQUIRE(fixed->mayMatchDescendant("/"_el));
        REQUIRE(fixed->mayMatchDescendant("src"_el));
        REQUIRE_FALSE(fixed->mayMatchDescendant("doc"_el));
        REQUIRE_FALSE(fixed->mayMatchDescendant("src/a.cpp"_el));
        REQUIRE(fixed->mayMatchDescendant("doc"_el, Mode::Suffix));
        REQUIRE_FALSE(Glob::compile("/src/*.cpp"_el)->mayMatchDescendant("doc"_el, Mode::Suffix));
        const auto patterns =
            el::StringList{"*"_el, "**"_el, "a/*"_el, "a/**/b"_el, "**/a/**/b"_el, "/a/b"_el, "a;**/b"_el};
        auto candidates = std::vector<el::Path>{el::Path{"/"_el}};
        auto level = candidates;
        for (int depth = 0; depth < 4; ++depth) {
            auto next = std::vector<el::Path>{};
            for (const auto &parent : level) {
                for (const auto &name : {"a"_el, "b"_el}) {
                    next.push_back(parent / name);
                }
            }
            candidates.insert(candidates.end(), next.begin(), next.end());
            level = std::move(next);
        }
        for (const auto &source : patterns) {
            const auto pattern =
                Glob::compile(source, Flags{Flag::Default, Flag::MultiplePatterns, Flag::SemicolonSeparator});
            for (const auto mode : {Mode::Full, Mode::Suffix}) {
                for (const auto &candidate : candidates) {
                    if (!pattern->match(candidate, mode)) {
                        continue;
                    }
                    auto parent = candidate.parent();
                    while (!parent.isEmpty() && parent != candidate) {
                        REQUIRE(pattern->mayMatchDescendant(parent, mode));
                        if (parent.isRoot()) {
                            break;
                        }
                        parent = parent.parent();
                    }
                }
            }
        }
    }
    void testConcurrentMatchingAndLongPatterns() {
        const auto pattern = Glob::compile("**/a*/**/file?.cpp"_el);
        auto success = std::atomic_bool{true};
        auto threads = std::vector<std::jthread>{};
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&]() -> void {
                for (int j = 0; j < 100; ++j) {
                    if (!pattern->fullMatch("root/abc/deep/file1.cpp"_el) ||
                        pattern->fullMatch("root/abc/file.cpp"_el)) {
                        success.store(false);
                    }
                }
            });
        }
        threads.clear();
        REQUIRE(success.load());
        auto source = el::StringEditor{};
        auto candidate = el::Path{"/"_el};
        for (int i = 0; i < 24; ++i) {
            source.append("**/a/"_el);
            candidate /= "a"_el;
        }
        source.append("missing"_el);
        REQUIRE_FALSE(Glob::compile(el::String{source})->fullMatch(candidate));
        REQUIRE(el::StringPattern::fromGlob("a**b"_el).matches("a/x/b"_el));
        REQUIRE(el::StringPattern::fromPosixGlob("[!x]*"_el).matches("a/path"_el));
    }
};
