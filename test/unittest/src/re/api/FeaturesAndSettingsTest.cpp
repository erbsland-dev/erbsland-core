// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../StringHelper.hpp"

#include <erbsland/re/Features.hpp>
#include <erbsland/re/Flags.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/re/Settings.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <format>
#include <string>

using namespace el::re;

TESTED_TARGETS(Feature Features Flag Flags Settings)
TAGS(Api Features)
class FeaturesAndSettingsTest final : public el::UnitTest {
public:
    void testFeatureToStringAndFormatter() {
        REQUIRE_EQUAL(toString(Feature::EscapeBell), "EscapeBell"_el);
        REQUIRE_EQUAL(std::format("{}", Feature::EscapeBell), std::string{"EscapeBell"});
    }

    void testFeaturesToStringAndFormatter() {
        REQUIRE_EQUAL(Features{}.toString(), String{});
        REQUIRE_EQUAL(std::format("{}", Features{}), std::string{});

        const auto features = Features{Feature::EscapeBell, Feature::PosixClasses, Feature::AnchorLowercaseZ};
        REQUIRE_EQUAL(features.toString(), "EscapeBell, PosixClasses, AnchorLowercaseZ"_el);
        REQUIRE_EQUAL(std::format("{}", features), std::string{"EscapeBell, PosixClasses, AnchorLowercaseZ"});
    }

    void testCoreFlagIntegration() {
        static_assert(std::same_as<decltype(Flags{Flag::Ascii} | Flag::Verbose), Flags>);
        static_assert(std::same_as<decltype(Flag::Ascii | Flags{Flag::Verbose}), Flags>);
        static_assert(std::same_as<decltype(Features{Feature::EscapeBell} & Feature::EscapeBell), Features>);
        static_assert(std::same_as<decltype(Feature::PosixClasses ^ Features{}), Features>);
        static_assert(std::same_as<decltype(Flags::fromRawValue(0x20U)), Flags>);
        static_assert(std::same_as<decltype(Features::fromRawValue(0x0400U)), Features>);

        const auto flags = Flags{Flag::Ascii} | Flag::Verbose;
        REQUIRE_FALSE(flags.isEmpty());
        REQUIRE(flags.hasAny());
        REQUIRE(flags.isSet(Flag::Ascii));
        REQUIRE_FALSE(flags.isCleared(Flag::Ascii));
        REQUIRE(flags.isCleared(Flag::DotAll));
        REQUIRE(flags.contains(Flags{Flag::Ascii, Flag::Verbose}));
        REQUIRE(flags.intersects(Flags{Flag::DotAll, Flag::Verbose}));
        REQUIRE_EQUAL(flags.toString(), "Ascii, Verbose"_el);

        const auto features = Features{Feature::EscapeBell, Feature::PosixClasses};
        REQUIRE(features.isSet(Feature::EscapeBell));
        REQUIRE_FALSE(features.isSet(Feature::AllCompatibility));
        REQUIRE(Features{Feature::Default}.isSet(Feature::AllCompatibility));
        REQUIRE(Features{Feature::Default}.isSet(Feature::AcceptNullInInput));
        REQUIRE_FALSE(Features{Feature::Default}.isSet(Feature::AcceptNullInPattern));
    }

    void testSettingsFeatureToggling() {
        Settings settings;
        REQUIRE(settings.hasFeature(Feature::EscapeBell));

        settings.disableFeature(Feature::EscapeBell);
        REQUIRE_FALSE(settings.hasFeature(Feature::EscapeBell));

        settings.enableFeature(Feature::EscapeBell);
        REQUIRE(settings.hasFeature(Feature::EscapeBell));

        REQUIRE(settings.hasFeature(Feature::AcceptNullInInput));
        settings.disableFeature(Feature::AcceptNullInInput);
        REQUIRE_FALSE(settings.hasFeature(Feature::AcceptNullInInput));
    }

    void testSettingsCaptureGroupCountIsIndependent() {
        Settings settings;

        settings.setMaximumGroupNestingDepth(3);
        settings.setMaximumCaptureGroupCount(7);

        REQUIRE_EQUAL(settings.maximumGroupNestingDepth(), 3U);
        REQUIRE_EQUAL(settings.maximumCaptureGroupCount(), 7U);
    }

    void testCompileRespectsMaximumPatternLength() {
        Settings settings;
        settings.setMaximumPatternLength(el::unit::CpLength{3U});

        try {
            (void)RegEx::compile("abcd"_el, {}, settings);
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), ErrorCategory::Parser);
            REQUIRE_EQUAL(e.title(), "Failed to parse regular expression"_el);
            REQUIRE_EQUAL(e.description(), "The maximum pattern length is 3 characters."_el);
        }
    }

    void testCompileRespectsMaximumCaptureGroupCount() {
        Settings settings;
        settings.setMaximumCaptureGroupCount(1);

        try {
            (void)RegEx::compile("(a)(b)"_el, {}, settings);
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), ErrorCategory::Parser);
            REQUIRE_EQUAL(e.title(), "Failed to parse regular expression"_el);
            REQUIRE_EQUAL(e.description(), "Maximum capture group count of 1 exceeded"_el);
        }
    }
};
