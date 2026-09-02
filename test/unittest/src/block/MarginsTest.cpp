// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/Margins.hpp>
#include <erbsland/block/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(Margins)
class MarginsTest final : public el::UnitTest {
public:
    void testConstructorsAndAccessors() {
        using el::block::Margins;

        const auto uniformMargins = Margins{5};
        const auto opposingMargins = Margins{2, 7};
        REQUIRE_EQUAL(uniformMargins, (Margins{5, 5, 5, 5}));
        REQUIRE_EQUAL(opposingMargins, (Margins{7, 2, 7, 2}));

        auto margins = Margins{1, 2, 3, 4};
        REQUIRE_EQUAL(margins.top(), 1);
        REQUIRE_EQUAL(margins.right(), 2);
        REQUIRE_EQUAL(margins.bottom(), 3);
        REQUIRE_EQUAL(margins.left(), 4);

        margins.setTop(5);
        margins.setRight(6);
        margins.setBottom(7);
        margins.setLeft(8);
        REQUIRE_EQUAL(margins, (Margins{5, 6, 7, 8}));
    }

    void testSideAndOrientationHelpers() {
        using el::block::MarginPair;
        using el::block::Margins;
        using el::geometry::Orientation;

        const auto margins = Margins{1, 2, 3, 4};

        REQUIRE_EQUAL(margins[Margins::Side::Top], 1);
        REQUIRE_EQUAL(margins[Margins::Side::Right], 2);
        REQUIRE_EQUAL(margins[Margins::Side::Bottom], 3);
        REQUIRE_EQUAL(margins[Margins::Side::Left], 4);
        REQUIRE_EQUAL(margins.horizontal(), (MarginPair{4, 2}));
        REQUIRE_EQUAL(margins.vertical(), (MarginPair{1, 3}));
        REQUIRE_EQUAL(margins.component(Orientation::Horizontal), (MarginPair{4, 2}));
        REQUIRE_EQUAL(margins.component(Orientation::Vertical), (MarginPair{1, 3}));

        auto changed = margins;
        changed.setHorizontal(MarginPair{8, 9});
        changed.setVertical(MarginPair{6, 7});
        REQUIRE_EQUAL(changed, (Margins{6, 9, 7, 8}));
    }

    void testExtentsSpacingAndDeltas() {
        using el::block::Margins;
        using el::block::Size;
        using el::geometry::Orientation;

        const auto margins = Margins{-3, 4, 5, 6};

        REQUIRE_EQUAL(margins.horizontal().extent(), 10);
        REQUIRE_EQUAL(margins.vertical().extent(), 5);
        REQUIRE_EQUAL(margins.extent(), (Size{10, 5}));
        REQUIRE_EQUAL(margins.spacing(), (Size{6, 5}));
        REQUIRE_EQUAL(margins.horizontal().spacing(), 6);
        REQUIRE_EQUAL(margins.vertical().spacing(), 5);
        REQUIRE_EQUAL(margins.horizontal().delta(), 10);
        REQUIRE_EQUAL(margins.vertical().delta(), 2);
    }

    void testExpansionAndLimiting() {
        using el::block::Margins;
        using el::geometry::Orientation;

        auto margins = Margins{1, 5, -2, 4};
        margins.expandTo(Margins{3, 2, 8, 1});
        REQUIRE_EQUAL(margins, (Margins{3, 5, 8, 4}));

        margins.limitTo(Margins{2, 4, 6, 9}, Orientation::Horizontal);
        REQUIRE_EQUAL(margins, (Margins{3, 4, 8, 4}));

        const auto expanded = Margins{-1, 2, -3, 4}.expandedPositive();
        const auto limited = Margins{1, 5, 3, 7}.limitedWith(Margins{4, 2, 9, 6});
        REQUIRE_EQUAL(expanded, (Margins{0, 2, 0, 4}));
        REQUIRE_EQUAL(limited, (Margins{1, 2, 3, 6}));
    }

    void testUnaryNegation() {
        using el::block::Margins;

        const auto negated = -Margins(1, 2, 3, 4);
        REQUIRE_EQUAL(negated, (Margins{-1, -2, -3, -4}));
    }
};
