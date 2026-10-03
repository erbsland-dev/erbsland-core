// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../util/MoveAwareTestValue.hpp"

#include <erbsland/ByteRange.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/StringCIHashMap.hpp>
#include <erbsland/StringCIMap.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringHashMap.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/U16StringCIHashMap.hpp>
#include <erbsland/U16StringCIMap.hpp>
#include <erbsland/U16StringHashMap.hpp>
#include <erbsland/U16StringMap.hpp>
#include <erbsland/U32StringCIHashMap.hpp>
#include <erbsland/U32StringCIMap.hpp>
#include <erbsland/U32StringHashMap.hpp>
#include <erbsland/U32StringMap.hpp>
#include <erbsland/U8StringCIHashMap.hpp>
#include <erbsland/U8StringCIMap.hpp>
#include <erbsland/U8StringHashMap.hpp>
#include <erbsland/U8StringMap.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>
#include <string>
#include <type_traits>
#include <utility>

using namespace el::text::literals;
using namespace el::text;

static_assert(std::is_same_v<StringMap<int>::Key, String>);
static_assert(std::is_same_v<StringHashMap<int>::Key, String>);
static_assert(std::is_same_v<U16StringMap<int>::Key, U16String>);
static_assert(std::is_same_v<U32StringHashMap<int>::Key, U32String>);

TESTED_TARGETS(
    StringMap StringCIMap StringHashMap StringCIHashMap U8StringMap U8StringCIMap U8StringHashMap U8StringCIHashMap
        U16StringMap U16StringCIMap U16StringHashMap U16StringCIHashMap U32StringMap U32StringCIMap U32StringHashMap
            U32StringCIHashMap)
class StringMapTest final : public el::UnitTest {
private:
    static constexpr auto cAKey = "a"_el;
    static constexpr auto cAlphaKey = "alpha"_el;
    static constexpr auto cDeltaKey = "delta"_el;
    static constexpr auto cGammaKey = "gamma"_el;
    static constexpr auto cMissingKey = "missing"_el;

public:
    using MoveValue = erbsland::test::MoveAwareTestValue;

public:
    void testSlicedKeysAreStoredCompactly() {

        auto backing = StringEditor::fromCharacter(U'x', el::unit::CpCount{1024U});
        backing.replace(el::unit::ByteRange{el::unit::ByteIndex{500U}, el::unit::ByteCount{3U}}, "key"_el);
        const auto source = String{backing};
        const auto key = source.slice({el::unit::ByteIndex{500U}, el::unit::ByteCount{3U}});

        auto ordered = StringMap<int>{};
        ordered.set(key, 1);
        REQUIRE_EQUAL(ordered.first().first, "key"_el);
        REQUIRE_NOT_EQUAL(ordered.first().first.storageId(), key.storageId());

        auto hashed = StringHashMap<int>{};
        REQUIRE(hashed.tryInsert(key, 2));
        REQUIRE_EQUAL(hashed.first().first, "key"_el);
        REQUIRE_NOT_EQUAL(hashed.first().first.storageId(), key.storageId());

        const auto initialized = StringMap<int>{{{key, 3}}};
        REQUIRE_NOT_EQUAL(initialized.first().first.storageId(), key.storageId());
    }

    void testAliasesAndKeyStringLists() {

        const auto common = StringMap<int>{{{"beta"_el, 2}, {cAlphaKey, 1}}};
        REQUIRE_EQUAL(StringConverter{common.first().first}.toStdString(), std::string{"alpha"});
        REQUIRE_EQUAL(StringConverter{common.toKeyStringList().join("|"_el)}.toStdString(), std::string{"alpha|beta"});
        REQUIRE_EQUAL(
            StringConverter{common.toKeyStringSet().toStringList().join("|"_el)}.toStdString(),
            std::string{"alpha|beta"});
        REQUIRE(common.toKeyStringHashSet().contains(cAlphaKey));

        const auto u8 = U8StringMap<int>{{{cAKey, 1}}};
        const auto u16 = U16StringMap<int>{{{u"a"_el, 1}}};
        const auto u32 = U32StringMap<int>{{{U"a"_el, 1}}};
        const auto hash = StringHashMap<int>{{{cAKey, 1}}};
        const auto u16Hash = U16StringHashMap<int>{{{u"a"_el, 1}}};
        const auto u32Hash = U32StringHashMap<int>{{{U"a"_el, 1}}};

        REQUIRE_EQUAL(u8.get(cAKey, 0), 1);
        REQUIRE_EQUAL(u16.get(u"a"_el, 0), 1);
        REQUIRE_EQUAL(u32.get(U"a"_el, 0), 1);
        REQUIRE_EQUAL(hash.get(cAKey, 0), 1);
        REQUIRE_EQUAL(u16Hash.get(u"a"_el, 0), 1);
        REQUIRE_EQUAL(u32Hash.get(U"a"_el, 0), 1);
    }

    void testCaseInsensitiveOrderedMapReplacesVisibleKey() {

        auto map = StringCIMap<int>{};
        map.set(u8"ÄbcK"_el, 1);

        REQUIRE(map.contains(u8"äbcK"_el));
        REQUIRE(!map.tryInsert(u8"äbcK"_el, 2));
        REQUIRE_EQUAL(map.get(u8"äbcK"_el, 0), 1);
        REQUIRE_EQUAL(StringConverter{map.toKeyStringList().first()}.toStdU8String(), std::u8string{u8"ÄbcK"});

        REQUIRE(map.tryReplace(u8"äbcK"_el, 3));
        REQUIRE_EQUAL(map.get(u8"ÄbcK"_el, 0), 3);
        REQUIRE_EQUAL(StringConverter{map.toKeyStringList().first()}.toStdU8String(), std::u8string{u8"äbcK"});

        const auto u16 = U16StringCIMap<int>{{{u"A"_el, 1}}};
        const auto u32 = U32StringCIMap<int>{{{U"A"_el, 1}}};
        REQUIRE_EQUAL(u16.get(u"a"_el, 0), 1);
        REQUIRE_EQUAL(u32.get(U"a"_el, 0), 1);
    }

    void testCaseInsensitiveHashMapReplacesVisibleKey() {

        auto map = StringCIHashMap<int>{};
        map.set(u8"ÄbcK"_el, 1);

        REQUIRE(map.contains(u8"äbcK"_el));
        REQUIRE(!map.tryInsert(u8"äbcK"_el, 2));
        REQUIRE_EQUAL(map.get(u8"äbcK"_el, 0), 1);
        REQUIRE_EQUAL(StringConverter{map.toKeyStringList().first()}.toStdU8String(), std::u8string{u8"ÄbcK"});

        map.set(u8"äbcK"_el, 3);
        REQUIRE_EQUAL(map.get(u8"ÄbcK"_el, 0), 3);
        REQUIRE_EQUAL(StringConverter{map.toKeyStringList().first()}.toStdU8String(), std::u8string{u8"äbcK"});

        const auto u8 = U8StringCIHashMap<int>{{{"A"_el, 1}}};
        const auto u16 = U16StringCIHashMap<int>{{{u"A"_el, 1}}};
        const auto u32 = U32StringCIHashMap<int>{{{U"A"_el, 1}}};
        REQUIRE_EQUAL(u8.get(cAKey, 0), 1);
        REQUIRE_EQUAL(u16.get(u"a"_el, 0), 1);
        REQUIRE_EQUAL(u32.get(U"a"_el, 0), 1);
    }

    void testOrderedMapViewKeyOperations() {

        auto map = StringMap<int>{{{cAlphaKey, 1}, {"beta"_el, 2}}};

        REQUIRE(map.contains(cAlphaKey));
        REQUIRE(map.contains("beta"_el));
        REQUIRE(map.get(cAlphaKey).has_value());
        REQUIRE_EQUAL(map.get(cAlphaKey).value(), 1);
        REQUIRE_EQUAL(map.get(cMissingKey, 9), 9);

        map.remove(cAlphaKey);
        REQUIRE_FALSE(map.contains(cAlphaKey));
        REQUIRE(map.contains("beta"_el));

        const auto removed = map.removed("beta"_el);
        REQUIRE_FALSE(removed.contains("beta"_el));
        REQUIRE(map.contains("beta"_el));
        REQUIRE_EQUAL(map.take("beta"_el), 2);
        REQUIRE(map.count().isZero());

        REQUIRE(map.tryInsert(cGammaKey, 3));
        REQUIRE_FALSE(map.tryInsert(cGammaKey, 4));
        REQUIRE_EQUAL(map.get(cGammaKey, 0), 3);
        REQUIRE(map.tryReplace(cGammaKey, 5));
        REQUIRE_EQUAL(map.get(cGammaKey, 0), 5);
        REQUIRE_FALSE(map.tryReplace(cMissingKey, 6));

        map.set(cDeltaKey, 7);
        REQUIRE_EQUAL(map.get(cDeltaKey, 0), 7);

        const auto u16 = U16StringMap<int>{{{u"alpha"_el, 1}}};
        const auto u32 = U32StringMap<int>{{{U"alpha"_el, 1}}};
        REQUIRE(u16.contains(u"alpha"_el));
        REQUIRE(u32.contains(U"alpha"_el));
        REQUIRE_EQUAL(u16.get(u"alpha"_el, 0), 1);
        REQUIRE_EQUAL(u32.get(U"alpha"_el, 0), 1);
    }

    void testHashMapViewKeyOperations() {

        auto map = StringHashMap<int>{{{cAlphaKey, 1}, {"beta"_el, 2}}};

        REQUIRE(map.contains(cAlphaKey));
        REQUIRE(map.contains("beta"_el));
        REQUIRE(map.get(cAlphaKey).has_value());
        REQUIRE_EQUAL(map.get(cAlphaKey).value(), 1);
        REQUIRE_EQUAL(map.get(cMissingKey, 9), 9);

        map.remove(cAlphaKey);
        REQUIRE_FALSE(map.contains(cAlphaKey));
        REQUIRE(map.contains("beta"_el));

        const auto removed = map.removed("beta"_el);
        REQUIRE_FALSE(removed.contains("beta"_el));
        REQUIRE(map.contains("beta"_el));
        REQUIRE_EQUAL(map.take("beta"_el), 2);
        REQUIRE(map.count().isZero());

        REQUIRE(map.tryInsert(cGammaKey, 3));
        REQUIRE_FALSE(map.tryInsert(cGammaKey, 4));
        REQUIRE_EQUAL(map.get(cGammaKey, 0), 3);
        REQUIRE(map.tryReplace(cGammaKey, 5));
        REQUIRE_EQUAL(map.get(cGammaKey, 0), 5);
        REQUIRE_FALSE(map.tryReplace(cMissingKey, 6));

        map.set(cDeltaKey, 7);
        REQUIRE_EQUAL(map.get(cDeltaKey, 0), 7);

        const auto u16 = U16StringHashMap<int>{{{u"alpha"_el, 1}}};
        const auto u32 = U32StringHashMap<int>{{{U"alpha"_el, 1}}};
        REQUIRE(u16.contains(u"alpha"_el));
        REQUIRE(u32.contains(U"alpha"_el));
        REQUIRE_EQUAL(u16.get(u"alpha"_el, 0), 1);
        REQUIRE_EQUAL(u32.get(U"alpha"_el, 0), 1);
    }

    void testViewKeyOperationsMoveValues() {

        auto ordered = StringMap<MoveValue>{};
        auto orderedSetValue = MoveValue{1};
        const auto orderedSetCounts = orderedSetValue.counts();
        ordered.set(cAlphaKey, std::move(orderedSetValue));
        REQUIRE_EQUAL(orderedSetCounts->copies, 0);
        REQUIRE(ordered.contains(cAlphaKey));

        auto orderedReplaceValue = MoveValue{2};
        const auto orderedReplaceCounts = orderedReplaceValue.counts();
        REQUIRE(ordered.tryReplace(cAlphaKey, std::move(orderedReplaceValue)));
        REQUIRE_EQUAL(orderedReplaceCounts->copies, 0);

        auto orderedInsertValue = MoveValue{3};
        const auto orderedInsertCounts = orderedInsertValue.counts();
        REQUIRE(ordered.tryInsert("beta"_el, std::move(orderedInsertValue)));
        REQUIRE_EQUAL(orderedInsertCounts->copies, 0);

        auto hash = StringHashMap<MoveValue>{};
        auto hashSetValue = MoveValue{1};
        const auto hashSetCounts = hashSetValue.counts();
        hash.set(cAlphaKey, std::move(hashSetValue));
        REQUIRE_EQUAL(hashSetCounts->copies, 0);
        REQUIRE(hash.contains(cAlphaKey));

        auto hashReplaceValue = MoveValue{2};
        const auto hashReplaceCounts = hashReplaceValue.counts();
        REQUIRE(hash.tryReplace(cAlphaKey, std::move(hashReplaceValue)));
        REQUIRE_EQUAL(hashReplaceCounts->copies, 0);

        auto hashInsertValue = MoveValue{3};
        const auto hashInsertCounts = hashInsertValue.counts();
        REQUIRE(hash.tryInsert("beta"_el, std::move(hashInsertValue)));
        REQUIRE_EQUAL(hashInsertCounts->copies, 0);
    }

    void testCaseInsensitiveMapViewKeyOperations() {

        auto ordered = StringCIMap<int>{};
        ordered.set(u8"ÄbcK"_el, 1);
        REQUIRE(ordered.contains(u8"äbck"_el));
        REQUIRE_EQUAL(ordered.get(u8"äbck"_el, 0), 1);
        REQUIRE_FALSE(ordered.tryInsert(u8"äbck"_el, 2));
        REQUIRE(ordered.tryReplace(u8"äbck"_el, 3));
        REQUIRE_EQUAL(ordered.get(u8"ÄBCK"_el, 0), 3);
        REQUIRE_EQUAL(ordered.take(u8"ÄBCK"_el), 3);
        REQUIRE(ordered.count().isZero());

        auto hash = StringCIHashMap<int>{};
        hash.set(u8"ÄbcK"_el, 1);
        REQUIRE(hash.contains(u8"äbck"_el));
        REQUIRE_EQUAL(hash.get(u8"äbck"_el, 0), 1);
        REQUIRE_FALSE(hash.tryInsert(u8"äbck"_el, 2));
        REQUIRE(hash.tryReplace(u8"äbck"_el, 3));
        REQUIRE_EQUAL(hash.get(u8"ÄBCK"_el, 0), 3);
        REQUIRE_EQUAL(hash.take(u8"ÄBCK"_el), 3);
        REQUIRE(hash.count().isZero());
    }

    void testStringHashes() {

        const auto u8 = String{u8"ÄbcK"_el};
        const auto u16 = U16String{u"ÄbcK"_el};
        const auto u32 = U32String{U"ÄbcK"_el};

        REQUIRE_EQUAL(u8.toHash(), u8.toHash());
        REQUIRE_EQUAL(u8.toHash(), String{u8"ÄbcK"_el}.toHash());
        REQUIRE_EQUAL(u16.toHash(), U16String{u"ÄbcK"_el}.toHash());
        REQUIRE_EQUAL(u32.toHash(), U32String{U"ÄbcK"_el}.toHash());
        REQUIRE_EQUAL(u8.toHash(), u16.toHash());
        REQUIRE_EQUAL(u8.toHash(), u32.toHash());
        REQUIRE_EQUAL(u8.toHashCI(), String{u8"äbck"_el}.toHashCI());
        REQUIRE_EQUAL(u16.toHashCI(), U16String{u"äbck"_el}.toHashCI());
        REQUIRE_EQUAL(u32.toHashCI(), U32String{U"äbck"_el}.toHashCI());
        REQUIRE_EQUAL(std::hash<StringEditor>{}(StringEditor{u8}), u8.toHash());
        REQUIRE_EQUAL(std::hash<U16StringEditor>{}(U16StringEditor{u16}), u16.toHash());
        REQUIRE_EQUAL(std::hash<U32StringEditor>{}(U32StringEditor{u32}), u32.toHash());
        REQUIRE_EQUAL(std::hash<String>{}(u8"ÄbcK"_el), u8.toHash());
        REQUIRE_EQUAL(std::hash<U16String>{}(u"ÄbcK"_el), u16.toHash());
        REQUIRE_EQUAL(std::hash<U32String>{}(U"ÄbcK"_el), u32.toHash());
    }
};
