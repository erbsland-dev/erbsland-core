// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/Match.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <vector>

using namespace el::re;
using namespace el::text::literals;

TESTED_TARGETS(RegEx)
class RegExLazyCompileTest final : public el::UnitTest {
public:
    void testEagerCompileState() {
        const auto regex = RegEx::compile("abc"_el);

        REQUIRE(regex != nullptr);
        REQUIRE(regex->isCompiled());
        REQUIRE_NOTHROW(regex->compileNow());
        REQUIRE(regex->fullMatch("abc"_el) != nullptr);
    }

    void testExplicitLazyCompile() {
        const auto regex = RegEx::lazyCompile("abc"_el);

        REQUIRE(regex != nullptr);
        REQUIRE_FALSE(regex->isCompiled());
        REQUIRE_EQUAL(regex->pattern(), "abc"_el);
        REQUIRE_NOTHROW(regex->compileNow());
        REQUIRE(regex->isCompiled());
        REQUIRE(regex->fullMatch("abc"_el) != nullptr);
    }

    void testFirstMatchCompiles() {
        const auto regex = RegEx::lazyCompile("abc"_el);

        REQUIRE_FALSE(regex->isCompiled());
        REQUIRE(regex->findFirst("--abc--"_el) != nullptr);
        REQUIRE(regex->isCompiled());
    }

    void testCopiesShareLazyState() {
        const auto regex = RegEx::lazyCompile("abc"_el);
        auto copy = *regex;

        REQUIRE_FALSE(regex->isCompiled());
        REQUIRE_FALSE(copy.isCompiled());
        REQUIRE_NOTHROW(copy.compileNow());
        REQUIRE(regex->isCompiled());
        REQUIRE(copy.isCompiled());
        REQUIRE(regex->fullMatch("abc"_el) != nullptr);
    }

    void testGeneratorCompilesWhenConsumed() {
        const auto regex = RegEx::lazyCompile("a"_el);
        auto matches = regex->findAll("aaa"_el);

        REQUIRE_FALSE(regex->isCompiled());
        auto count = std::size_t{};
        for ([[maybe_unused]] const auto &match : matches) {
            ++count;
        }
        REQUIRE_EQUAL(count, std::size_t{3U});
        REQUIRE(regex->isCompiled());
    }

    void testInvalidPatternIsDelayedAndRetried() {
        RegExPtr regex;
        REQUIRE_NOTHROW(regex = RegEx::lazyCompile("("_el));
        REQUIRE(regex != nullptr);
        REQUIRE_FALSE(regex->isCompiled());

        REQUIRE_THROWS_AS(RegExError, regex->compileNow());
        REQUIRE_FALSE(regex->isCompiled());
        REQUIRE_THROWS_AS(RegExError, regex->match("anything"_el));
        REQUIRE_FALSE(regex->isCompiled());
    }

    void testFlagsAndSettingsArePreserved() {
        auto settings = Settings{};
        settings.enableFeature(Feature::EmptyGroups);
        const auto regex = RegEx::lazyCompile("()abc"_el, Flags{Flag::IgnoreCase}, settings);

        REQUIRE(regex->fullMatch("ABC"_el) != nullptr);
        REQUIRE(regex->isCompiled());
    }

    void testConcurrentFirstUse() {
        const auto regex = RegEx::lazyCompile("(abc)+"_el);
        auto failed = std::atomic_bool{false};
        auto threads = std::vector<std::thread>{};
        for (auto threadIndex = std::size_t{}; threadIndex < 8U; ++threadIndex) {
            threads.emplace_back([&regex, &failed]() -> void {
                for (auto iteration = std::size_t{}; iteration < 25U; ++iteration) {
                    try {
                        if (regex->fullMatch("abcabc"_el) == nullptr) {
                            failed.store(true, std::memory_order_relaxed);
                        }
                    } catch (...) {
                        failed.store(true, std::memory_order_relaxed);
                    }
                }
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }

        REQUIRE_FALSE(failed.load(std::memory_order_relaxed));
        REQUIRE(regex->isCompiled());
    }
};
