// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/vr/VersionMask.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <limits>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(VersionMask)
class VersionMaskTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
    using ConfVersionRange = el::conf::impl::ConfVersionRange;
    using VersionMask = el::conf::impl::VersionMask;

public:
    void testVersionRangeDefault() {
        ConfVersionRange r; // default constructed
        REQUIRE_EQUAL(r.first, 0);
        REQUIRE_EQUAL(r.last, 0);
    }

    void testVersionRangeSingleValue() {
        ConfVersionRange r{5};
        REQUIRE_EQUAL(r.first, 5);
        REQUIRE_EQUAL(r.last, 5);

        ConfVersionRange rn{-3};
        REQUIRE_EQUAL(rn.first, 0);
        REQUIRE_EQUAL(rn.last, 0);
    }

    void testVersionRangeTwoValues() {
        ConfVersionRange r1{10, 3};
        REQUIRE_EQUAL(r1.first, 3);
        REQUIRE_EQUAL(r1.last, 10);

        ConfVersionRange r2{-5, 2};
        REQUIRE_EQUAL(r2.first, 0);
        REQUIRE_EQUAL(r2.last, 2);

        ConfVersionRange r3{-5, -1};
        REQUIRE_EQUAL(r3.first, 0);
        REQUIRE_EQUAL(r3.last, 0);
    }

    void testVersionRangeAll() {
        ConfVersionRange all = ConfVersionRange::all();
        REQUIRE_EQUAL(all.first, 0);
        REQUIRE_EQUAL(all.last, std::numeric_limits<Integer>::max());
    }

    void testMaskDefaultMatchesAll() {
        VersionMask m; // default mask matches all versions
        REQUIRE(m.matches(0));
        REQUIRE(m.matches(1));
        REQUIRE(m.matches(123456));
        REQUIRE(m.matches(std::numeric_limits<Integer>::max()));
    }

    void testMaskSingleRangeMatches() {
        VersionMask m{ConfVersionRange{5, 7}};
        REQUIRE_FALSE(m.matches(4));
        REQUIRE(m.matches(5));
        REQUIRE(m.matches(6));
        REQUIRE(m.matches(7));
        REQUIRE_FALSE(m.matches(8));
    }

    void testConstructFromIntegers() {
        // Empty -> empty mask (matches nothing)
        VersionMask e = VersionMask::fromIntegers({});
        REQUIRE(e.isEmpty());
        REQUIRE_FALSE(e.matches(0));

        // Unsorted with duplicates and negatives; adjacency should coalesce
        VersionMask m = VersionMask::fromIntegers({10, -1, 3, 2, 2, 1, 0, 4});
        // Should match 0-4 and 10
        REQUIRE(m.matches(0));
        REQUIRE(m.matches(1));
        REQUIRE(m.matches(2));
        REQUIRE(m.matches(3));
        REQUIRE(m.matches(4));
        REQUIRE_FALSE(m.matches(5));
        REQUIRE_FALSE(m.matches(9));
        REQUIRE(m.matches(10));
        REQUIRE_FALSE(m.matches(11));
    }

    void testConstructFromRanges() {
        // Empty -> empty mask
        VersionMask e = VersionMask::empty();
        REQUIRE(e.isEmpty());
        REQUIRE_FALSE(e.matches(0));

        // Overlapping and adjacent ranges should coalesce
        VersionMask m = VersionMask::fromRanges(
            {ConfVersionRange{5, 7}, ConfVersionRange{1, 3}, ConfVersionRange{4, 4}, ConfVersionRange{8, 8}});
        // Combined should be 1-8
        REQUIRE_FALSE(m.matches(0));
        REQUIRE(m.matches(1));
        REQUIRE(m.matches(3));
        REQUIRE(m.matches(4));
        REQUIRE(m.matches(7));
        REQUIRE(m.matches(8));
        REQUIRE_FALSE(m.matches(9));

        // Negative and reversed inputs are normalized by ConfVersionRange
        VersionMask m2 = VersionMask::fromRanges({ConfVersionRange{-5, 2}, ConfVersionRange{10, 7}});
        REQUIRE(m2.matches(0));
        REQUIRE(m2.matches(2));
        REQUIRE_FALSE(m2.matches(3));
        REQUIRE(m2.matches(7));
        REQUIRE(m2.matches(10));
        REQUIRE_FALSE(m2.matches(11));
    }

    void testUnionMergeOverlapAdjacencyDisjoint() {
        // (1-3, 7-10) | (4-8, 12-12) => (1-10, 12-12)
        VersionMask a = VersionMask::fromRanges({ConfVersionRange{1, 3}, ConfVersionRange{7, 10}});
        VersionMask b = VersionMask::fromRanges({ConfVersionRange{4, 8}, ConfVersionRange{12, 12}});
        VersionMask u = a | b;

        REQUIRE_FALSE(a.matches(0));
        REQUIRE_FALSE(b.matches(0));
        REQUIRE_FALSE(u.matches(0));
        REQUIRE(u.matches(1));
        REQUIRE(u.matches(3));
        REQUIRE(u.matches(4));
        REQUIRE(u.matches(6));
        REQUIRE(u.matches(10));
        REQUIRE_FALSE(u.matches(11));
        REQUIRE(u.matches(12));
        REQUIRE_FALSE(u.matches(13));

        // Adjacency should coalesce: (1-3) | (4-6) => (1-6)
        VersionMask adj = VersionMask::fromRanges({ConfVersionRange{1, 3}, ConfVersionRange{4, 6}});
        REQUIRE(adj.matches(1));
        REQUIRE(adj.matches(3));
        REQUIRE(adj.matches(4));
        REQUIRE(adj.matches(6));
        REQUIRE_FALSE(adj.matches(0));
        REQUIRE_FALSE(adj.matches(7));
    }

    void testIntersection() {
        VersionMask a = VersionMask::fromRanges({ConfVersionRange{1, 3}, ConfVersionRange{7, 10}});
        VersionMask b = VersionMask::fromRanges({ConfVersionRange{4, 8}, ConfVersionRange{12, 12}});
        VersionMask i = a & b;

        REQUIRE_FALSE(i.matches(6));
        REQUIRE(i.matches(7));
        REQUIRE(i.matches(8));
        REQUIRE_FALSE(i.matches(9));
        REQUIRE_FALSE(i.matches(12));

        // Boundary intersection + adjacency coalescing: (1-4) & (3-8) => (3-4)
        VersionMask a2 = VersionMask::fromRanges({ConfVersionRange{1, 4}});
        VersionMask b2 = VersionMask::fromRanges({ConfVersionRange{3, 8}});
        VersionMask i2 = a2 & b2;
        REQUIRE_FALSE(i2.matches(2));
        REQUIRE(i2.matches(3));
        REQUIRE(i2.matches(4));
        REQUIRE_FALSE(i2.matches(5));
    }

    void testToTextAndHelpers() {
        // Empty mask
        VersionMask e = VersionMask::empty();
        REQUIRE(e.isEmpty());
        REQUIRE_FALSE(e.isAny());
        REQUIRE_EQUAL(e.toText(), "none"_el);

        // Any mask (default constructed)
        VersionMask any;
        REQUIRE(any.isAny());
        REQUIRE_FALSE(any.isEmpty());
        REQUIRE_EQUAL(any.toText(), "any"_el);

        // <=N form
        VersionMask le = VersionMask::fromRanges({ConfVersionRange{0, 5}});
        REQUIRE_EQUAL(le.toText(), "<=5"_el);
        REQUIRE(le.matches(0));
        REQUIRE(le.matches(5));
        REQUIRE_FALSE(le.matches(6));

        // >=M form
        const auto maxI = std::numeric_limits<Integer>::max();
        VersionMask ge = VersionMask::fromRanges({ConfVersionRange{7, maxI}});
        REQUIRE_EQUAL(ge.toText(), ">=7"_el);
        REQUIRE_FALSE(ge.matches(6));
        REQUIRE(ge.matches(7));
        REQUIRE(ge.matches(maxI));

        // Singleton
        VersionMask single = VersionMask::fromRanges({ConfVersionRange{5, 5}});
        REQUIRE_EQUAL(single.toText(), "5"_el);
        REQUIRE_FALSE(single.matches(4));
        REQUIRE(single.matches(5));
        REQUIRE_FALSE(single.matches(6));

        // General list formatting
        VersionMask list = VersionMask::fromRanges({ConfVersionRange{1, 3}, ConfVersionRange{7, 10}});
        REQUIRE_EQUAL(list.toText(), "1-3, 7-10"_el);
    }

    void testIdentityOperators() {
        VersionMask e = VersionMask::empty();
        VersionMask any; // matches all
        VersionMask a = VersionMask::fromRanges({ConfVersionRange{2, 4}});

        // empty | a == a
        const auto unionMask = e | a;
        const auto unionRangeCount = unionMask.ranges().size();
        const auto expectedRangeCount = a.ranges().size();
        REQUIRE_EQUAL(unionRangeCount, expectedRangeCount);
        REQUIRE((e | a).matches(3));
        // empty & a == empty
        REQUIRE((e & a).isEmpty());

        // any | a == any
        REQUIRE(((any | a).isAny()));
        // any & a == a
        VersionMask i = any & a;
        REQUIRE_FALSE(i.isAny());
        REQUIRE_FALSE(i.isEmpty());
        REQUIRE(i.matches(2));
        REQUIRE(i.matches(3));
        REQUIRE(i.matches(4));
        REQUIRE_FALSE(i.matches(5));

        // In-place variants
        VersionMask x = a;
        x |= e;   // no effect
        REQUIRE(x.matches(3));
        x &= any; // no effect
        REQUIRE(x.matches(3));
        x |= any; // becomes any
        REQUIRE(x.isAny());
        x &= e;   // becomes empty
        REQUIRE(x.isEmpty());
    }

    void testBoundaryValues() {
        const auto maxI = std::numeric_limits<Integer>::max();

        // Range [0, max] is any
        auto any2 = VersionMask::fromRanges({ConfVersionRange{0, maxI}});
        REQUIRE(any2.isAny());
        REQUIRE_EQUAL(any2.toText(), "any"_el);
        REQUIRE(any2.matches(0));
        REQUIRE(any2.matches(maxI));

        // Range [max, max]
        auto top = VersionMask::fromRanges({ConfVersionRange{maxI, maxI}});
        REQUIRE(top.matches(maxI));
        if (maxI > 0) {
            REQUIRE_FALSE(top.matches(maxI - 1));
        }

        // minimum_version-like mask
        auto minv = VersionMask::fromRanges({ConfVersionRange{5, maxI}});
        REQUIRE_FALSE(minv.matches(4));
        REQUIRE(minv.matches(5));
        REQUIRE(minv.matches(maxI));

        // maximum_version-like mask
        auto maxv = VersionMask::fromRanges({ConfVersionRange{0, 0}});
        REQUIRE(maxv.matches(0));
        REQUIRE_FALSE(maxv.matches(1));
        REQUIRE_EQUAL(maxv.toText(), el::text::String{"0"_el});
    }

    void testRangesAccessorNormalization() {
        VersionMask m = VersionMask::fromIntegers({0, 2, 1, 3, 4, 10});
        const auto &rs = m.ranges();
        REQUIRE_EQUAL(rs.size(), 2);
        REQUIRE_EQUAL(rs[0].first, 0);
        REQUIRE_EQUAL(rs[0].last, 4);
        REQUIRE_EQUAL(rs[1].first, 10);
        REQUIRE_EQUAL(rs[1].last, 10);
    }

    void testConstructorCornerCases() {
        // Default ConfVersionRange -> 0-0
        VersionMask m{ConfVersionRange{}};
        REQUIRE(m.matches(0));
        REQUIRE_FALSE(m.matches(1));
        REQUIRE_EQUAL(m.toText(), el::text::String{"0"_el});

        // fromRanges with empty input -> empty mask
        auto e = VersionMask::fromRanges(std::vector<ConfVersionRange>{});
        REQUIRE(e.isEmpty());
        REQUIRE_EQUAL(e.toText(), el::text::String{"none"_el});
    }

    void testNotOperator() {
        const auto maxI = std::numeric_limits<Integer>::max();

        // !any == empty
        VersionMask any; // matches all
        VersionMask nAny = !any;
        REQUIRE(nAny.isEmpty());

        // !empty == any
        VersionMask empty = VersionMask::empty();
        VersionMask nEmpty = !empty;
        REQUIRE(nEmpty.isAny());

        // Complement of [0,5] -> [6, max]
        VersionMask upTo5 = VersionMask::fromRanges({ConfVersionRange{0, 5}});
        VersionMask c1 = !upTo5;
        REQUIRE_FALSE(c1.matches(5));
        REQUIRE(c1.matches(6));
        REQUIRE(c1.matches(maxI));
        // textual form should be ">=6"
        REQUIRE_EQUAL(c1.toText(), ">=6"_el);

        // Complement of [5, max] -> [0,4]
        VersionMask from5 = VersionMask::fromRanges({ConfVersionRange{5, maxI}});
        VersionMask c2 = !from5;
        REQUIRE(c2.matches(0));
        REQUIRE(c2.matches(4));
        REQUIRE_FALSE(c2.matches(5));
        REQUIRE_EQUAL(c2.toText(), "<=4"_el);

        // Complement of (1-3, 7-10) -> (0-0, 4-6, 11-max)
        VersionMask complex = VersionMask::fromRanges({ConfVersionRange{1, 3}, ConfVersionRange{7, 10}});
        VersionMask cc = !complex;
        REQUIRE(cc.matches(0));
        REQUIRE_FALSE(cc.matches(1));
        REQUIRE_FALSE(cc.matches(2));
        REQUIRE_FALSE(cc.matches(3));
        REQUIRE(cc.matches(4));
        REQUIRE(cc.matches(6));
        REQUIRE_FALSE(cc.matches(7));
        REQUIRE_FALSE(cc.matches(10));
        REQUIRE(cc.matches(11));
        REQUIRE(cc.matches(maxI));
        REQUIRE(cc.toText().find("0"_el).isValid());
    }
};
