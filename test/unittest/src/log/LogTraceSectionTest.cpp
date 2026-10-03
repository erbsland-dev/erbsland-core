// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ByteCount.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogTraceSection.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>

using namespace el::text::literals;

TESTED_TARGETS(LogTraceSection validateTraceSectionName)
class LogTraceSectionTest final : public el::UnitTest {
    static constexpr auto cName = "Verbose-Details"_el;
    static constexpr auto cSection = el::LogTraceSection{cName};
    static constexpr auto cCopy = cSection;
    static constexpr auto cEmpty = el::LogTraceSection{};
    static_assert(!cSection.isEmpty());
    static_assert(cEmpty.isEmpty());

public:
    void testConstantConstructionAndSpelling() {
        REQUIRE_EQUAL(cCopy.toString(), "Verbose-Details"_el);
        REQUIRE(el::LogTraceSection{""_el}.isEmpty());
        REQUIRE(el::LogTraceSection{el::String{}}.isEmpty());
        REQUIRE_EQUAL(cSection.toString().storageId(), el::String{cName}.storageId());
    }

    void testValidationParity() {
        const auto valid = std::array{""_el, "a"_el, "Z0_-"_el, "a__-"_el};
        for (const auto &name : valid) {
            REQUIRE_NOTHROW(el::LogTraceSection{name});
            REQUIRE_NOTHROW(el::LogTraceSection{el::String{name}});
        }
        const auto invalid = std::array{
            "_"_el, "_a"_el, "0a"_el, "-a"_el, "a b"_el, "a/b"_el, "a.b"_el, "a\0b"_el, "\0"_el, "a\n"_el, "\u00e4"_el};
        for (const auto &name : invalid) {
            REQUIRE_THROWS_AS(el::ParameterFailure, el::LogTraceSection{name});
            REQUIRE_THROWS_AS(el::ParameterFailure, el::LogTraceSection{el::String{name}});
        }
    }

    void testLengthBoundaries() {
        constexpr auto maximumName =
            "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij"_el;
        constexpr auto maximumSection = el::LogTraceSection{maximumName};
        static_assert(maximumName.length() == el::ByteCount{100U});
        static_assert(!maximumSection.isEmpty());
        REQUIRE_EQUAL(maximumSection.toString(), maximumName);
        REQUIRE_NOTHROW(el::LogTraceSection{el::String{maximumName}});

        constexpr auto excessiveName =
            "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghija"_el;
        static_assert(excessiveName.length() == el::ByteCount{101U});
        REQUIRE_THROWS_AS(el::ParameterFailure, el::LogTraceSection{excessiveName});
        REQUIRE_THROWS_AS(el::ParameterFailure, el::LogTraceSection{el::String{excessiveName}});
    }

    void testEqualityAcrossStorageAlternatives() {
        const auto literal = el::LogTraceSection{"Foo-Bar"_el};
        const auto otherLiteral = el::LogTraceSection{"fOO_bAR"_el};
        const auto string = el::LogTraceSection{el::String{"FOO_BAR"_el}.copy()};
        const auto otherString = el::LogTraceSection{el::String{"foo-bar"_el}.copy()};
        REQUIRE(literal == otherLiteral);
        REQUIRE(literal == string);
        REQUIRE(string == literal);
        REQUIRE(string == otherString);
        REQUIRE_FALSE(literal == el::LogTraceSection{"foo-bars"_el});
        REQUIRE_FALSE(literal == el::LogTraceSection{"foobar"_el});
        REQUIRE_FALSE(literal == cEmpty);
        REQUIRE(cEmpty == el::LogTraceSection{el::String{}});
        REQUIRE_EQUAL(literal.toString(), "Foo-Bar"_el);
        REQUIRE_EQUAL(string.toString(), "FOO_BAR"_el);
    }

    void testProgrammaticDuplicatesRetainFirstSpelling() {
        auto configuration = el::LogConfiguration{};
        configuration.enableTraceSection(el::LogTraceSection{"Foo-Bar"_el})
            .enableTraceSection(el::LogTraceSection{"foo_bar"_el})
            .enableTraceSection(cEmpty);
        REQUIRE_EQUAL(configuration.traceSections().size(), std::size_t{1});
        REQUIRE_EQUAL(configuration.traceSections().front().toString(), "Foo-Bar"_el);
        REQUIRE(configuration.isTraceSectionEnabled(el::LogTraceSection{"FOO-BAR"_el}));
    }
};
