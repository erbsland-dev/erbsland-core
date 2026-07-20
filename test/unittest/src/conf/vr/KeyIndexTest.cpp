// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/impl/vr/KeyIndex.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>
#include <erbsland/text/CaseSensitivity.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>

using namespace el::conf;
using namespace el::text::literals;
using namespace el::conf::impl;
using el::text::CaseSensitivity;

TESTED_TARGETS(KeyIndex)
class KeyIndexTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void testConstructorAndAccessors() {
        const auto name = Name::createRegular("my_index"_el);
        const KeyIndex index{name, CaseSensitivity::CaseInsensitive, 2};

        REQUIRE(index.name() == name);
        REQUIRE(index.caseSensitivity() == CaseSensitivity::CaseInsensitive);
    }

    void testConstructorRejectsZeroElementCount() {
        REQUIRE_THROWS_AS(ConfError, KeyIndex(Name::createRegular("index"_el), CaseSensitivity::CaseSensitive, 0));
    }

    void testEmptyIndexSingleElement() {
        const KeyIndex index{Name::createRegular("single"_el), CaseSensitivity::CaseSensitive, 1};

        REQUIRE_FALSE(index.hasKey("alpha"_el));
        REQUIRE_FALSE(index.hasKey(ConfKey{"alpha"_el}));
        REQUIRE_FALSE(index.hasKey("alpha"_el, 0));
        REQUIRE_FALSE(index.hasKey("alpha"_el, 1));
    }

    void testEmptyIndexMultiElement() {
        const KeyIndex index{Name::createRegular("multi"_el), CaseSensitivity::CaseSensitive, 3};

        REQUIRE_FALSE(index.hasKey("alpha,beta,gamma"_el));
        REQUIRE_FALSE(index.hasKey(ConfKey{el::text::StringList{"alpha"_el, "beta"_el, "gamma"_el}}));
        REQUIRE_FALSE(index.hasKey("alpha"_el, 0));
        REQUIRE_FALSE(index.hasKey("beta"_el, 1));
        REQUIRE_FALSE(index.hasKey("gamma"_el, 2));
        REQUIRE_FALSE(index.hasKey("alpha,beta"_el, 3));
    }

    void testSingleElementCaseSensitiveIndex() {
        KeyIndex index{Name::createRegular("single_cs"_el), CaseSensitivity::CaseSensitive, 1};

        REQUIRE(index.tryAddKey(ConfKey{"Alpha"_el}));
        REQUIRE_FALSE(index.tryAddKey(ConfKey{"Alpha"_el}));
        REQUIRE(index.tryAddKey(ConfKey{"alpha"_el}));

        REQUIRE(index.hasKey("Alpha"_el));
        REQUIRE(index.hasKey(ConfKey{"Alpha"_el}));
        REQUIRE(index.hasKey("alpha"_el));
        REQUIRE(index.hasKey(ConfKey{"alpha"_el}));
        REQUIRE_FALSE(index.hasKey("ALPHA"_el));
        REQUIRE(index.hasKey("Alpha"_el, 0));
        REQUIRE_FALSE(index.hasKey("ALPHA"_el, 0));
        REQUIRE_FALSE(index.hasKey("Alpha"_el, 1));
    }

    void testSingleElementCaseInsensitiveIndex() {
        KeyIndex index{Name::createRegular("single_ci"_el), CaseSensitivity::CaseInsensitive, 1};

        REQUIRE(index.tryAddKey(ConfKey{"Alpha"_el}));
        REQUIRE_FALSE(index.tryAddKey(ConfKey{"ALPHA"_el}));

        REQUIRE(index.hasKey("Alpha"_el));
        REQUIRE(index.hasKey("ALPHA"_el));
        REQUIRE(index.hasKey(ConfKey{"alpha"_el}));
        REQUIRE(index.hasKey("aLpHa"_el, 0));
    }

    void testMultiElementCaseSensitiveIndex() {
        KeyIndex index{Name::createRegular("multi_cs"_el), CaseSensitivity::CaseSensitive, 2};

        REQUIRE(index.tryAddKey(ConfKey{el::text::StringList{"Alpha"_el, "Beta"_el}}));
        REQUIRE_FALSE(index.tryAddKey(ConfKey{el::text::StringList{"Alpha"_el, "Beta"_el}}));
        REQUIRE(index.tryAddKey(ConfKey{el::text::StringList{"Alpha"_el, "beta"_el}}));
        REQUIRE(index.tryAddKey(ConfKey{el::text::StringList{"alpha"_el, "Beta"_el}}));

        REQUIRE(index.hasKey("Alpha,Beta"_el));
        REQUIRE(index.hasKey(ConfKey{el::text::StringList{"Alpha"_el, "Beta"_el}}));
        REQUIRE_FALSE(index.hasKey("ALPHA,BETA"_el));

        REQUIRE(index.hasKey("Alpha"_el, 0));
        REQUIRE(index.hasKey("alpha"_el, 0));
        REQUIRE(index.hasKey("Beta"_el, 1));
        REQUIRE(index.hasKey("beta"_el, 1));
        REQUIRE_FALSE(index.hasKey("ALPHA"_el, 0));
        REQUIRE_FALSE(index.hasKey("BETA"_el, 1));
        REQUIRE_FALSE(index.hasKey("Alpha"_el, 2));
    }

    void testMultiElementCaseInsensitiveIndex() {
        KeyIndex index{Name::createRegular("multi_ci"_el), CaseSensitivity::CaseInsensitive, 2};

        REQUIRE(index.tryAddKey(ConfKey{el::text::StringList{"Alpha"_el, "Beta"_el}}));
        REQUIRE_FALSE(index.tryAddKey(ConfKey{el::text::StringList{"ALPHA"_el, "beta"_el}}));

        REQUIRE(index.hasKey("alpha,beta"_el));
        REQUIRE(index.hasKey(ConfKey{el::text::StringList{"ALPHA"_el, "BETA"_el}}));
        REQUIRE(index.hasKey("aLpHa"_el, 0));
        REQUIRE(index.hasKey("BeTa"_el, 1));
    }

    void testTryAddKeyRejectsMismatchingElementCount() {
        KeyIndex index{Name::createRegular("count_check"_el), CaseSensitivity::CaseSensitive, 2};

        REQUIRE_THROWS_AS(ConfError, index.tryAddKey(ConfKey{}));
        REQUIRE_THROWS_AS(ConfError, index.tryAddKey(ConfKey{"only_one"_el}));
        REQUIRE_THROWS_AS(ConfError, index.tryAddKey(ConfKey{el::text::StringList{"one"_el, "two"_el, "three"_el}}));
    }

    void testHasKeyStringWithMismatchingElementCount() {
        KeyIndex index{Name::createRegular("string_count"_el), CaseSensitivity::CaseSensitive, 3};
        REQUIRE(index.tryAddKey(ConfKey{el::text::StringList{"a"_el, "b"_el, "c"_el}}));

        REQUIRE_FALSE(index.hasKey("a,b"_el));
        REQUIRE_FALSE(index.hasKey("a"_el));
        REQUIRE(index.hasKey("a,b,c"_el));
    }

    void testIndexWithOneSeveralAndLargeNumberOfKeys() {
        KeyIndex single{Name::createRegular("one"_el), CaseSensitivity::CaseSensitive, 1};
        REQUIRE(single.tryAddKey(ConfKey{"one"_el}));
        REQUIRE(single.hasKey("one"_el));

        KeyIndex several{Name::createRegular("several"_el), CaseSensitivity::CaseSensitive, 2};
        REQUIRE(several.tryAddKey(ConfKey{el::text::StringList{"a1"_el, "b1"_el}}));
        REQUIRE(several.tryAddKey(ConfKey{el::text::StringList{"a2"_el, "b2"_el}}));
        REQUIRE(several.tryAddKey(ConfKey{el::text::StringList{"a3"_el, "b3"_el}}));
        REQUIRE(several.hasKey("a2,b2"_el));
        REQUIRE(several.hasKey("a3"_el, 0));
        REQUIRE(several.hasKey("b1"_el, 1));
        REQUIRE_FALSE(several.hasKey("a4,b4"_el));

        KeyIndex many{Name::createRegular("many"_el), CaseSensitivity::CaseSensitive, 1};
        constexpr std::size_t keyCount = 2000;
        for (std::size_t i = 0; i < keyCount; ++i) {
            const el::text::String keyText{std::format("k_{:04}", i)};
            WITH_CONTEXT(keyText);
            REQUIRE(many.tryAddKey(ConfKey{keyText}));
        }

        REQUIRE(many.hasKey("k_0000"_el));
        REQUIRE(many.hasKey("k_1099"_el));
        REQUIRE(many.hasKey("k_1999"_el));
        REQUIRE_FALSE(many.hasKey("k_2000"_el));
    }
};
