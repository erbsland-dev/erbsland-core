// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormatForConf.hpp>

TESTED_TARGETS(Document Value)
class ValueTypeMethodTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    TESTED_TARGETS(type)
    void testTypeMethod() {
        setupTemplate1("1");
        REQUIRE_EQUAL(doc->type(), ValueType::Document);
        value = doc->value(el::text::String{"main"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->type(), ValueType::SectionWithNames);
        value = doc->value(el::text::String{"main.sub.sub.a"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->type(), ValueType::SectionWithNames);
        value = doc->value(el::text::String{"main.sub.sub"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->type(), ValueType::IntermediateSection);
        value = doc->value(el::text::String{"list"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->type(), ValueType::SectionList);
        value = doc->value(el::text::String{"main.text"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->type(), ValueType::SectionWithTexts);
        value = doc->value(el::text::String{"main.sub_text"});
        REQUIRE(value != nullptr);
        REQUIRE_EQUAL(value->type(), ValueType::SectionWithTexts);
        setupTemplate2("100");
        REQUIRE_EQUAL(value->type(), ValueType::Integer);
        setupTemplate2("0x100");
        REQUIRE_EQUAL(value->type(), ValueType::Integer);
        setupTemplate2("100 mb");
        REQUIRE_EQUAL(value->type(), ValueType::Integer);
        setupTemplate2("true");
        REQUIRE_EQUAL(value->type(), ValueType::Boolean);
        setupTemplate2("0.1");
        REQUIRE_EQUAL(value->type(), ValueType::Float);
        setupTemplate2("\"text\"");
        REQUIRE_EQUAL(value->type(), ValueType::Text);
        setupTemplate2("`text`");
        REQUIRE_EQUAL(value->type(), ValueType::Text);
        setupTemplate2("2025-01-01");
        REQUIRE_EQUAL(value->type(), ValueType::Date);
        setupTemplate2("14:20:34");
        REQUIRE_EQUAL(value->type(), ValueType::Time);
        setupTemplate2("2025-01-01 14:20:34");
        REQUIRE_EQUAL(value->type(), ValueType::DateTime);
        setupTemplate2("<00 01 02>");
        REQUIRE_EQUAL(value->type(), ValueType::Bytes);
        setupTemplate2("2 weeks");
        REQUIRE_EQUAL(value->type(), ValueType::TimeDelta);
        setupTemplate2("/regex/");
        REQUIRE_EQUAL(value->type(), ValueType::RegEx);
        setupTemplate2("1, 2, 3");
        REQUIRE_EQUAL(value->type(), ValueType::ValueList);
    }
};
