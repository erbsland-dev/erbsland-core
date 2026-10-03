// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DisplayTextTestLayer.hpp"

#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/i18n/impl/DisplayTextCore.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(DisplayTextData DisplayTextDomain DisplayTextDomainEntry DisplayTextCore)
class DisplayTextDataTest final : public el::UnitTest {
public:
    void testExactLookupAndEmptyValues() {
        const auto data = DisplayTextTestData{
            {""_el, "name"_el, "root"_el},
            {"demo"_el, "empty"_el, ""_el},
            {"demo"_el, "name"_el, "domain"_el},
            {"demo.deep"_el, "name"_el, "exact"_el},
        };
        REQUIRE_EQUAL(data.findText("name"_el).value(), "root"_el);
        REQUIRE_EQUAL(data.findText("demo.name"_el).value(), "domain"_el);
        REQUIRE_EQUAL(data.findText("demo.deep.name"_el).value(), "exact"_el);
        REQUIRE(data.findText("demo.empty"_el).has_value());
        REQUIRE(data.findText("demo.empty"_el)->isEmpty());
        REQUIRE_FALSE(data.findText("demo.other.name"_el).has_value());
        REQUIRE_FALSE(data.findText("Demo.name"_el).has_value());
        REQUIRE_FALSE(data.findText("missing"_el).has_value());
        REQUIRE_FALSE(data.findText(""_el).has_value());
    }

    void testUtf8OrderingAndPrefixes() {
        const auto data = DisplayTextTestData{
            {""_el, "zebra"_el, "ascii"_el},
            {""_el, "äpfel"_el, "umlaut"_el},
            {""_el, "日本語"_el, "japanese"_el},
            {"zebra"_el, "name"_el, "ascii domain"_el},
            {"äpfel"_el, "name"_el, "umlaut domain"_el},
            {"äpfel"_el, "names"_el, "longer name"_el},
            {"äpfel.baum"_el, "日本語"_el, "nested unicode"_el},
            {"日本語"_el, "name"_el, "japanese domain"_el},
        };
        REQUIRE_EQUAL(data.findText("zebra"_el).value(), "ascii"_el);
        REQUIRE_EQUAL(data.findText("äpfel"_el).value(), "umlaut"_el);
        REQUIRE_EQUAL(data.findText("日本語"_el).value(), "japanese"_el);
        REQUIRE_EQUAL(data.findText("zebra.name"_el).value(), "ascii domain"_el);
        REQUIRE_EQUAL(data.findText("äpfel.name"_el).value(), "umlaut domain"_el);
        REQUIRE_EQUAL(data.findText("äpfel.names"_el).value(), "longer name"_el);
        REQUIRE_EQUAL(data.findText("äpfel.baum.日本語"_el).value(), "nested unicode"_el);
        REQUIRE_EQUAL(data.findText("日本語.name"_el).value(), "japanese domain"_el);
        REQUIRE_FALSE(data.findText("äpfel.nam"_el).has_value());
        REQUIRE_FALSE(data.findText("äpfel.namesake"_el).has_value());
        REQUIRE_FALSE(data.findText("äpf.name"_el).has_value());
        REQUIRE_FALSE(data.findText("äpfels.name"_el).has_value());
        REQUIRE_FALSE(data.findText("äpfel."_el).has_value());
    }

    void testSlicedKeys() {
        const auto data = DisplayTextTestData{{"demo"_el, "name"_el, "found"_el}};
        const auto literalKey = el::text::String{"prefix.demo.name.suffix"_el};
        const auto ownedKey = el::text::String{el::text::StringEditor{literalKey}};
        const auto range = el::unit::ByteRange{el::unit::ByteIndex{7U}, el::unit::ByteCount{9U}};
        REQUIRE_EQUAL(data.findText(literalKey.slice(range)).value(), "found"_el);
        REQUIRE_EQUAL(data.findText(ownedKey.slice(range)).value(), "found"_el);
    }

    void testEmptyCatalog() {
        const auto data = DisplayTextTestData{{}};
        REQUIRE(data.domains().empty());
        REQUIRE_FALSE(data.findText("missing"_el).has_value());
    }

    void testCoreCatalogIsSortedAndComplete() {
        const auto data = el::i18n::impl::DisplayTextCore{};
        auto domainCount = std::size_t{};
        auto entryCount = std::size_t{};
        auto previousDomain = el::text::String{};
        for (const auto &domain : data.domains()) {
            const auto name = el::text::String{domain.name};
            if (domainCount != 0U) {
                REQUIRE_LESS(previousDomain, name);
            }
            previousDomain = name;
            ++domainCount;
            auto previousKey = el::text::String{};
            for (const auto &entry : domain.entries) {
                const auto key = el::text::String{entry.key};
                REQUIRE_LESS(previousKey, key);
                previousKey = key;
                ++entryCount;
            }
        }
        REQUIRE_EQUAL(domainCount, 3U);
        REQUIRE_EQUAL(entryCount, 141U);
        REQUIRE_EQUAL(data.findText("unknown_error"_el).value(), "Unknown Error"_el);
        REQUIRE_EQUAL(
            data.findText("options.help_option_details"_el).value(),
            "Display the available options, or use --help=<name> to find help for a command, option or choice."_el);
        REQUIRE_EQUAL(
            data.findText("options.short_help_does_not_accept_a_value_title"_el).value(),
            "Detailed help requires --help"_el);
    }
};
