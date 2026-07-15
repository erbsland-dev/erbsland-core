// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/i18n/DisplayTextMap.hpp>
#include <erbsland/text/FormatError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

using namespace el::text::literals;

class TestDisplayTextTranslator final : public el::i18n::DisplayTextTranslator {
public:
    [[nodiscard]] auto translate(const el::text::StringView &key, const el::text::StringView &) const
        -> el::text::StringView override {
        ++callCount;
        if (key == "options.UsageLabel"_el) {
            return "Verwendung"_el;
        }
        if (key == "Same"_el) {
            return "Translated"_el;
        }
        return {};
    }

public:
    mutable std::atomic<int> callCount{};
};

TESTED_TARGETS(DisplayTextMap DisplayTextTranslator)
class DisplayTextMapTest final : public el::UnitTest {
public:
    void testDefaultsAndFallback() {
        auto map = el::i18n::DisplayTextMap{};
        map.set("Label"_el, "global"_el).set("demo.Label"_el, "domain"_el).set("demo.deep.Label"_el, "exact"_el);

        REQUIRE_EQUAL(map.text("options.UsageLabel"_el), "Usage"_el);
        REQUIRE_EQUAL(map.text("PathValuesHeading"_el), "Paths"_el);
        REQUIRE_EQUAL(map.text("PlatformErrorHeading"_el), "Platform Error"_el);
        REQUIRE_EQUAL(map.text("demo.deep.Label"_el), "exact"_el);
        map.remove("demo.deep.Label"_el);
        REQUIRE_EQUAL(map.text("demo.deep.Label"_el), "domain"_el);
        map.remove("demo.Label"_el);
        REQUIRE_EQUAL(map.text("demo.deep.Label"_el), "global"_el);
        REQUIRE_EQUAL(map.text("missing.Key"_el), "missing.Key"_el);
    }

    void testCloneIsIndependentAndClearsCaches() {
        auto original = el::i18n::DisplayTextMap{};
        REQUIRE_EQUAL(original.text("options.UsageLabel"_el), "Usage"_el);
        auto clone = original.clone();
        clone->set("options.UsageLabel"_el, "Use"_el);

        REQUIRE_EQUAL(original.text("options.UsageLabel"_el), "Usage"_el);
        REQUIRE_EQUAL(clone->text("options.UsageLabel"_el), "Use"_el);
    }

    void testTranslationAndFormatCaching() {
        auto translator = std::make_shared<TestDisplayTextTranslator>();
        auto map = el::i18n::DisplayTextMap{};
        map.setTranslator(translator);

        REQUIRE_EQUAL(map.text("options.UsageLabel"_el), "Verwendung"_el);
        REQUIRE_EQUAL(map.text("options.UsageLabel"_el), "Verwendung"_el);
        REQUIRE_EQUAL(map.format("options.UsageLabel"_el).build(), "Verwendung"_el);
        REQUIRE_EQUAL(translator->callCount.load(), 1);
        REQUIRE_EQUAL(map.text("options.VersionLabel"_el), "Version"_el);
        REQUIRE_EQUAL(translator->callCount.load(), 2);
        map.set("Same"_el, "Same"_el);
        REQUIRE_EQUAL(map.text("Same"_el), "Translated"_el);
    }

    void testInvalidFormatIsReported() {
        auto map = el::i18n::DisplayTextMap{};
        map.set("demo.InvalidFormat"_el, "{"_el);
        REQUIRE_THROWS_AS(el::text::FormatError, map.format("demo.InvalidFormat"_el));
    }

    void testConcurrentConstReads() {
        const auto map = el::i18n::DisplayTextMap::defaultMap();
        auto threads = std::vector<std::thread>{};
        auto failed = std::atomic<bool>{false};
        for (auto index = 0; index < 8; ++index) {
            threads.emplace_back([map, &failed]() -> void {
                for (auto repetition = 0; repetition < 1'000; ++repetition) {
                    if (map->text("options.UsageLabel"_el) != "Usage"_el) {
                        failed = true;
                    }
                }
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }
        REQUIRE_FALSE(failed.load());
    }
};
