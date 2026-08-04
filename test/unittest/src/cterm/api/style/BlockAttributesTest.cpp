// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <functional>

TESTED_TARGETS(BlockAttributes)
class BlockAttributesTest final : public el::UnitTest {
public:
    void testDefaultResetAndMaskFactoriesDescribeSpecifiedAndEnabledBits() {
        constexpr auto inherited = BlockAttributes{};
        constexpr auto reset = BlockAttributes::reset();
        constexpr auto supported = BlockAttributes::fromMask(
            static_cast<uint8_t>(BlockAttributes::Bold.value | BlockAttributes::Underline.value));

        REQUIRE_EQUAL(sizeof(BlockAttributes), std::size_t{2});
        REQUIRE_FALSE(inherited.isBoldSpecified());
        REQUIRE_FALSE(inherited.isBold());
        REQUIRE(reset.isBoldSpecified());
        REQUIRE_FALSE(reset.isBold());
        REQUIRE(supported.isBoldSpecified());
        REQUIRE(supported.isBold());
        REQUIRE(supported.isUnderlineSpecified());
        REQUIRE(supported.isUnderline());
        REQUIRE_FALSE(supported.isDimSpecified());
    }

    void testSettersCanEnableDisableAndReinheritIndividualFlags() {
        auto attributes = BlockAttributes{};
        attributes.setBold(true);
        attributes.setDim(false);
        attributes.setItalic(true);
        attributes.setUnderline(true);

        REQUIRE(attributes.isBoldSpecified());
        REQUIRE(attributes.isBold());
        REQUIRE(attributes.isDimSpecified());
        REQUIRE_FALSE(attributes.isDim());
        REQUIRE(attributes.isItalic());
        REQUIRE(attributes.isUnderline());

        attributes.setBoldInherited();
        attributes.setUnderlineInherited();

        REQUIRE_FALSE(attributes.isBoldSpecified());
        REQUIRE_FALSE(attributes.isUnderlineSpecified());
        REQUIRE(attributes.isDimSpecified());
        REQUIRE(attributes.isItalicSpecified());
    }

    void testResolvedWithOverwritesOnlySpecifiedFlags() {
        auto base = BlockAttributes::reset();
        base.setBold(true);
        base.setItalic(true);
        base.setHidden(true);

        auto overlay = BlockAttributes{};
        overlay.setBold(false);
        overlay.setUnderline(true);

        const auto resolved = overlay.withBase(base);

        REQUIRE(resolved.isBoldSpecified());
        REQUIRE_FALSE(resolved.isBold());
        REQUIRE(resolved.isItalic());
        REQUIRE(resolved.isUnderline());
        REQUIRE(resolved.isHidden());
        REQUIRE_FALSE(resolved.isBlink());
    }

    void testEqualityAndHashReflectEnabledAndSpecifiedBits() {
        auto left = BlockAttributes{};
        left.setBold(true);
        left.setStrikethrough(false);

        auto equal = left;
        auto different = left;
        different.setBoldInherited();

        REQUIRE_EQUAL(left, equal);
        REQUIRE_EQUAL(left, equal);
        REQUIRE_NOT_EQUAL(left, different);
        REQUIRE_EQUAL(left.hash(), std::hash<BlockAttributes>{}(left));
        REQUIRE_NOT_EQUAL(left.hash(), different.hash());
    }

    void testWithFlagCreatesUpdatedCopiesWithoutMutatingTheSource() {
        const auto original = BlockAttributes{};
        const auto bold = original.withFlag(BlockAttributes::Bold, true);
        const auto noBold = bold.withFlag(BlockAttributes::Bold, false);

        REQUIRE_FALSE(original.isBoldSpecified());
        REQUIRE(bold.isBoldSpecified());
        REQUIRE(bold.isBold());
        REQUIRE(noBold.isBoldSpecified());
        REQUIRE_FALSE(noBold.isBold());
    }
};
