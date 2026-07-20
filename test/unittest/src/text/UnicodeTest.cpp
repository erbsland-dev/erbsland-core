// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/impl/UnicodeData.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/UnicodeVersion.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using el::text::ucdVersion;
using el::unit::Version;

TESTED_TARGETS(ucdVersion unicodeDisplayWidthFor)
class UnicodeTest final : public el::UnitTest {
public:
    void testUcdVersion() {
        const auto expectedVersion = Version{17, 0, 0};
        REQUIRE_EQUAL(ucdVersion(), expectedVersion);
    }

    void testDisplayWidth() {
        using el::text::impl::unicodeDisplayWidthFor;

        REQUIRE_EQUAL(unicodeDisplayWidthFor(U'A'), 1U);
        REQUIRE_EQUAL(unicodeDisplayWidthFor(U'\n'), 0U);
        REQUIRE_EQUAL(unicodeDisplayWidthFor(0x00ADU), 0U);
        REQUIRE_EQUAL(unicodeDisplayWidthFor(0x0301U), 0U);
        REQUIRE_EQUAL(unicodeDisplayWidthFor(0x2500U), 1U);
        REQUIRE_EQUAL(unicodeDisplayWidthFor(0x4E00U), 2U);
        REQUIRE_EQUAL(unicodeDisplayWidthFor(0x1F600U), 2U);
        REQUIRE_EQUAL(unicodeDisplayWidthFor(0xD800U), 1U);
        REQUIRE_EQUAL(unicodeDisplayWidthFor(0x110000U), 1U);
    }
};
