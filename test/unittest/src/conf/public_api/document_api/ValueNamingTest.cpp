// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Document Value NamePath)
class ValueNamingTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    TESTED_TARGETS(name)
    void testName() {
        setupTemplate1("1");
        value = doc->value(el::text::String{"main"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        const auto mainName = Name::createRegular("main"_el);
        REQUIRE_EQUAL(value->name(), mainName);
        value = doc->value(el::text::String{"main.value1"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        const auto value1Name = Name::createRegular("value1"_el);
        REQUIRE_EQUAL(value->name(), value1Name);
        value = doc->value(el::text::String{"main.sub.sub.a.value"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        const auto valueName = Name::createRegular("value"_el);
        REQUIRE_EQUAL(value->name(), valueName);
        value = doc->value(el::text::String{"list[1]"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        const auto indexName = Name::createIndex(1);
        REQUIRE_EQUAL(value->name(), indexName);
        value = doc->value(el::text::String{"main.text.\"second\""});
        REQUIRE_NOT_EQUAL(value, nullptr);
        const auto secondName = Name::createText("second"_el);
        REQUIRE_EQUAL(value->name(), secondName);
        value = doc->value(el::text::String{"main.sub_text.\"third\""});
        REQUIRE_NOT_EQUAL(value, nullptr);
        const auto thirdName = Name::createText("third"_el);
        REQUIRE_EQUAL(value->name(), thirdName);
        value = doc->value(el::text::String{"main.value_list"});
        REQUIRE_NOTHROW(value = value->asValueList().at(1));
        REQUIRE_EQUAL(value->name(), indexName);
        value = doc->value(el::text::String{"main.value_matrix"});
        REQUIRE_NOTHROW(value = value->asValueList().at(2));
        REQUIRE_NOTHROW(value = value->asValueList().at(1));
        REQUIRE_EQUAL(value->name(), indexName);
    }

    TESTED_TARGETS(namePath)
    void testNamePath() {
        setupTemplate1("1");
        value = doc->value(el::text::String{"main"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        requireNamePath("main"_el);
        value = doc->value(el::text::String{"main.value1"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        requireNamePath("main.value1"_el);
        value = doc->value(el::text::String{"main.sub.sub.a.value"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        requireNamePath("main.sub.sub.a.value"_el);
        value = doc->value(el::text::String{"list[1]"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        requireNamePath("list[1]"_el);
        value = doc->value(el::text::String{"main.text.\"second\""});
        REQUIRE_NOT_EQUAL(value, nullptr);
        requireNamePath("main.text.\"second\""_el);
        value = doc->value(el::text::String{"main.sub_text.\"third\""});
        REQUIRE_NOT_EQUAL(value, nullptr);
        requireNamePath("main.sub_text.\"third\""_el);
        value = doc->value(el::text::String{"main.value_list"});
        REQUIRE_NOTHROW(value = value->asValueList().at(1));
        REQUIRE_NOT_EQUAL(value, nullptr);
        requireNamePath("main.value_list[1]"_el);
        value = doc->value(el::text::String{"main.value_matrix"});
        REQUIRE_NOTHROW(value = value->asValueList().at(2));
        REQUIRE_NOTHROW(value = value->asValueList().at(1));
        requireNamePath("main.value_matrix[2][1]"_el);
    }

private:
    void requireNamePath(const el::text::String &expected) {
        const auto actual = value->namePath().toText();
        REQUIRE_EQUAL(actual, expected);
    }
};
