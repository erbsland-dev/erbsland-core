// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/RuleType.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>

using namespace el::conf;
using namespace el::text::literals;
using namespace el::conf::vr;

TESTED_TARGETS(RuleType)
class RuleTypeTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void testDefaultConstructor() {
        RuleType ruleType;

        REQUIRE(ruleType == RuleType::Undefined);
        REQUIRE(ruleType.isUndefined());
        REQUIRE(ruleType.raw() == RuleType::Undefined);
    }

    void testParameterizedConstructor() {
        RuleType ruleTypeInteger{RuleType::Integer};
        RuleType ruleTypeSection{RuleType::Section};
        RuleType ruleTypeAlternatives{RuleType::Alternatives};

        REQUIRE(ruleTypeInteger == RuleType::Integer);
        REQUIRE(ruleTypeSection == RuleType::Section);
        REQUIRE(ruleTypeAlternatives == RuleType::Alternatives);
    }

    void testAcceptsDefaults() {
        const std::array<RuleType, 5> disallowed = {
            RuleType::Section,
            RuleType::SectionList,
            RuleType::SectionWithTexts,
            RuleType::NotValidated,
            RuleType::Alternatives};
        for (const auto &ruleType : disallowed) {
            REQUIRE_FALSE(ruleType.acceptsDefaults());
        }
        REQUIRE(RuleType{RuleType::Integer}.acceptsDefaults());
        REQUIRE(RuleType{RuleType::Value}.acceptsDefaults());
    }

    void testMatchesValueType() {
        REQUIRE_FALSE(RuleType{RuleType::Undefined}.matchesValueType(ValueType::Integer));

        REQUIRE(RuleType{RuleType::Value}.matchesValueType(ValueType::Integer));
        REQUIRE_FALSE(RuleType{RuleType::Value}.matchesValueType(ValueType::ValueList));

        REQUIRE(RuleType{RuleType::ValueList}.matchesValueType(ValueType::ValueList));
        REQUIRE(RuleType{RuleType::ValueList}.matchesValueType(ValueType::Float));
        REQUIRE_FALSE(RuleType{RuleType::ValueList}.matchesValueType(ValueType::SectionList));

        REQUIRE(RuleType{RuleType::ValueMatrix}.matchesValueType(ValueType::ValueList));
        REQUIRE(RuleType{RuleType::ValueMatrix}.matchesValueType(ValueType::Boolean));
        REQUIRE_FALSE(RuleType{RuleType::ValueMatrix}.matchesValueType(ValueType::SectionWithTexts));

        REQUIRE(RuleType{RuleType::Section}.matchesValueType(ValueType::SectionWithNames));
        REQUIRE(RuleType{RuleType::Section}.matchesValueType(ValueType::IntermediateSection));
        REQUIRE_FALSE(RuleType{RuleType::Section}.matchesValueType(ValueType::SectionList));

        REQUIRE(RuleType{RuleType::NotValidated}.matchesValueType(ValueType::SectionWithTexts));
        REQUIRE(RuleType{RuleType::Alternatives}.matchesValueType(ValueType::SectionList));

        REQUIRE(RuleType{RuleType::Integer}.matchesValueType(ValueType::Integer));
        REQUIRE_FALSE(RuleType{RuleType::Integer}.matchesValueType(ValueType::Boolean));
    }

    void testToTextAndValueTypeMapping() {
        // pin down mappings to prevent accidental changes
        struct Mapping {
            RuleType ruleType;
            el::text::String text;
            ValueType valueType;
            el::text::String expectedValueTypeText;
        };
        const std::array<Mapping, 19> mappings = {
            Mapping{RuleType::Undefined, el::text::String{"Undefined"_el}, ValueType::Undefined, el::text::String{}},
            Mapping{
                RuleType::Integer,
                el::text::String{"Integer"_el},
                ValueType::Integer,
                el::text::String{"an integer value"_el}},
            Mapping{
                RuleType::Boolean,
                el::text::String{"Boolean"_el},
                ValueType::Boolean,
                el::text::String{"a Boolean value"_el}},
            Mapping{
                RuleType::Float,
                el::text::String{"Float"_el},
                ValueType::Float,
                el::text::String{"a floating-point or integer value"_el}},
            Mapping{RuleType::Text, el::text::String{"Text"_el}, ValueType::Text, el::text::String{"a text value"_el}},
            Mapping{RuleType::Date, el::text::String{"Date"_el}, ValueType::Date, el::text::String{"a date value"_el}},
            Mapping{RuleType::Time, el::text::String{"Time"_el}, ValueType::Time, el::text::String{"a time value"_el}},
            Mapping{
                RuleType::DateTime,
                el::text::String{"DateTime"_el},
                ValueType::DateTime,
                el::text::String{"a date-time value"_el}},
            Mapping{
                RuleType::Bytes, el::text::String{"Bytes"_el}, ValueType::Bytes, el::text::String{"a byte value"_el}},
            Mapping{
                RuleType::TimeDelta,
                el::text::String{"TimeDelta"_el},
                ValueType::TimeDelta,
                el::text::String{"a time-delta value"_el}},
            Mapping{
                RuleType::RegEx,
                el::text::String{"RegEx"_el},
                ValueType::RegEx,
                el::text::String{"a regular expression"_el}},
            Mapping{
                RuleType::Value,
                el::text::String{"Value"_el},
                ValueType::Undefined,
                el::text::String{"any scalar value"_el}},
            Mapping{
                RuleType::ValueList,
                el::text::String{"ValueList"_el},
                ValueType::ValueList,
                el::text::String{"a value list or scalar value"_el}},
            Mapping{
                RuleType::ValueMatrix,
                el::text::String{"ValueMatrix"_el},
                ValueType::Undefined,
                el::text::String{"a nested value list or scalar value"_el}},
            Mapping{
                RuleType::Section,
                el::text::String{"Section"_el},
                ValueType::SectionWithNames,
                el::text::String{"a section"_el}},
            Mapping{
                RuleType::SectionList,
                el::text::String{"SectionList"_el},
                ValueType::SectionList,
                el::text::String{"a section list"_el}},
            Mapping{
                RuleType::SectionWithTexts,
                el::text::String{"SectionWithTexts"_el},
                ValueType::SectionWithTexts,
                el::text::String{"a section with texts"_el}},
            Mapping{
                RuleType::NotValidated, el::text::String{"NotValidated"_el}, ValueType::Undefined, el::text::String{}},
            Mapping{
                RuleType::Alternatives, el::text::String{"Alternatives"_el}, ValueType::Undefined, el::text::String{}}};

        for (const auto &mapping : mappings) {
            RuleType ruleType(mapping.ruleType);
            REQUIRE_EQUAL(ruleType.toText(), mapping.text);
            REQUIRE(ruleType.toValueType() == mapping.valueType);
            REQUIRE_EQUAL(ruleType.expectedValueTypeText(), mapping.expectedValueTypeText);
        }
    }

    void testFromText() {
        struct Mapping {
            el::text::String text;
            RuleType::Enum ruleType;
        };
        const std::array<Mapping, 9> mappings = {
            Mapping{el::text::String{"integer"_el}, RuleType::Integer},
            Mapping{el::text::String{"DateTime"_el}, RuleType::DateTime},
            Mapping{el::text::String{"date_time"_el}, RuleType::DateTime},
            Mapping{el::text::String{"value_list"_el}, RuleType::ValueList},
            Mapping{el::text::String{"section_with_names"_el}, RuleType::Section},
            Mapping{el::text::String{"SECTION_WITH_TEXTS"_el}, RuleType::SectionWithTexts},
            Mapping{el::text::String{"notvalidated"_el}, RuleType::NotValidated},
            Mapping{el::text::String{"time_delta"_el}, RuleType::TimeDelta},
            Mapping{el::text::String{"regex"_el}, RuleType::RegEx}};

        for (const auto &mapping : mappings) {
            REQUIRE(RuleType::fromText(mapping.text) == mapping.ruleType);
        }

        REQUIRE(RuleType::fromText(el::text::String{}) == RuleType::Undefined);
        REQUIRE(RuleType::fromText(el::text::String{"unknown"_el}) == RuleType::Undefined);
        REQUIRE(RuleType::fromText(el::text::String{"123456789012345678901"_el}) == RuleType::Undefined);
    }

    void testAllEnumeration() {
        const std::array<RuleType, 19> expected = {
            RuleType::Undefined,
            RuleType::Integer,
            RuleType::Boolean,
            RuleType::Float,
            RuleType::Text,
            RuleType::Date,
            RuleType::Time,
            RuleType::DateTime,
            RuleType::Bytes,
            RuleType::TimeDelta,
            RuleType::RegEx,
            RuleType::Value,
            RuleType::ValueList,
            RuleType::ValueMatrix,
            RuleType::Section,
            RuleType::SectionList,
            RuleType::SectionWithTexts,
            RuleType::NotValidated,
            RuleType::Alternatives};
        const auto &values = RuleType::all();
        REQUIRE_EQUAL(values.size(), expected.size());
        for (size_t i = 0; i < expected.size(); ++i) {
            REQUIRE(values[i] == expected[i]);
        }
    }
};
