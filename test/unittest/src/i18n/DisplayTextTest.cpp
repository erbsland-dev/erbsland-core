// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DisplayTextTestLayer.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/FormatError.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <thread>
#include <vector>

using namespace el::text::literals;

class CountingDisplayTextTranslator final : public el::i18n::DisplayTextTranslator {
public:
    [[nodiscard]] auto translate(const el::text::String &key, const el::text::String &) const
        -> el::text::String override {
        ++callCount;
        if (key == "options.usage_label"_el || key == "demo.deep.name"_el) {
            return "Verwendung"_el;
        }
        return {};
    }

public:
    mutable std::atomic<int> callCount{};
};

TESTED_TARGETS(DisplayText DisplayTextTranslator displayText displayFormat)
class DisplayTextTest final : public el::UnitTest {
public:
    void testSpecificityBeforeLayerPrecedence() {
        auto scope = ApplicationTestScope<>{};
        const auto registry = scope.app().displayText();
        const auto low = std::make_shared<DisplayTextTestData>(std::initializer_list<DisplayTextTestEntry>{
            {"demo.deep"_el, "name"_el, "exact"_el},
            {"demo"_el, "name"_el, "domain"_el},
            {""_el, "name"_el, "root"_el},
        });
        const auto high = std::make_shared<DisplayTextTestData>(std::initializer_list<DisplayTextTestEntry>{
            {"demo"_el, "name"_el, "upper domain"_el},
            {""_el, "name"_el, "upper root"_el},
        });
        registry->addData(low);
        registry->addData(high);
        REQUIRE_EQUAL(el::displayText("demo.deep.name"_el), "exact"_el);
        REQUIRE_EQUAL(el::displayText("demo.other.name"_el), "upper domain"_el);
        REQUIRE_EQUAL(el::displayText("other.deep.name"_el), "upper root"_el);
        REQUIRE_EQUAL(el::displayText("missing.key"_el), "missing.key"_el);
        registry->addData(low);
        REQUIRE_EQUAL(el::displayText("demo.name"_el), "upper domain"_el);
        REQUIRE(registry->removeData(high));
        REQUIRE_EQUAL(el::displayText("demo.other.name"_el), "domain"_el);
        REQUIRE_FALSE(registry->removeData(high));
        REQUIRE(registry->removeData(low));
        REQUIRE_EQUAL(el::displayText("demo.deep.name"_el), "demo.deep.name"_el);
        REQUIRE_THROWS_AS(el::err::LogicFailure, registry->addData(nullptr));
        REQUIRE_THROWS_AS(el::err::LogicFailure, registry->removeData(nullptr));
    }

    void testCachesAndTranslatorReplacement() {
        auto scope = ApplicationTestScope<>{};
        const auto registry = scope.app().displayText();
        const auto translator = std::make_shared<CountingDisplayTextTranslator>();
        registry->setTranslator(translator);
        REQUIRE_EQUAL(registry->translator(), translator);
        REQUIRE_EQUAL(el::displayText("options.usage_label"_el), "Verwendung"_el);
        REQUIRE_EQUAL(el::displayText("options.usage_label"_el), "Verwendung"_el);
        REQUIRE_EQUAL(el::displayFormat("options.usage_label"_el).build(), "Verwendung"_el);
        REQUIRE_EQUAL(translator->callCount.load(), 1);
        REQUIRE_EQUAL(el::displayText("options.version_label"_el), "Version"_el);
        REQUIRE_EQUAL(translator->callCount.load(), 2);
        REQUIRE_EQUAL(el::displayText("missing"_el), "missing"_el);
        REQUIRE_EQUAL(translator->callCount.load(), 2);
        registry->setTranslator(translator);
        REQUIRE_EQUAL(el::displayText("options.usage_label"_el), "Verwendung"_el);
        REQUIRE_EQUAL(translator->callCount.load(), 3);
        registry->setTranslator(nullptr);
        REQUIRE_FALSE(registry->translator());
        REQUIRE_EQUAL(el::displayFormat("options.usage_label"_el).build(), "Usage"_el);
        {
            auto layer = DisplayTextTestLayer{{"options"_el, "usage_label"_el, "Use {}"_el}};
            REQUIRE_EQUAL(el::displayFormat("options.usage_label"_el).build("this"_el), "Use this"_el);
        }
        REQUIRE_EQUAL(el::displayFormat("options.usage_label"_el).build(), "Usage"_el);
    }

    void testOriginalKeyIsTranslatedAfterFallback() {
        auto scope = ApplicationTestScope<>{};
        auto layer = DisplayTextTestLayer{{"demo"_el, "name"_el, "English"_el}};
        const auto translator = std::make_shared<CountingDisplayTextTranslator>();
        scope.app().displayText()->setTranslator(translator);
        REQUIRE_EQUAL(el::displayText("demo.deep.name"_el), "Verwendung"_el);
        REQUIRE_EQUAL(el::displayText("demo.name"_el), "English"_el);
    }

    void testEmptyTextAndInvalidFormat() {
        auto scope = ApplicationTestScope<>{};
        auto layer = DisplayTextTestLayer{
            {"demo"_el, "empty"_el, ""_el},
            {"demo"_el, "invalid"_el, "{"_el},
        };
        REQUIRE(el::displayText("demo.empty"_el).isEmpty());
        REQUIRE(el::displayFormat("demo.empty"_el).build().isEmpty());
        REQUIRE_THROWS_AS(el::text::FormatError, el::displayFormat("demo.invalid"_el));
        REQUIRE_THROWS_AS(el::text::FormatError, el::displayFormat("demo.invalid"_el));
    }

    void testApplicationIsolation() {
        {
            auto scope = ApplicationTestScope<>{};
            auto layer = DisplayTextTestLayer{{"options"_el, "usage_label"_el, "Use"_el}};
            REQUIRE_EQUAL(el::displayText("options.usage_label"_el), "Use"_el);
        }
        auto scope = ApplicationTestScope<>{};
        REQUIRE_EQUAL(el::displayText("options.usage_label"_el), "Usage"_el);
    }

    void testConcurrentReadsAndRegistration() {
        auto scope = ApplicationTestScope<>{};
        const auto registry = scope.app().displayText();
        const auto layer = std::make_shared<DisplayTextTestData>(std::initializer_list<DisplayTextTestEntry>{
            {"options"_el, "usage_label"_el, "Use"_el},
        });
        auto failed = std::atomic<bool>{false};
        auto threads = std::vector<std::thread>{};
        for (auto index = 0; index < 4; ++index) {
            threads.emplace_back([registry, &failed]() -> void {
                for (auto repetition = 0; repetition < 200; ++repetition) {
                    const auto text = registry->format("options.usage_label"_el).build();
                    if (text != "Usage"_el && text != "Use"_el) {
                        failed = true;
                    }
                }
            });
        }
        for (auto repetition = 0; repetition < 100; ++repetition) {
            registry->addData(layer);
            REQUIRE(registry->removeData(layer));
        }
        for (auto &thread : threads) {
            thread.join();
        }
        REQUIRE_FALSE(failed.load());
    }
};
