// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/vr/ConfKey.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/CaseSensitivity.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>

using namespace el::conf;
using namespace el::text::literals;
using namespace el::conf::impl;
using el::text::CaseSensitivity;

TESTED_TARGETS(ConfKey)
class ConfKeyTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void testDefaultConstruction() {
        ConfKey key;
        REQUIRE_EQUAL(key.size(), 0);
        REQUIRE(key.elements().isEmpty());
        REQUIRE(key.toText().isEmpty());
    }

    void testSingleElementConstructionAndAccess() {
        ConfKey key{"Alpha"_el};

        REQUIRE_EQUAL(key.size(), 1);
        const auto elementCount = key.elements().count().toSizeT();
        REQUIRE_EQUAL(elementCount, 1);
        REQUIRE_EQUAL(key.element(0), "Alpha"_el);
        REQUIRE_EQUAL(key.toText(), "Alpha"_el);
    }

    void testMultipleElementConstructionAndAccess() {
        ConfKey key{el::text::StringList{"Alpha"_el, "Beta"_el, "Gamma"_el}};

        REQUIRE_EQUAL(key.size(), 3);
        const auto elementCount = key.elements().count().toSizeT();
        REQUIRE_EQUAL(elementCount, 3);
        REQUIRE_EQUAL(key.element(0), "Alpha"_el);
        REQUIRE_EQUAL(key.element(1), "Beta"_el);
        REQUIRE_EQUAL(key.element(2), "Gamma"_el);
        REQUIRE_EQUAL(key.toText(), "Alpha,Beta,Gamma"_el);
    }

    void testElementOutOfRangeReturnsEmptyString() {
        ConfKey key{el::text::StringList{"Alpha"_el, "Beta"_el}};

        REQUIRE(key.element(2).isEmpty());
        REQUIRE(key.element(99).isEmpty());

        ConfKey emptyKey;
        REQUIRE(emptyKey.element(0).isEmpty());
    }

    void testCompareAllElementsCaseSensitive() {
        ConfKey left{el::text::StringList{"Alpha"_el, "Beta"_el}};
        ConfKey same{el::text::StringList{"Alpha"_el, "Beta"_el}};
        ConfKey differentCase{el::text::StringList{"alpha"_el, "Beta"_el}};
        ConfKey differentValue{el::text::StringList{"Alpha"_el, "Gamma"_el}};

        REQUIRE(left.isEqual(same, CaseSensitivity::CaseSensitive));
        REQUIRE_FALSE(left.isEqual(differentCase, CaseSensitivity::CaseSensitive));
        REQUIRE_FALSE(left.isEqual(differentValue, CaseSensitivity::CaseSensitive));
    }

    void testCompareAllElementsCaseInsensitive() {
        ConfKey left{el::text::StringList{"Alpha"_el, "Beta"_el}};
        ConfKey right{el::text::StringList{"ALPHA"_el, "beta"_el}};

        REQUIRE(left.isEqual(right, CaseSensitivity::CaseInsensitive));
    }

    void testCompareSingleElementByIndex() {
        ConfKey left{el::text::StringList{"Alpha"_el, "Beta"_el}};
        ConfKey right{el::text::StringList{"ALPHA"_el, "Beta"_el}};

        REQUIRE_FALSE(left.isEqual(right, CaseSensitivity::CaseSensitive, 0));
        REQUIRE(left.isEqual(right, CaseSensitivity::CaseInsensitive, 0));
        REQUIRE(left.isEqual(right, CaseSensitivity::CaseSensitive, 1));
    }

    void testCompareSingleElementOutOfRange() {
        ConfKey one{"Alpha"_el};
        ConfKey oneAndEmpty{el::text::StringList{"Alpha"_el, ""_el}};
        ConfKey oneAndValue{el::text::StringList{"Alpha"_el, "Beta"_el}};

        REQUIRE(one.isEqual(oneAndEmpty, CaseSensitivity::CaseSensitive, 1));
        REQUIRE_FALSE(one.isEqual(oneAndValue, CaseSensitivity::CaseSensitive, 1));
        REQUIRE_FALSE(one.isEqual(oneAndEmpty, CaseSensitivity::CaseSensitive));
        REQUIRE_FALSE(one.isEqual(oneAndValue, CaseSensitivity::CaseSensitive));
    }

    void testFormatter() {
        const ConfKey key{el::text::StringList{"a"_el, "b"_el}};
        const auto text = std::format("{}", key);
        REQUIRE_EQUAL(text, "a,b");
    }
};
