// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../engine/MockStringMatch.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/impl/text/Replacement.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::re;
using impl::Replacement;

TESTED_TARGETS(Replacement)
TAGS(Text Replacement)
class ReplacementTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    void testAppendTo_EmptyReplacementDoesNothing() {
        const auto replacement = Replacement{};
        StringEditor out{"prefix"_el};
        REQUIRE_NOTHROW(replacement.appendTo(out, nullptr));
        REQUIRE_EQUAL(out, StringEditor{"prefix"_el});
    }

    void testAppendToAndSize_StaticTextOnly() {
        Replacement replacement;
        replacement.addStaticText(StringEditor{"abc"_el});
        replacement.addStaticText(StringEditor{"def"_el});

        REQUIRE_EQUAL(replacement.length(nullptr), el::unit::ByteLength{6U});

        StringEditor out;
        replacement.appendTo(out, nullptr);
        REQUIRE_EQUAL(out, StringEditor{"abcdef"_el});
    }

    void testAppendToAndSize_CaptureGroups() {
        constexpr auto text = "abcd"_el;
        CaptureGroupList groups;
        groups.emplace_back(0, CaptureRange{0, 4}, String{});
        groups.emplace_back(1, CaptureRange{0, 2}, String{});
        groups.emplace_back(2, CaptureRange{2, 4}, String{});
        const auto match = std::make_shared<MockStringMatch>(std::move(groups), text);
        REQUIRE(match != nullptr);

        Replacement replacement;
        replacement.addStaticText(StringEditor{"<"_el});
        replacement.addCaptureGroup(2);
        replacement.addStaticText(StringEditor{">-"_el});
        replacement.addCaptureGroup(1);
        replacement.addStaticText(StringEditor{"-"_el});
        replacement.addCaptureGroup(0);

        REQUIRE_EQUAL(replacement.length(match), el::unit::ByteLength{1U + 2U + 2U + 2U + 1U + 4U});

        StringEditor out;
        replacement.appendTo(out, match);
        REQUIRE_EQUAL(out, StringEditor{"<cd>-ab-abcd"_el});
    }
};
