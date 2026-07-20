// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormatForConf.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Document Value)
class ValueGetListTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    template <typename T>
    void requireIntegerList() {
        auto list = doc->getList<T>("main.value_list"_el);
        REQUIRE_EQUAL(list.size(), 3U);
        REQUIRE_EQUAL(list[0], static_cast<T>(1));
        REQUIRE_EQUAL(list[1], static_cast<T>(2));
        REQUIRE_EQUAL(list[2], static_cast<T>(3));

        REQUIRE_NOTHROW(list = doc->getListOrThrow<T>("main.value_list"_el));
        REQUIRE_EQUAL(list.size(), 3U);
        REQUIRE_EQUAL(list[0], static_cast<T>(1));
        REQUIRE_EQUAL(list[1], static_cast<T>(2));
        REQUIRE_EQUAL(list[2], static_cast<T>(3));

        list = doc->getList<T>("main.nok_value_list"_el);
        REQUIRE(list.empty());

        try {
            list = doc->getListOrThrow<T>("main.nok_value_list"_el);
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

        auto list = doc->getList<el::text::String>("main.value_list"_el);
        REQUIRE_EQUAL(list.size(), 3U);
        REQUIRE_EQUAL(list[0], el::text::String{"one"_el});
        REQUIRE_EQUAL(list[1], el::text::String{"two"_el});
        REQUIRE_EQUAL(list[2], el::text::String{"three"_el});

        REQUIRE_NOTHROW(list = doc->getListOrThrow<el::text::String>("main.value_list"_el));
        REQUIRE_EQUAL(list.size(), 3U);
        REQUIRE_EQUAL(list[0], el::text::String{"one"_el});
        REQUIRE_EQUAL(list[1], el::text::String{"two"_el});
        REQUIRE_EQUAL(list[2], el::text::String{"three"_el});

        list = doc->getList<el::text::String>("main.nok_value_list"_el);
        REQUIRE(list.empty());

        try {
            list = doc->getListOrThrow<el::text::String>("main.nok_value_list"_el);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::TypeMismatch);
        }
    }
};
