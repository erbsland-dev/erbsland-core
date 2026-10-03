// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Document Value)
class ValueGetListTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
private:
    static constexpr auto cMainNokValueListValue = "main.nok_value_list"_el;
    static constexpr auto cMainValue1Value = "main.value1"_el;
    static constexpr auto cMainValueListValue = "main.value_list"_el;
    static constexpr auto cMissingValue = "missing"_el;

public:
    template <typename T>
    void requireIntegerList() {
        auto list = doc->getList<T>(cMainValueListValue);
        REQUIRE_EQUAL(list.count().toSizeT(), 3U);
        REQUIRE_EQUAL(list.get(el::ItemIndex{0}), static_cast<T>(1));
        REQUIRE_EQUAL(list.get(el::ItemIndex{1}), static_cast<T>(2));
        REQUIRE_EQUAL(list.get(el::ItemIndex{2}), static_cast<T>(3));

        REQUIRE_NOTHROW(list = doc->getListOrThrow<T>(cMainValueListValue));
        REQUIRE_EQUAL(list.count().toSizeT(), 3U);
        REQUIRE_EQUAL(list.get(el::ItemIndex{0}), static_cast<T>(1));
        REQUIRE_EQUAL(list.get(el::ItemIndex{1}), static_cast<T>(2));
        REQUIRE_EQUAL(list.get(el::ItemIndex{2}), static_cast<T>(3));

        list = doc->getList<T>(cMainNokValueListValue);
        REQUIRE(list.isEmpty());

        try {
            list = doc->getListOrThrow<T>(cMainNokValueListValue);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::TypeMismatch);
        }
    }

    TESTED_TARGETS(getList)
    void testGetIntegerList() {
        WITH_CONTEXT(setupTemplate1("1", "2", "3", "\"text\""));
        WITH_CONTEXT(requireIntegerList<uint8_t>())
        WITH_CONTEXT(requireIntegerList<int8_t>())
        WITH_CONTEXT(requireIntegerList<uint16_t>())
        WITH_CONTEXT(requireIntegerList<int16_t>())
        WITH_CONTEXT(requireIntegerList<uint32_t>())
        WITH_CONTEXT(requireIntegerList<int32_t>())
        WITH_CONTEXT(requireIntegerList<uint64_t>())
        WITH_CONTEXT(requireIntegerList<int64_t>())
        WITH_CONTEXT(requireIntegerList<int>())
        WITH_CONTEXT(requireIntegerList<unsigned int>())
        WITH_CONTEXT(requireIntegerList<short>())
        WITH_CONTEXT(requireIntegerList<unsigned short>())
        WITH_CONTEXT(requireIntegerList<long>())
        WITH_CONTEXT(requireIntegerList<unsigned long>())
        WITH_CONTEXT(requireIntegerList<long long>())
        WITH_CONTEXT(requireIntegerList<unsigned long long>())
    }

    TESTED_TARGETS(getList)
    void testGetStringList() {
        WITH_CONTEXT(setupTemplate1("\"one\"", "\"two\"", "\"three\"", "true"));

        auto list = doc->getList<el::text::String>(cMainValueListValue);
        REQUIRE_EQUAL(list.count().toSizeT(), 3U);
        REQUIRE_EQUAL(list.get(el::ItemIndex{0}), el::text::String{"one"_el});
        REQUIRE_EQUAL(list.get(el::ItemIndex{1}), el::text::String{"two"_el});
        REQUIRE_EQUAL(list.get(el::ItemIndex{2}), el::text::String{"three"_el});

        REQUIRE_NOTHROW(list = doc->getListOrThrow<el::text::String>(cMainValueListValue));
        REQUIRE_EQUAL(list.count().toSizeT(), 3U);
        REQUIRE_EQUAL(list.get(el::ItemIndex{0}), el::text::String{"one"_el});
        REQUIRE_EQUAL(list.get(el::ItemIndex{1}), el::text::String{"two"_el});
        REQUIRE_EQUAL(list.get(el::ItemIndex{2}), el::text::String{"three"_el});

        list = doc->getList<el::text::String>(cMainNokValueListValue);
        REQUIRE(list.isEmpty());

        try {
            list = doc->getListOrThrow<el::text::String>(cMainNokValueListValue);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::TypeMismatch);
        }
    }

    void testListDefaultsAndCopyOnWrite() {
        WITH_CONTEXT(setupTemplate1("1", "2", "3", "false"));
        const auto fallback = el::List<int>{9, 10};
        REQUIRE_EQUAL(doc->getList<int>(cMissingValue, fallback), fallback);
        REQUIRE_EQUAL(doc->getList<int>(cMainNokValueListValue, fallback), fallback);
        REQUIRE_EQUAL(doc->getList<int>(cMainValue1Value, fallback), el::List<int>{1});
        auto result = doc->getList<int>(cMissingValue, fallback);
        result.append(11);
        REQUIRE_EQUAL(fallback.count(), el::ItemCount{2});
        REQUIRE_EQUAL(result.count(), el::ItemCount{3});
    }

    void testStringListConvenience() {
        WITH_CONTEXT(setupTemplate1("\"one\"", "\"two\"", "\"three\"", "false"));
        const auto expected = el::StringList{"one"_el, "two"_el, "three"_el};
        const auto fallback = el::StringList{"fallback"_el};
        REQUIRE_EQUAL(doc->getStringList(cMainValueListValue), expected);
        REQUIRE_EQUAL(doc->getStringListOrThrow(cMainValueListValue), expected);
        REQUIRE_EQUAL(doc->getStringList(cMainValue1Value), el::StringList{"one"_el});
        REQUIRE_EQUAL(doc->getStringList(cMissingValue, fallback), fallback);
        REQUIRE_EQUAL(doc->getStringList(cMainNokValueListValue, fallback), fallback);
        REQUIRE(doc->getStringList(cMainNokValueListValue).isEmpty());
        REQUIRE_THROWS_AS(ConfError, doc->getStringListOrThrow(cMissingValue));
        REQUIRE_THROWS_AS(ConfError, doc->getStringListOrThrow(cMainNokValueListValue));
    }
    void testStrictListDefaults() {
        WITH_CONTEXT(setupTemplate1("1", "2", "3", "false"));
        const auto fallback = el::List<int>{9, 10};
        REQUIRE_EQUAL(doc->getListOrThrow<int>(cMissingValue, fallback), fallback);
        REQUIRE_EQUAL(doc->getListOrThrow<int>(cMainValue1Value, fallback), el::List<int>{1});
        REQUIRE_THROWS_AS(ConfError, doc->getListOrThrow<int>(cMainNokValueListValue, fallback));
        REQUIRE_THROWS_AS(ConfError, doc->getListOrThrow<int>("main..value_list"_el, fallback));
    }

    void testStringListDefaultsAndDirectConversion() {
        WITH_CONTEXT(setupTemplate1("\"one\"", "\"two\"", "\"three\"", "false"));
        const auto expected = el::StringList{"one"_el, "two"_el, "three"_el};
        const auto fallback = el::StringList{"fallback"_el};
        REQUIRE_EQUAL(doc->getStringListOrThrow(cMissingValue, fallback), fallback);
        REQUIRE_EQUAL(doc->getStringListOrThrow(cMainValueListValue, fallback), expected);
        REQUIRE_THROWS_AS(ConfError, doc->getStringListOrThrow(cMainNokValueListValue, fallback));
        REQUIRE_THROWS_AS(ConfError, doc->getStringListOrThrow("main..value_list"_el, fallback));
        REQUIRE_EQUAL(doc->valueOrThrow(cMainValueListValue)->asStringList(), expected);
        REQUIRE_EQUAL(doc->valueOrThrow(cMainValueListValue)->asStringListOrThrow(), expected);
        REQUIRE_EQUAL(doc->valueOrThrow(cMainValue1Value)->asStringList(), el::StringList{"one"_el});
        REQUIRE(doc->valueOrThrow(cMainNokValueListValue)->asStringList().isEmpty());
        REQUIRE_THROWS_AS(ConfError, doc->valueOrThrow(cMainNokValueListValue)->asStringListOrThrow());
        auto result = doc->getStringListOrThrow(cMissingValue, fallback);
        result.append("changed"_el);
        REQUIRE_EQUAL(fallback.count(), el::ItemCount{1});
        REQUIRE_EQUAL(result.count(), el::ItemCount{2});
    }
};
