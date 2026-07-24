// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using el::conf::ConfError;
using el::conf::ConfErrorCategory;
using el::conf::NamePathLike;
using namespace el::text::literals;

TESTED_TARGETS(Document Value)
class ValueChildValueTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    el::text::String errorText;

    auto additionalErrorMessages() -> std::string override {
        auto result = ValueTestHelper::additionalErrorMessages();
        if (!errorText.isEmpty()) {
            result += "error: ";
            result += el::text::StringConverter{errorText}.toStdString();
            result += "\n";
        }
        return result;
    }

    void requireError(const ConfErrorCategory errorCategory, const NamePathLike &namePath) {
        try {
            value = doc->valueOrThrow(namePath);
            REQUIRE(false);
        } catch (const ConfError &e) {
            errorText = e.description();
            REQUIRE_EQUAL(e.category(), errorCategory);
        }
    }

    TESTED_TARGETS(valueOrThrow)
    void testValueOrThrow() {
        setupTemplate1("1");
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main"}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main"_el}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main"_el}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(Name::createRegular("main"_el)));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(NamePath{Name::createRegular("main"_el)}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.sub.sub.a.value"}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.sub.sub.a.value"_el}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.sub.sub.a.value"_el}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(Name::createRegular("main"_el)));
        REQUIRE_NOTHROW(value = value->valueOrThrow(Name::createRegular("sub"_el)));
        REQUIRE_NOTHROW(value = value->valueOrThrow(Name::createRegular("sub"_el)));
        REQUIRE_NOTHROW(value = value->valueOrThrow(Name::createRegular("a"_el)));
        REQUIRE_NOTHROW(value = value->valueOrThrow(Name::createRegular("value"_el)));
        const auto namePath = NamePath{{
            Name::createRegular("main"_el),
            Name::createRegular("sub"_el),
            Name::createRegular("sub"_el),
            Name::createRegular("a"_el),
            Name::createRegular("value"_el),
        }};
        REQUIRE_NOTHROW(value = doc->valueOrThrow(namePath));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.value_list[2]"_el}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.value_matrix[2][2]"_el}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.text.\"second\""_el}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.text.\"\"[1]"_el}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.sub_text.\"second\""_el}));
        REQUIRE_NOTHROW(value = doc->valueOrThrow(el::text::String{"main.sub_text.\"\"[1]"_el}));

        // not found
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, ""_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "unknown"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "\"unknown\""_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "[0]"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "\"\"[0]"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.unknown"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.\"unknown\""_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.\"\"[0]"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.value_list.unknown"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.value_list.\"unknown\""_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.value_list.\"\"[0]"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.value_list[1].unknown"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.value_list[1].\"unknown\""_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.value_list[1].\"\"[0]"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.value_matrix[1][2].unknown"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.value_matrix[1][2].\"unknown\""_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.value_matrix[1][2].\"\"[0]"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.text.\"unknown\""_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.text.\"first\".unknown"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.text.\"first\".\"unknown\""_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::ValueNotFound, "main.text.\"first\".\"\"[0]"_el));

        WITH_CONTEXT(requireError(ConfErrorCategory::Syntax, "main.[0]"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::Syntax, "main.text.\"first\".[0]"_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::Syntax, "main..value1"_el));
    }

    TESTED_TARGETS(value)
    void testValue() {
        setupTemplate1("1");
        REQUIRE(doc->value(el::text::String{"main"}) != nullptr);

        REQUIRE(doc->value(el::text::String{"main"_el}) != nullptr);
        REQUIRE(doc->value(el::text::String{"main"_el}) != nullptr);
        REQUIRE(doc->value(Name::createRegular("main"_el)) != nullptr);
        REQUIRE(doc->value(NamePath{Name::createRegular("main"_el)}) != nullptr);
        REQUIRE(doc->value(el::text::String{"main.sub.sub.a.value"}) != nullptr);
        REQUIRE(doc->value(el::text::String{"main.sub.sub.a.value"_el}) != nullptr);
        REQUIRE(doc->value(el::text::String{"main.sub.sub.a.value"_el}) != nullptr);
        REQUIRE((value = doc->value(Name::createRegular("main"_el))) != nullptr);
        REQUIRE((value = value->value(Name::createRegular("sub"_el))) != nullptr);
        REQUIRE((value = value->value(Name::createRegular("sub"_el))) != nullptr);
        REQUIRE((value = value->value(Name::createRegular("a"_el))) != nullptr);
        REQUIRE((value = value->value(Name::createRegular("value"_el))) != nullptr);
        const auto namePath = NamePath{{
            Name::createRegular("main"_el),
            Name::createRegular("sub"_el),
            Name::createRegular("sub"_el),
            Name::createRegular("a"_el),
            Name::createRegular("value"_el),
        }};
        REQUIRE(doc->value(namePath) != nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_list[2]"_el}) != nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_matrix[2][2]"_el}) != nullptr);
        REQUIRE(doc->value(el::text::String{"main.text.\"second\""_el}) != nullptr);
        REQUIRE(doc->value(el::text::String{"main.text.\"\"[1]"_el}) != nullptr);
        REQUIRE(doc->value(el::text::String{"main.sub_text.\"second\""_el}) != nullptr);
        REQUIRE(doc->value(el::text::String{"main.sub_text.\"\"[1]"_el}) != nullptr);

        // not found
        REQUIRE(doc->value(el::text::String{""_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"unknown"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"\"unknown\""_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"[0]"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"\"\"[0]"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.unknown"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.\"unknown\""_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.\"\"[0]"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_list.unknown"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_list.\"unknown\""_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_list.\"\"[0]"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_list[1].unknown"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_list[1].\"unknown\""_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_list[1].\"\"[0]"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_matrix[1][2].unknown"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_matrix[1][2].\"unknown\""_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.value_matrix[1][2].\"\"[0]"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.text.\"unknown\""_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.text.\"first\".unknown"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.text.\"first\".\"unknown\""_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.text.\"first\".\"\"[0]"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.[0]"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main.text.\"first\".[0]"_el}) == nullptr);
        REQUIRE(doc->value(el::text::String{"main..value1"_el}) == nullptr);
    }

    TESTED_TARGETS(hasValue)
    void testHasValue() {
        setupTemplate1("1");
        REQUIRE(doc->hasValue(el::text::String{"main"}) == true);

        REQUIRE(doc->hasValue(el::text::String{"main"_el}) == true);
        REQUIRE(doc->hasValue(el::text::String{"main"_el}) == true);
        REQUIRE(doc->hasValue(Name::createRegular("main"_el)) == true);
        REQUIRE(doc->hasValue(NamePath{Name::createRegular("main"_el)}) == true);
        REQUIRE(doc->hasValue(el::text::String{"main.sub.sub.a.value"}) == true);
        REQUIRE(doc->hasValue(el::text::String{"main.sub.sub.a.value"_el}) == true);
        REQUIRE(doc->hasValue(el::text::String{"main.sub.sub.a.value"_el}) == true);
        const auto namePath = NamePath{{
            Name::createRegular("main"_el),
            Name::createRegular("sub"_el),
            Name::createRegular("sub"_el),
            Name::createRegular("a"_el),
            Name::createRegular("value"_el),
        }};
        REQUIRE(doc->hasValue(namePath) == true);
        REQUIRE(doc->hasValue(el::text::String{"main.value_list[2]"_el}) == true);
        REQUIRE(doc->hasValue(el::text::String{"main.value_matrix[2][2]"_el}) == true);
        REQUIRE(doc->hasValue(el::text::String{"main.text.\"second\""_el}) == true);
        REQUIRE(doc->hasValue(el::text::String{"main.text.\"\"[1]"_el}) == true);
        REQUIRE(doc->hasValue(el::text::String{"main.sub_text.\"second\""_el}) == true);
        REQUIRE(doc->hasValue(el::text::String{"main.sub_text.\"\"[1]"_el}) == true);

        // not found
        REQUIRE(doc->hasValue(el::text::String{""_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"unknown"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"\"unknown\""_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"[0]"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"\"\"[0]"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.unknown"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.\"unknown\""_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.\"\"[0]"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.value_list.unknown"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.value_list.\"unknown\""_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.value_list.\"\"[0]"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.value_list[1].unknown"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.value_list[1].\"unknown\""_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.value_list[1].\"\"[0]"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.value_matrix[1][2].unknown"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.value_matrix[1][2].\"unknown\""_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.value_matrix[1][2].\"\"[0]"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.text.\"unknown\""_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.text.\"first\".unknown"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.text.\"first\".\"unknown\""_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.text.\"first\".\"\"[0]"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.[0]"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main.text.\"first\".[0]"_el}) == false);
        REQUIRE(doc->hasValue(el::text::String{"main..value1"_el}) == false);
    }

    TESTED_TARGETS(size)
    void testSize() {
        setupTemplate1("1");
        REQUIRE_EQUAL(doc->size(), 2);
        value = doc->valueOrThrow(el::text::String{"main"});
        REQUIRE_EQUAL(value->size(), 11);
        value = doc->valueOrThrow(el::text::String{"main.sub"_el});
        REQUIRE_EQUAL(value->size(), 1);
        value = doc->valueOrThrow(el::text::String{"main.sub.sub"_el});
        REQUIRE_EQUAL(value->size(), 3);
        value = doc->valueOrThrow(el::text::String{"main.sub.sub.a.value"_el});
        REQUIRE_EQUAL(value->size(), 0);
        value = doc->valueOrThrow(el::text::String{"main.value_list"_el});
        REQUIRE_EQUAL(value->size(), 3);
        value = doc->valueOrThrow(el::text::String{"main.value_list[2]"_el});
        REQUIRE_EQUAL(value->size(), 0);
        value = doc->valueOrThrow(el::text::String{"main.value_matrix"_el});
        REQUIRE_EQUAL(value->size(), 3);
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[2]"_el});
        REQUIRE_EQUAL(value->size(), 3);
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[2][2]"_el});
        REQUIRE_EQUAL(value->size(), 0);
        value = doc->valueOrThrow(el::text::String{"main.text"_el});
        REQUIRE_EQUAL(value->size(), 3);
        value = doc->valueOrThrow(el::text::String{"main.sub_text"_el});
        REQUIRE_EQUAL(value->size(), 3);
        value = doc->valueOrThrow(el::text::String{"main.sub_text.\"first\""_el});
        REQUIRE_EQUAL(value->size(), 1);
    }

    void testBeginAndEnd() {
        setupTemplate1("1");
        auto it = doc->begin();
        REQUIRE(it != doc->end());
        REQUIRE_EQUAL(it->name(), Name::createRegular("main"_el));
        REQUIRE_EQUAL(it->size(), 11);
        ++it;
        REQUIRE(it != doc->end());
        REQUIRE_EQUAL(it->name(), Name::createRegular("list"_el));
        ++it;
        REQUIRE(it == doc->end());

        value = doc->valueOrThrow(el::text::String{"main.value1"});
        it = value->begin();
        REQUIRE(it == value->end());
    }

    void testEmpty() {
        setupTemplate1("1");
        REQUIRE_FALSE(doc->empty());
        value = doc->valueOrThrow(el::text::String{"main"});
        REQUIRE_FALSE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.sub"_el});
        REQUIRE_FALSE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.sub.sub"_el});
        REQUIRE_FALSE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.sub.sub.a.value"_el});
        REQUIRE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.value_list"_el});
        REQUIRE_FALSE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.value_list[2]"_el});
        REQUIRE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.value_matrix"_el});
        REQUIRE_FALSE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[2]"_el});
        REQUIRE_FALSE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.value_matrix[2][2]"_el});
        REQUIRE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.text"_el});
        REQUIRE_FALSE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.sub_text"_el});
        REQUIRE_FALSE(value->empty());
        value = doc->valueOrThrow(el::text::String{"main.sub_text.\"first\""_el});
        REQUIRE_FALSE(value->empty());
    }

    void testFirstAndLastValue() {
        setupTemplate1("1", "2", "3");
        value = doc->valueOrThrow(el::text::String{"main"});
        REQUIRE(value != nullptr);
        value = value->firstValue();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->toTestText(), el::text::String{"Integer(1)"_el});

        value = doc->valueOrThrow(el::text::String{"main.value_list"});
        REQUIRE(value != nullptr);
        value = value->lastValue();
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->toTestText(), el::text::String{"Integer(3)"_el});

        value = doc->valueOrThrow(el::text::String{"main.value1"});
        REQUIRE(value != nullptr);
        value = value->firstValue();
        REQUIRE(value == nullptr);

        value = doc->valueOrThrow(el::text::String{"main.value1"});
        REQUIRE(value != nullptr);
        value = value->lastValue();
        REQUIRE(value == nullptr);
    }
};
