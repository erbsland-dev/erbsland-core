// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "../../core/ApplicationTestScope.hpp"

#include <erbsland/core/impl/application_data/ApplicationRegExData.hpp>
#include <erbsland/core/impl/ApplicationDataAccess.hpp>
#include <erbsland/core/impl/ApplicationExecutionContext.hpp>
#include <erbsland/re/impl/CachedMatches.hpp>
#include <erbsland/re/impl/RegExCache.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/re/Match16.hpp>
#include <erbsland/re/Match32.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/String.hpp>
#include <erbsland/U16String.hpp>
#include <erbsland/U32String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <barrier>
#include <thread>
#include <type_traits>

using namespace el::text::literals;

TESTED_TARGETS(RegExCached RegExCache cachedMatches ApplicationRegExData)
class RegExCachedTest final : public el::UnitTest {
    template <typename T>
    static constexpr bool cCanCompile = requires(T pattern) { el::re::compile(pattern); };
    template <typename T>
    static constexpr bool cCanReplace =
        requires(T replacement) { el::re::replaceAll("a"_el, el::String{"a"_el}, replacement); };
    static_assert(cCanCompile<el::StringLiteral>);
    static_assert(!cCanCompile<el::String>);
    static_assert(!cCanCompile<const char *>);
    static_assert(cCanReplace<el::StringLiteral>);
    static_assert(!cCanReplace<el::String>);
    static_assert(std::is_same_v<decltype(el::re::compile("a"_el)), el::re::ConstRegExPtr>);

public:
    void testLiteralReuseAndUncachedIndependence() {
        auto scope = ApplicationTestScope<>{};
        const auto data = el::core::impl::applicationData();
        REQUIRE(data->regularExpressions().getIfExists() == nullptr);
        const auto ordinary = el::RegEx::compile("a+"_el);
        REQUIRE(data->regularExpressions().getIfExists() == nullptr);
        const auto first = el::re::compile("a+"_el);
        static constexpr char samePattern[] = {'a', '+', '\0'};
        constexpr auto sameLiteral = el::StringLiteral{samePattern};
        REQUIRE(el::re::compile(sameLiteral) == first);
        REQUIRE(first != ordinary);
        REQUIRE(el::RegEx::compile("a+"_el) != first);
        REQUIRE(data->regularExpressions().getIfExists() != nullptr);
    }
    void testMatchingAndReplacement() {
        auto scope = ApplicationTestScope<>{};
        REQUIRE(el::re::match("a"_el, "ba"_el) == nullptr);
        REQUIRE(el::re::findFirst("a"_el, "ba"_el) != nullptr);
        REQUIRE(el::re::fullMatch("a"_el, "ab"_el) == nullptr);
        REQUIRE_EQUAL(el::re::fullMatch("(?i)(?<letter>a)"_el, "A"_el)->content("letter"_el), "A"_el);
        REQUIRE_EQUAL(el::re::collectAll("a"_el, "aba"_el).size(), 2U);
        REQUIRE_EQUAL(el::re::replaceAll("(?<letter>a)"_el, "aba"_el, "[{letter}]"_el), "[a]b[a]"_el);
        REQUIRE_THROWS_AS(el::re::RegExError, el::re::compile("("_el));
        REQUIRE_THROWS_AS(el::re::RegExError, el::re::compile("("_el));
        REQUIRE_THROWS_AS(el::re::RegExError, el::re::replaceAll("a"_el, "a"_el, "{missing}"_el));
        REQUIRE_THROWS_AS(el::re::RegExError, el::re::findAll("("_el, "a"_el));
    }
    void testAllStringWidths() {
        auto scope = ApplicationTestScope<>{};
        WITH_CONTEXT(checkWidth(el::String{"a😀a"_el}));
        WITH_CONTEXT(checkWidth(el::U16String{u"a😀a"_el}));
        WITH_CONTEXT(checkWidth(el::U32String{U"a😀a"_el}));
    }
    void testPromotionAndBoundedRetention() {
        auto cache = el::re::impl::RegExCache{el::ItemCount{1}, el::ItemCount{2}};
        auto first = cache.compile("a"_el);
        auto second = cache.compile("b"_el);
        REQUIRE(cache.compile("a"_el) == first); // Backing hit promotes a and makes it most recent.
        const auto third = cache.compile("c"_el);
        REQUIRE(cache.compile("a"_el) == first);
        REQUIRE(cache.compile("b"_el) != second);
        REQUIRE(third != nullptr);
        auto disabled = el::re::impl::RegExCache{el::ItemCount{0}, el::ItemCount{0}};
        REQUIRE(disabled.compile("a"_el) != disabled.compile("a"_el));
    }
    void testFifoHitsDoNotTouchBackingRecency() {
        auto cache = el::re::impl::RegExCache{el::ItemCount{2}, el::ItemCount{2}};
        const auto first = cache.compile("a"_el);
        static_cast<void>(cache.compile("b"_el));
        REQUIRE(cache.compile("a"_el) == first);
        static_cast<void>(cache.compile("c"_el));
        REQUIRE(cache.compile("a"_el) != first);
    }
    void testDelayedGeneratorSurvivesEviction() {
        auto cache = el::re::impl::RegExCache{el::ItemCount{1}, el::ItemCount{1}};
        auto expression = cache.compile("a"_el);
        const auto weak = std::weak_ptr<const el::re::RegEx>{expression};
        auto matches = el::re::impl::cachedMatches(expression, el::String{"aba"_el});
        expression.reset();
        static_cast<void>(cache.compile("b"_el));
        REQUIRE_FALSE(weak.expired());
        REQUIRE_EQUAL((*matches.next())->content(), "a"_el);
        static_cast<void>(cache.compile("c"_el));
        REQUIRE_EQUAL((*matches.next())->content(), "a"_el);
        REQUIRE_FALSE(matches.next().has_value());
    }
    void testGeneratorOutlivesApplicationScope() {
        auto matches = el::re::MatchGenerator{};
        auto weak = std::weak_ptr<const el::re::RegEx>{};
        {
            auto scope = ApplicationTestScope<>{};
            weak = el::re::compile(R"(\b)"_el);
            matches = el::re::findAll(R"(\b)"_el, el::String{"ab"_el});
        }
        REQUIRE_FALSE(weak.expired());
        REQUIRE_EQUAL((*matches.next())->content(), ""_el);
        auto remaining = 0;
        while (matches.next()) {
            ++remaining;
        }
        REQUIRE_EQUAL(remaining, 1);
        matches = {};
        REQUIRE(weak.expired());
    }
    void testScopeIsolation() {
        auto first = el::re::ConstRegExPtr{};
        {
            auto scope = ApplicationTestScope<>{};
            first = el::re::compile("a"_el);
        }
        auto scope = ApplicationTestScope<>{};
        REQUIRE(el::re::compile("a"_el) != first);
        REQUIRE(first->match("a"_el) != nullptr);
    }
    void testConcurrentMissesAndHits() {
        auto scope = ApplicationTestScope<>{};
        const auto data = el::core::impl::applicationData();
        auto results = std::array<el::re::ConstRegExPtr, 8>{};
        auto threads = std::array<std::thread, 8>{};
        auto start = std::barrier{8};
        for (auto index = std::size_t{0}; index < threads.size(); ++index) {
            threads[index] = std::thread{[&, index]() {
                const auto context = el::core::impl::ApplicationExecutionContext{data};
                start.arrive_and_wait();
                results[index] = el::re::compile("(?<letter>[a-z])+"_el);
                for (auto iteration = 0; iteration < 20; ++iteration) {
                    results[index] = el::re::compile("(?<letter>[a-z])+"_el);
                }
            }};
        }
        for (auto &thread : threads) {
            thread.join();
        }
        for (const auto &result : results) {
            REQUIRE(result == results.front());
        }
    }

private:
    template <typename T>
    void checkWidth(const T &text) {
        REQUIRE(el::re::match("a"_el, text) != nullptr);
        REQUIRE(el::re::fullMatch("a.a"_el, text) != nullptr);
        REQUIRE(el::re::findFirst("😀"_el, text) != nullptr);
        REQUIRE_EQUAL(el::re::collectAll("a"_el, text).size(), 2U);
        auto matches = el::re::findAll("a"_el, T{text});
        REQUIRE(matches.next().has_value());
        REQUIRE(matches.next().has_value());
        REQUIRE_FALSE(matches.next().has_value());
    }
};
