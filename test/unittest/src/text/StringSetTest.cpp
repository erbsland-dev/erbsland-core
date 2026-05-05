// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringCIHashSet.hpp>
#include <erbsland/text/StringCISet.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringHashSet.hpp>
#include <erbsland/text/StringSet.hpp>
#include <erbsland/text/u16/U16StringCIHashSet.hpp>
#include <erbsland/text/u16/U16StringCISet.hpp>
#include <erbsland/text/u16/U16StringHashSet.hpp>
#include <erbsland/text/u16/U16StringSet.hpp>
#include <erbsland/text/u32/U32StringCIHashSet.hpp>
#include <erbsland/text/u32/U32StringCISet.hpp>
#include <erbsland/text/u32/U32StringHashSet.hpp>
#include <erbsland/text/u32/U32StringSet.hpp>
#include <erbsland/text/u8/U8StringCIHashSet.hpp>
#include <erbsland/text/u8/U8StringCISet.hpp>
#include <erbsland/text/u8/U8StringHashSet.hpp>
#include <erbsland/text/u8/U8StringSet.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>

using namespace el::text;

TESTED_TARGETS(
    StringSet StringCISet StringHashSet StringCIHashSet U8StringSet U8StringCISet U8StringHashSet U8StringCIHashSet
        U16StringSet U16StringCISet U16StringHashSet U16StringCIHashSet U32StringSet U32StringCISet U32StringHashSet
            U32StringCIHashSet)
class StringSetTest final : public el::UnitTest {
public:
    void testAliasesAndStringLists() {
        using namespace el::text::literals;

        const auto common = StringSet{"beta"_els, "alpha"_els, "alpha"_els};
        REQUIRE_EQUAL(StringConverter{common.toStringList().join("|"_els)}.toStdString(), std::string{"alpha|beta"});

        const auto u8 = U8StringSet{"a"_els};
        const auto u16 = U16StringSet{u"a"_els};
        const auto u32 = U32StringSet{U"a"_els};
        const auto hash = StringHashSet{"a"_els};
        const auto u16Hash = U16StringHashSet{u"a"_els};
        const auto u32Hash = U32StringHashSet{U"a"_els};

        REQUIRE(u8.contains("a"_els));
        REQUIRE(u16.contains(u"a"_els));
        REQUIRE(u32.contains(U"a"_els));
        REQUIRE(hash.contains("a"_els));
        REQUIRE(u16Hash.contains(u"a"_els));
        REQUIRE(u32Hash.contains(U"a"_els));
    }

    void testCaseInsensitiveOrderedSet() {
        using namespace el::text::literals;

        auto set = StringCISet{};
        set.insert(u8"ÄbcK"_els);

        REQUIRE(set.contains(u8"äbcK"_els));
        REQUIRE(!set.tryInsert(u8"äbcK"_els));
        REQUIRE_EQUAL(StringConverter{set.toStringList().first()}.toStdU8String(), std::u8string{u8"ÄbcK"});
        REQUIRE(set.compareCI(StringSet{u8"äbcK"_els}));
        REQUIRE(set.tryRemove(u8"äbcK"_els));
        REQUIRE(set.count().isZero());

        const auto u8 = U8StringCISet{"A"_els};
        const auto u16 = U16StringCISet{u"A"_els};
        const auto u32 = U32StringCISet{U"A"_els};
        REQUIRE(u8.contains("a"_els));
        REQUIRE(u16.contains(u"a"_els));
        REQUIRE(u32.contains(U"a"_els));
    }

    void testCaseInsensitiveHashSet() {
        using namespace el::text::literals;

        auto set = StringCIHashSet{};
        set.insert(u8"ÄbcK"_els);

        REQUIRE(set.contains(u8"äbcK"_els));
        REQUIRE(!set.tryInsert(u8"äbcK"_els));
        REQUIRE_EQUAL(StringConverter{set.toStringList().first()}.toStdU8String(), std::u8string{u8"ÄbcK"});
        REQUIRE(set.compareCI(StringHashSet{u8"äbcK"_els}));
        REQUIRE(set.tryRemove(u8"äbcK"_els));
        REQUIRE(set.count().isZero());

        const auto u8 = U8StringCIHashSet{"A"_els};
        const auto u16 = U16StringCIHashSet{u"A"_els};
        const auto u32 = U32StringCIHashSet{U"A"_els};
        REQUIRE(u8.contains("a"_els));
        REQUIRE(u16.contains(u"a"_els));
        REQUIRE(u32.contains(U"a"_els));
    }

    void testOrderedSetViewKeyOperations() {
        using namespace el::text::literals;

        auto set = StringSet{"alpha"_els, "beta"_els};

        REQUIRE(set.contains("alpha"_el));
        REQUIRE(set.contains("beta"_elv));

        set.remove("alpha"_el);
        REQUIRE_FALSE(set.contains("alpha"_elv));
        REQUIRE(set.contains("beta"_el));

        const auto removed = set.removed("beta"_elv);
        REQUIRE_FALSE(removed.contains("beta"_el));
        REQUIRE(set.contains("beta"_el));
        REQUIRE(set.tryRemove("beta"_elv));
        REQUIRE(set.count().isZero());

        set.insert("gamma"_el);
        REQUIRE(set.contains("gamma"_elv));
        REQUIRE_FALSE(set.tryInsert("gamma"_elv));
        REQUIRE(set.tryInsert("delta"_el));
        REQUIRE(set.contains("delta"_elv));

        const auto u16 = U16StringSet{u"alpha"_els};
        const auto u32 = U32StringSet{U"alpha"_els};
        REQUIRE(u16.contains(u"alpha"_el));
        REQUIRE(u32.contains(U"alpha"_elv));
    }

    void testHashSetViewKeyOperations() {
        using namespace el::text::literals;

        auto set = StringHashSet{"alpha"_els, "beta"_els};

        REQUIRE(set.contains("alpha"_el));
        REQUIRE(set.contains("beta"_elv));

        set.remove("alpha"_el);
        REQUIRE_FALSE(set.contains("alpha"_elv));
        REQUIRE(set.contains("beta"_el));

        const auto removed = set.removed("beta"_elv);
        REQUIRE_FALSE(removed.contains("beta"_el));
        REQUIRE(set.contains("beta"_el));
        REQUIRE(set.tryRemove("beta"_elv));
        REQUIRE(set.count().isZero());

        set.insert("gamma"_el);
        REQUIRE(set.contains("gamma"_elv));
        REQUIRE_FALSE(set.tryInsert("gamma"_elv));
        REQUIRE(set.tryInsert("delta"_el));
        REQUIRE(set.contains("delta"_elv));

        const auto u16 = U16StringHashSet{u"alpha"_els};
        const auto u32 = U32StringHashSet{U"alpha"_els};
        REQUIRE(u16.contains(u"alpha"_el));
        REQUIRE(u32.contains(U"alpha"_elv));
    }

    void testCaseInsensitiveSetViewKeyOperations() {
        using namespace el::text::literals;

        auto ordered = StringCISet{};
        ordered.insert(u8"ÄbcK"_el);
        REQUIRE(ordered.contains(u8"äbck"_elv));
        REQUIRE_FALSE(ordered.tryInsert(u8"ÄBCK"_el));
        REQUIRE(ordered.tryRemove(u8"äbck"_elv));
        REQUIRE(ordered.count().isZero());

        auto hash = StringCIHashSet{};
        hash.insert(u8"ÄbcK"_el);
        REQUIRE(hash.contains(u8"äbck"_elv));
        REQUIRE_FALSE(hash.tryInsert(u8"ÄBCK"_el));
        REQUIRE(hash.tryRemove(u8"äbck"_elv));
        REQUIRE(hash.count().isZero());
    }
};
