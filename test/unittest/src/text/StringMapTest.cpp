// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringCIHashMap.hpp>
#include <erbsland/text/StringCIMap.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringHashMap.hpp>
#include <erbsland/text/StringMap.hpp>
#include <erbsland/text/u16/U16StringCIHashMap.hpp>
#include <erbsland/text/u16/U16StringCIMap.hpp>
#include <erbsland/text/u16/U16StringHashMap.hpp>
#include <erbsland/text/u16/U16StringMap.hpp>
#include <erbsland/text/u32/U32StringCIHashMap.hpp>
#include <erbsland/text/u32/U32StringCIMap.hpp>
#include <erbsland/text/u32/U32StringHashMap.hpp>
#include <erbsland/text/u32/U32StringMap.hpp>
#include <erbsland/text/u8/U8StringCIHashMap.hpp>
#include <erbsland/text/u8/U8StringCIMap.hpp>
#include <erbsland/text/u8/U8StringHashMap.hpp>
#include <erbsland/text/u8/U8StringMap.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>
#include <string>

using namespace el::text;

TESTED_TARGETS(
    StringMap StringCIMap StringHashMap StringCIHashMap U8StringMap U8StringCIMap U8StringHashMap U8StringCIHashMap
        U16StringMap U16StringCIMap U16StringHashMap U16StringCIHashMap U32StringMap U32StringCIMap U32StringHashMap
            U32StringCIHashMap)
class StringMapTest final : public el::UnitTest {
public:
    void testAliasesAndKeyStringLists() {
        using namespace el::text::literals;

        const auto common = StringMap<int>{{{"beta"_els, 2}, {"alpha"_els, 1}}};
        REQUIRE_EQUAL(StringConverter{common.first().first}.toStdString(), std::string{"alpha"});
        REQUIRE_EQUAL(StringConverter{common.toKeyStringList().join("|"_els)}.toStdString(), std::string{"alpha|beta"});
        REQUIRE_EQUAL(
            StringConverter{common.toKeyStringSet().toStringList().join("|"_els)}.toStdString(),
            std::string{"alpha|beta"});
        REQUIRE(common.toKeyStringHashSet().contains("alpha"_els));

        const auto u8 = U8StringMap<int>{{{"a"_els, 1}}};
        const auto u16 = U16StringMap<int>{{{u"a"_els, 1}}};
        const auto u32 = U32StringMap<int>{{{U"a"_els, 1}}};
        const auto hash = StringHashMap<int>{{{"a"_els, 1}}};
        const auto u16Hash = U16StringHashMap<int>{{{u"a"_els, 1}}};
        const auto u32Hash = U32StringHashMap<int>{{{U"a"_els, 1}}};

        REQUIRE_EQUAL(u8.get("a"_els, 0), 1);
        REQUIRE_EQUAL(u16.get(u"a"_els, 0), 1);
        REQUIRE_EQUAL(u32.get(U"a"_els, 0), 1);
        REQUIRE_EQUAL(hash.get("a"_els, 0), 1);
        REQUIRE_EQUAL(u16Hash.get(u"a"_els, 0), 1);
        REQUIRE_EQUAL(u32Hash.get(U"a"_els, 0), 1);
    }

    void testCaseInsensitiveOrderedMapReplacesVisibleKey() {
        using namespace el::text::literals;

        auto map = StringCIMap<int>{};
        map.set(u8"ÄbcK"_els, 1);

        REQUIRE(map.contains(u8"äbcK"_els));
        REQUIRE(!map.tryInsert(u8"äbcK"_els, 2));
        REQUIRE_EQUAL(map.get(u8"äbcK"_els, 0), 1);
        REQUIRE_EQUAL(StringConverter{map.toKeyStringList().first()}.toStdU8String(), std::u8string{u8"ÄbcK"});

        REQUIRE(map.tryReplace(u8"äbcK"_els, 3));
        REQUIRE_EQUAL(map.get(u8"ÄbcK"_els, 0), 3);
        REQUIRE_EQUAL(StringConverter{map.toKeyStringList().first()}.toStdU8String(), std::u8string{u8"äbcK"});

        const auto u16 = U16StringCIMap<int>{{{u"A"_els, 1}}};
        const auto u32 = U32StringCIMap<int>{{{U"A"_els, 1}}};
        REQUIRE_EQUAL(u16.get(u"a"_els, 0), 1);
        REQUIRE_EQUAL(u32.get(U"a"_els, 0), 1);
    }

    void testCaseInsensitiveHashMapReplacesVisibleKey() {
        using namespace el::text::literals;

        auto map = StringCIHashMap<int>{};
        map.set(u8"ÄbcK"_els, 1);

        REQUIRE(map.contains(u8"äbcK"_els));
        REQUIRE(!map.tryInsert(u8"äbcK"_els, 2));
        REQUIRE_EQUAL(map.get(u8"äbcK"_els, 0), 1);
        REQUIRE_EQUAL(StringConverter{map.toKeyStringList().first()}.toStdU8String(), std::u8string{u8"ÄbcK"});

        map.set(u8"äbcK"_els, 3);
        REQUIRE_EQUAL(map.get(u8"ÄbcK"_els, 0), 3);
        REQUIRE_EQUAL(StringConverter{map.toKeyStringList().first()}.toStdU8String(), std::u8string{u8"äbcK"});

        const auto u8 = U8StringCIHashMap<int>{{{"A"_els, 1}}};
        const auto u16 = U16StringCIHashMap<int>{{{u"A"_els, 1}}};
        const auto u32 = U32StringCIHashMap<int>{{{U"A"_els, 1}}};
        REQUIRE_EQUAL(u8.get("a"_els, 0), 1);
        REQUIRE_EQUAL(u16.get(u"a"_els, 0), 1);
        REQUIRE_EQUAL(u32.get(U"a"_els, 0), 1);
    }

    void testOrderedMapViewKeyOperations() {
        using namespace el::text::literals;

        auto map = StringMap<int>{{{"alpha"_els, 1}, {"beta"_els, 2}}};

        REQUIRE(map.contains("alpha"_el));
        REQUIRE(map.contains("beta"_elv));
        REQUIRE(map.get("alpha"_el).has_value());
        REQUIRE_EQUAL(map.get("alpha"_el).value(), 1);
        REQUIRE_EQUAL(map.get("missing"_elv, 9), 9);

        map.remove("alpha"_el);
        REQUIRE_FALSE(map.contains("alpha"_elv));
        REQUIRE(map.contains("beta"_el));

        const auto removed = map.removed("beta"_elv);
        REQUIRE_FALSE(removed.contains("beta"_el));
        REQUIRE(map.contains("beta"_el));
        REQUIRE_EQUAL(map.take("beta"_elv), 2);
        REQUIRE(map.count().isZero());

        REQUIRE(map.tryInsert("gamma"_el, 3));
        REQUIRE_FALSE(map.tryInsert("gamma"_elv, 4));
        REQUIRE_EQUAL(map.get("gamma"_elv, 0), 3);
        REQUIRE(map.tryReplace("gamma"_elv, 5));
        REQUIRE_EQUAL(map.get("gamma"_el, 0), 5);
        REQUIRE_FALSE(map.tryReplace("missing"_el, 6));

        map.set("delta"_el, 7);
        REQUIRE_EQUAL(map.get("delta"_elv, 0), 7);

        const auto u16 = U16StringMap<int>{{{u"alpha"_els, 1}}};
        const auto u32 = U32StringMap<int>{{{U"alpha"_els, 1}}};
        REQUIRE(u16.contains(u"alpha"_el));
        REQUIRE(u32.contains(U"alpha"_elv));
        REQUIRE_EQUAL(u16.get(u"alpha"_elv, 0), 1);
        REQUIRE_EQUAL(u32.get(U"alpha"_el, 0), 1);
    }

    void testHashMapViewKeyOperations() {
        using namespace el::text::literals;

        auto map = StringHashMap<int>{{{"alpha"_els, 1}, {"beta"_els, 2}}};

        REQUIRE(map.contains("alpha"_el));
        REQUIRE(map.contains("beta"_elv));
        REQUIRE(map.get("alpha"_el).has_value());
        REQUIRE_EQUAL(map.get("alpha"_el).value(), 1);
        REQUIRE_EQUAL(map.get("missing"_elv, 9), 9);

        map.remove("alpha"_el);
        REQUIRE_FALSE(map.contains("alpha"_elv));
        REQUIRE(map.contains("beta"_el));

        const auto removed = map.removed("beta"_elv);
        REQUIRE_FALSE(removed.contains("beta"_el));
        REQUIRE(map.contains("beta"_el));
        REQUIRE_EQUAL(map.take("beta"_elv), 2);
        REQUIRE(map.count().isZero());

        REQUIRE(map.tryInsert("gamma"_el, 3));
        REQUIRE_FALSE(map.tryInsert("gamma"_elv, 4));
        REQUIRE_EQUAL(map.get("gamma"_elv, 0), 3);
        REQUIRE(map.tryReplace("gamma"_elv, 5));
        REQUIRE_EQUAL(map.get("gamma"_el, 0), 5);
        REQUIRE_FALSE(map.tryReplace("missing"_el, 6));

        map.set("delta"_el, 7);
        REQUIRE_EQUAL(map.get("delta"_elv, 0), 7);

        const auto u16 = U16StringHashMap<int>{{{u"alpha"_els, 1}}};
        const auto u32 = U32StringHashMap<int>{{{U"alpha"_els, 1}}};
        REQUIRE(u16.contains(u"alpha"_el));
        REQUIRE(u32.contains(U"alpha"_elv));
        REQUIRE_EQUAL(u16.get(u"alpha"_elv, 0), 1);
        REQUIRE_EQUAL(u32.get(U"alpha"_el, 0), 1);
    }

    void testCaseInsensitiveMapViewKeyOperations() {
        using namespace el::text::literals;

        auto ordered = StringCIMap<int>{};
        ordered.set(u8"ÄbcK"_el, 1);
        REQUIRE(ordered.contains(u8"äbck"_elv));
        REQUIRE_EQUAL(ordered.get(u8"äbck"_el, 0), 1);
        REQUIRE_FALSE(ordered.tryInsert(u8"äbck"_el, 2));
        REQUIRE(ordered.tryReplace(u8"äbck"_elv, 3));
        REQUIRE_EQUAL(ordered.get(u8"ÄBCK"_elv, 0), 3);
        REQUIRE_EQUAL(ordered.take(u8"ÄBCK"_el), 3);
        REQUIRE(ordered.count().isZero());

        auto hash = StringCIHashMap<int>{};
        hash.set(u8"ÄbcK"_el, 1);
        REQUIRE(hash.contains(u8"äbck"_elv));
        REQUIRE_EQUAL(hash.get(u8"äbck"_el, 0), 1);
        REQUIRE_FALSE(hash.tryInsert(u8"äbck"_el, 2));
        REQUIRE(hash.tryReplace(u8"äbck"_elv, 3));
        REQUIRE_EQUAL(hash.get(u8"ÄBCK"_elv, 0), 3);
        REQUIRE_EQUAL(hash.take(u8"ÄBCK"_el), 3);
        REQUIRE(hash.count().isZero());
    }

    void testStringHashes() {
        using namespace el::text::literals;

        const auto u8 = u8"ÄbcK"_els;
        const auto u16 = u"ÄbcK"_els;
        const auto u32 = U"ÄbcK"_els;

        REQUIRE_EQUAL(u8.toHash(), u8.toHash());
        REQUIRE_EQUAL(u8.toHash(), u8"ÄbcK"_elv.toHash());
        REQUIRE_EQUAL(u16.toHash(), u"ÄbcK"_elv.toHash());
        REQUIRE_EQUAL(u32.toHash(), U"ÄbcK"_elv.toHash());
        REQUIRE_EQUAL(u8.toHash(), u16.toHash());
        REQUIRE_EQUAL(u8.toHash(), u32.toHash());
        REQUIRE_EQUAL(u8.toHashCI(), u8"äbck"_els.toHashCI());
        REQUIRE_EQUAL(u16.toHashCI(), u"äbck"_els.toHashCI());
        REQUIRE_EQUAL(u32.toHashCI(), U"äbck"_els.toHashCI());
        REQUIRE_EQUAL(std::hash<U8String>{}(u8), u8.toHash());
        REQUIRE_EQUAL(std::hash<U16String>{}(u16), u16.toHash());
        REQUIRE_EQUAL(std::hash<U32String>{}(u32), u32.toHash());
        REQUIRE_EQUAL(std::hash<U8StringView>{}(u8"ÄbcK"_elv), u8.toHash());
        REQUIRE_EQUAL(std::hash<U16StringView>{}(u"ÄbcK"_elv), u16.toHash());
        REQUIRE_EQUAL(std::hash<U32StringView>{}(U"ÄbcK"_elv), u32.toHash());
    }
};
