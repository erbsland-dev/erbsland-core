// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormatForConf.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Document Value NamePath)
class ValueParentTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    TESTED_TARGETS(hasParent parent)
    void testParent() {
        setupTemplate1("1");
        REQUIRE_EQUAL(doc->hasParent(), false);
        value = doc->value(el::text::String{"main"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        value = doc->value(el::text::String{"main"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        value = doc->value(el::text::String{"main.sub.sub.a.value"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.sub.sub.a.value"_el));
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.sub.sub.a"_el));
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.sub.sub"_el));
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.sub"_el));
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main"_el));
        value = value->parent(); // now we reached the document.
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), false);
        REQUIRE(value->namePath().empty());
        value = value->parent();
        REQUIRE(value == nullptr);
        value = doc->value(el::text::String{"main.text.\"second\""});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.text"_el));
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main"_el));
        value = doc->value(el::text::String{"main.value_matrix[2][1]"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.value_matrix[2]"_el));
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main.value_matrix"_el));
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("main"_el));
        value = doc->value(el::text::String{"list[2].value"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("list[2]"_el));
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        REQUIRE_EQUAL(value->namePath().toText(), el::text::String("list"_el));
        value = value->parent();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->hasParent(), false);
    }
};
