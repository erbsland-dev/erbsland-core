// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

#include <functional>

TESTED_TARGETS(BlockStyle)
class BlockStyleTest final : public el::UnitTest {
public:
    void testConstructionAccessorsAndHashReflectColorAndAttributes() {
        auto attributes = BlockAttributes{};
        attributes.setBold(true);

        const auto inherited = BlockStyle{};
        const auto styled = BlockStyle{Color{fg::Yellow, bg::Blue}, attributes};

        REQUIRE_EQUAL(inherited.color(), Color{});
        REQUIRE_EQUAL(inherited.attributes(), BlockAttributes{});
        REQUIRE_EQUAL(styled.color(), Color(fg::Yellow, bg::Blue));
        REQUIRE(styled.attributes().isBold());
        REQUIRE_EQUAL(styled.hash(), std::hash<BlockStyle>{}(styled));
    }

    void testSettersUpdateIndividualStyleParts() {
        auto style = BlockStyle{};
        auto attributes = BlockAttributes{};
        attributes.setUnderline(true);

        style.setFg(fg::Green);
        style.setBg(bg::Black);
        style.setAttributes(attributes);

        REQUIRE_EQUAL(style.fg(), Foreground{fg::Green});
        REQUIRE_EQUAL(style.bg(), Background{bg::Black});
        REQUIRE(style.attributes().isUnderline());
    }

    void testOverlayAndBaseCombineColorAndAttributesTogether() {
        auto baseAttributes = BlockAttributes{};
        baseAttributes.setBold(true);
        auto overlayAttributes = BlockAttributes{};
        overlayAttributes.setBold(false);
        overlayAttributes.setItalic(true);

        const auto base = BlockStyle{Color{fg::Green, bg::Blue}, baseAttributes};
        const auto overlay = BlockStyle{Color{fg::Inherited, bg::Black}, overlayAttributes};

        const auto overlaid = base.withOverlay(overlay);
        REQUIRE_EQUAL(overlaid.color(), Color(fg::Green, bg::Black));
        REQUIRE_FALSE(overlaid.attributes().isBold());
        REQUIRE(overlaid.attributes().isItalic());

        const auto reappliedBase = overlay.withBase(base);
        REQUIRE_EQUAL(reappliedBase, overlaid);
    }
};
