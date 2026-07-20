// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormatForConf.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Document Value NamePath)
class ValueNamingTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    TESTED_TARGETS(name)
    void testName() {
        setupTemplate1("1");
        value = doc->value(el::text::String{"main"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->name(), Name::createRegular("main"_el));
        value = doc->value(el::text::String{"main.value1"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->name(), Name::createRegular("value1"_el));
        value = doc->value(el::text::String{"main.sub.sub.a.value"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->name(), Name::createRegular("value"_el));
        value = doc->value(el::text::String{"list[1]"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->name(), Name::createIndex(1));
        value = doc->value(el::text::String{"main.text.\"second\""});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->name(), Name::createText("second"_el));
        value = doc->value(el::text::String{"main.sub_text.\"third\""});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->name(), Name::createText("third"_el));
        value = doc->value(el::text::String{"main.value_list"});
        REQUIRE_NOTHROW(value = value->asValueList().at(1));
        REQUIRE_EQUAL(value->name(), Name::createIndex(1));
        value = doc->value(el::text::String{"main.value_matrix"});
        REQUIRE_NOTHROW(value = value->asValueList().at(2));
        REQUIRE_NOTHROW(value = value->asValueList().at(1));
        REQUIRE_EQUAL(value->name(), Name::createIndex(1));
    }

    TESTED_TARGETS(namePath)
    void testNamePath() {
        setupTemplate1("1");
        value = doc->value(el::text::String{"main"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main"_el));
        value = doc->value(el::text::String{"main.value1"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.value1"_el));
        value = doc->value(el::text::String{"main.sub.sub.a.value"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.sub.sub.a.value"_el));
        value = doc->value(el::text::String{"list[1]"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("list[1]"_el));
        value = doc->value(el::text::String{"main.text.\"second\""});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.text.\"second\""_el));
        value = doc->value(el::text::String{"main.sub_text.\"third\""});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.sub_text.\"third\""_el));
        value = doc->value(el::text::String{"main.value_list"});
        REQUIRE_NOTHROW(value = value->asValueList().at(1));
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.value_list[1]"_el));
        value = doc->value(el::text::String{"main.value_matrix"});
        REQUIRE_NOTHROW(value = value->asValueList().at(2));
        REQUIRE_NOTHROW(value = value->asValueList().at(1));
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.value_matrix[2][1]"_el));
    }
};
