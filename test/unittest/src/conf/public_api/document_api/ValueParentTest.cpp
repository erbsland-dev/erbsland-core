// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Document Value NamePath)
class ValueParentTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    TESTED_TARGETS(hasParent parent)
    void testParent() {
        setupTemplate1("1");
        REQUIRE_EQUAL(doc->hasParent(), false);
        value = doc->value(el::text::String{"main"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        value = doc->value(el::text::String{"main"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        value = doc->value(el::text::String{"main.sub.sub.a.value"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("main.sub.sub.a.value"_el);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("main.sub.sub.a"_el);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("main.sub.sub"_el);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("main.sub"_el);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("main"_el);
        value = value->parent(); // now we reached the document.
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), false);
        REQUIRE(value->namePath().empty());
        value = value->parent();
        REQUIRE_EQUAL(value, nullptr);
        value = doc->value(el::text::String{"main.text.\"second\""});
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("main.text"_el);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("main"_el);
        value = doc->value(el::text::String{"main.value_matrix[2][1]"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("main.value_matrix[2]"_el);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("main.value_matrix"_el);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("main"_el);
        value = doc->value(el::text::String{"list[2].value"});
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("list[2]"_el);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), true);
        requireNamePath("list"_el);
        value = value->parent();
        REQUIRE_NOT_EQUAL(value, nullptr);
        REQUIRE_EQUAL(value->hasParent(), false);
    }

private:
    void requireNamePath(const el::text::String &expected) {
        const auto actual = value->namePath().toText();
        REQUIRE_EQUAL(actual, expected);
    }
};
