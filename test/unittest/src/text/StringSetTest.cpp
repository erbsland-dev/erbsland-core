// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringCIHashSet.hpp>
#include <erbsland/text/StringCISet.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
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
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <type_traits>

using namespace el::text::literals;

using namespace el::text;

static_assert(std::is_same_v<StringSet::Key, String>);
static_assert(std::is_same_v<StringHashSet::Key, String>);
static_assert(std::is_same_v<U16StringSet::Key, U16String>);
static_assert(std::is_same_v<U32StringHashSet::Key, U32String>);

TESTED_TARGETS(
    StringSet StringCISet StringHashSet StringCIHashSet U8StringSet U8StringCISet U8StringHashSet U8StringCIHashSet
        U16StringSet U16StringCISet U16StringHashSet U16StringCIHashSet U32StringSet U32StringCISet U32StringHashSet
            U32StringCIHashSet)
class StringSetTest final : public el::UnitTest {
public:
    void testSlicedKeysAreStoredCompactly() {

        auto backing = StringEditor::fromCharacter(U'x', el::unit::CpLength{1024U});
        backing.replace(el::unit::ByteRange{el::unit::ByteIndex{500U}, el::unit::ByteLength{3U}}, "key"_el);
        const auto source = String{backing};
        const auto key = source.slice(el::unit::ByteRange{el::unit::ByteIndex{500U}, el::unit::ByteLength{3U}});

        auto ordered = StringSet{};
        ordered.insert(key);
        REQUIRE_EQUAL(ordered.first(), "key"_el);
        REQUIRE_NOT_EQUAL(ordered.first().storageId(), key.storageId());

        auto hashed = StringHashSet{};
        REQUIRE(hashed.tryInsert(key));
        REQUIRE_EQUAL(hashed.first(), "key"_el);
        REQUIRE_NOT_EQUAL(hashed.first().storageId(), key.storageId());

        const auto initialized = StringSet{key};
        REQUIRE_NOT_EQUAL(initialized.first().storageId(), key.storageId());
    }

    void testAliasesAndStringLists() {

        const auto common = StringSet{"beta"_el, "alpha"_el, "alpha"_el};
        REQUIRE_EQUAL(StringConverter{common.toStringList().join("|"_el)}.toStdString(), std::string{"alpha|beta"});

        const auto u8 = U8StringSet{"a"_el};
        const auto u16 = U16StringSet{u"a"_el};
        const auto u32 = U32StringSet{U"a"_el};
        const auto hash = StringHashSet{"a"_el};
        const auto u16Hash = U16StringHashSet{u"a"_el};
        const auto u32Hash = U32StringHashSet{U"a"_el};

        REQUIRE(u8.contains("a"_el));
        REQUIRE(u16.contains(u"a"_el));
        REQUIRE(u32.contains(U"a"_el));
        REQUIRE(hash.contains("a"_el));
        REQUIRE(u16Hash.contains(u"a"_el));
        REQUIRE(u32Hash.contains(U"a"_el));
    }

    void testCaseInsensitiveOrderedSet() {

        auto set = StringCISet{};
        set.insert(u8"ÄbcK"_el);

        REQUIRE(set.contains(u8"äbcK"_el));
        REQUIRE(!set.tryInsert(u8"äbcK"_el));
        REQUIRE_EQUAL(StringConverter{set.toStringList().first()}.toStdU8String(), std::u8string{u8"ÄbcK"});
        REQUIRE(set.compareCI(StringSet{u8"äbcK"_el}));
        REQUIRE(set.tryRemove(u8"äbcK"_el));
        REQUIRE(set.count().isZero());

        const auto u8 = U8StringCISet{"A"_el};
        const auto u16 = U16StringCISet{u"A"_el};
        const auto u32 = U32StringCISet{U"A"_el};
        REQUIRE(u8.contains("a"_el));
        REQUIRE(u16.contains(u"a"_el));
        REQUIRE(u32.contains(U"a"_el));
    }

    void testCaseInsensitiveHashSet() {

        auto set = StringCIHashSet{};
        set.insert(u8"ÄbcK"_el);

        REQUIRE(set.contains(u8"äbcK"_el));
        REQUIRE(!set.tryInsert(u8"äbcK"_el));
        REQUIRE_EQUAL(StringConverter{set.toStringList().first()}.toStdU8String(), std::u8string{u8"ÄbcK"});
        REQUIRE(set.compareCI(StringHashSet{u8"äbcK"_el}));
        REQUIRE(set.tryRemove(u8"äbcK"_el));
        REQUIRE(set.count().isZero());

        const auto u8 = U8StringCIHashSet{"A"_el};
        const auto u16 = U16StringCIHashSet{u"A"_el};
        const auto u32 = U32StringCIHashSet{U"A"_el};
        REQUIRE(u8.contains("a"_el));
        REQUIRE(u16.contains(u"a"_el));
        REQUIRE(u32.contains(U"a"_el));
    }

    void testOrderedSetViewKeyOperations() {

        auto set = StringSet{"alpha"_el, "beta"_el};

        REQUIRE(set.contains("alpha"_el));
        REQUIRE(set.contains("beta"_el));

        set.remove("alpha"_el);
        REQUIRE_FALSE(set.contains("alpha"_el));
        REQUIRE(set.contains("beta"_el));

        const auto removed = set.removed("beta"_el);
        REQUIRE_FALSE(removed.contains("beta"_el));
        REQUIRE(set.contains("beta"_el));
        REQUIRE(set.tryRemove("beta"_el));
        REQUIRE(set.count().isZero());

        set.insert("gamma"_el);
        REQUIRE(set.contains("gamma"_el));
        REQUIRE_FALSE(set.tryInsert("gamma"_el));
        REQUIRE(set.tryInsert("delta"_el));
        REQUIRE(set.contains("delta"_el));

        const auto u16 = U16StringSet{u"alpha"_el};
        const auto u32 = U32StringSet{U"alpha"_el};
        REQUIRE(u16.contains(u"alpha"_el));
        REQUIRE(u32.contains(U"alpha"_el));
    }

    void testHashSetViewKeyOperations() {

        auto set = StringHashSet{"alpha"_el, "beta"_el};

        REQUIRE(set.contains("alpha"_el));
        REQUIRE(set.contains("beta"_el));

        set.remove("alpha"_el);
        REQUIRE_FALSE(set.contains("alpha"_el));
        REQUIRE(set.contains("beta"_el));

        const auto removed = set.removed("beta"_el);
        REQUIRE_FALSE(removed.contains("beta"_el));
        REQUIRE(set.contains("beta"_el));
        REQUIRE(set.tryRemove("beta"_el));
        REQUIRE(set.count().isZero());

        set.insert("gamma"_el);
        REQUIRE(set.contains("gamma"_el));
        REQUIRE_FALSE(set.tryInsert("gamma"_el));
        REQUIRE(set.tryInsert("delta"_el));
        REQUIRE(set.contains("delta"_el));

        const auto u16 = U16StringHashSet{u"alpha"_el};
        const auto u32 = U32StringHashSet{U"alpha"_el};
        REQUIRE(u16.contains(u"alpha"_el));
        REQUIRE(u32.contains(U"alpha"_el));
    }

    void testCaseInsensitiveSetViewKeyOperations() {

        auto ordered = StringCISet{};
        ordered.insert(u8"ÄbcK"_el);
        REQUIRE(ordered.contains(u8"äbck"_el));
        REQUIRE_FALSE(ordered.tryInsert(u8"ÄBCK"_el));
        REQUIRE(ordered.tryRemove(u8"äbck"_el));
        REQUIRE(ordered.count().isZero());

        auto hash = StringCIHashSet{};
        hash.insert(u8"ÄbcK"_el);
        REQUIRE(hash.contains(u8"äbck"_el));
        REQUIRE_FALSE(hash.tryInsert(u8"ÄBCK"_el));
        REQUIRE(hash.tryRemove(u8"äbck"_el));
        REQUIRE(hash.count().isZero());
    }
};
