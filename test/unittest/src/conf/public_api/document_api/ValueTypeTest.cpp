// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../ConfTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/ValueType.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <stdexcept>
#include <string>
#include <unordered_map>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(ValueType)
class ValueTypeTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void testDefaultConstructor() {
        ValueType vt;

        // The default unit should be Undefined
        REQUIRE(vt == ValueType::Undefined);
        REQUIRE_FALSE(vt != ValueType::Undefined);
        REQUIRE(vt.isUndefined());
    }

    void testParameterizedConstructor() {
        ValueType vtUndefined(ValueType::Undefined);
        ValueType vtInteger(ValueType::Integer);
        ValueType vtBoolean(ValueType::Boolean);
        ValueType vtFloat(ValueType::Float);
        ValueType vtText(ValueType::Text);
        ValueType vtDate(ValueType::Date);
        ValueType vtTime(ValueType::Time);
        ValueType vtDateTime(ValueType::DateTime);
        ValueType vtBytes(ValueType::Bytes);
        ValueType vtTimeDelta(ValueType::TimeDelta);
        ValueType vtRegEx(ValueType::RegEx);
        ValueType vtValueList(ValueType::ValueList);
        ValueType vtSectionList(ValueType::SectionList);
        ValueType vtIntermediateSection(ValueType::IntermediateSection);
        ValueType vtSectionWithNames(ValueType::SectionWithNames);
        ValueType vtSectionWithTexts(ValueType::SectionWithTexts);

        // Verify each constructed ValueType
        REQUIRE(vtUndefined == ValueType::Undefined);
        REQUIRE(vtInteger == ValueType::Integer);
        REQUIRE(vtBoolean == ValueType::Boolean);
        REQUIRE(vtFloat == ValueType::Float);
        REQUIRE(vtText == ValueType::Text);
        REQUIRE(vtDate == ValueType::Date);
        REQUIRE(vtTime == ValueType::Time);
        REQUIRE(vtDateTime == ValueType::DateTime);
        REQUIRE(vtBytes == ValueType::Bytes);
        REQUIRE(vtTimeDelta == ValueType::TimeDelta);
        REQUIRE(vtRegEx == ValueType::RegEx);
        REQUIRE(vtValueList == ValueType::ValueList);
        REQUIRE(vtSectionList == ValueType::SectionList);
        REQUIRE(vtIntermediateSection == ValueType::IntermediateSection);
        REQUIRE(vtSectionWithNames == ValueType::SectionWithNames);
        REQUIRE(vtSectionWithTexts == ValueType::SectionWithTexts);
    }

    void testAssignmentFromEnum() {
        ValueType vt;
        vt = ValueType::Float;

        // The unit should now be Float
        REQUIRE(vt == ValueType::Float);
        REQUIRE_FALSE(vt != ValueType::Float);
    }

    void testAssignmentToEnum() {
        ValueType vt(ValueType::Boolean);
        ValueType::Enum enumVal = vt;

        // The enum value should match
        REQUIRE(enumVal == ValueType::Boolean);
    }

    void testOperators() {
        WITH_CONTEXT(
            requireAllOperators<ValueType, ValueType>(
                ValueType::Integer,
                ValueType::Boolean,
                ValueType::SectionWithTexts,
                ValueType::Integer,
                ValueType::Boolean,
                ValueType::SectionWithTexts));
        WITH_CONTEXT(
            requireAllOperators<ValueType, ValueType::Enum>(
                ValueType::Integer,
                ValueType::Boolean,
                ValueType::SectionWithTexts,
                ValueType::Integer,
                ValueType::Boolean,
                ValueType::SectionWithTexts));
        WITH_CONTEXT(
            requireAllOperators<ValueType::Enum, ValueType>(
                ValueType::Integer,
                ValueType::Boolean,
                ValueType::SectionWithTexts,
                ValueType::Integer,
                ValueType::Boolean,
                ValueType::SectionWithTexts));
    }

    void testOrder() {
        WITH_CONTEXT(requireStrictOrder(
            std::array<ValueType, 16>(
                {ValueType::Undefined,
                    ValueType::Integer,
                    ValueType::Boolean,
                    ValueType::Float,
                    ValueType::Text,
                    ValueType::Date,
                    ValueType::Time,
                    ValueType::DateTime,
                    ValueType::Bytes,
                    ValueType::TimeDelta,
                    ValueType::RegEx,
                    ValueType::ValueList,
                    ValueType::SectionList,
                    ValueType::IntermediateSection,
                    ValueType::SectionWithNames,
                    ValueType::SectionWithTexts})));
        WITH_CONTEXT(requireStrictOrder(ValueType::all()));
    }

    void testConversionToEnum() {
        ValueType vtText(ValueType::Text);
        ValueType::Enum enumVal = static_cast<ValueType::Enum>(vtText);

        REQUIRE(enumVal == ValueType::Text);
    }

    void testIsUndefined() {
        ValueType vtUndefined;
        ValueType vtInteger(ValueType::Integer);

        REQUIRE(vtUndefined.isUndefined());
        REQUIRE_FALSE(vtInteger.isUndefined());
    }

    void testIsSection() {
        ValueType vtIntermediateSection(ValueType::IntermediateSection);
        ValueType vtSectionWithNames(ValueType::SectionWithNames);
        ValueType vtSectionWithTexts(ValueType::SectionWithTexts);
        ValueType vtValueList(ValueType::ValueList);
        ValueType vtInteger(ValueType::Integer);

        // These should be sections
        REQUIRE(vtIntermediateSection.isMap());
        REQUIRE(vtSectionWithNames.isMap());
        REQUIRE(vtSectionWithTexts.isMap());

        // These should not be sections
        REQUIRE_FALSE(vtValueList.isMap());
        REQUIRE_FALSE(vtInteger.isMap());
    }

    void testIsList() {
        ValueType vtValueList(ValueType::ValueList);
        ValueType vtSectionList(ValueType::SectionList);
        ValueType vtInteger(ValueType::Integer);
        ValueType vtIntermediateSection(ValueType::IntermediateSection);

        REQUIRE(vtValueList.isList());
        REQUIRE(vtSectionList.isList());

        REQUIRE_FALSE(vtInteger.isList());
        REQUIRE_FALSE(vtIntermediateSection.isList());
    }

    void testIsSingle() {
        ValueType vtInteger(ValueType::Integer);
        ValueType vtBoolean(ValueType::Boolean);
        ValueType vtFloat(ValueType::Float);
        ValueType vtText(ValueType::Text);
        ValueType vtDate(ValueType::Date);
        ValueType vtTime(ValueType::Time);
        ValueType vtDateTime(ValueType::DateTime);
        ValueType vtBytes(ValueType::Bytes);
        ValueType vtTimeDelta(ValueType::TimeDelta);
        ValueType vtRegEx(ValueType::RegEx);
        ValueType vtValueList(ValueType::ValueList);
        ValueType vtIntermediateSection(ValueType::IntermediateSection);

        // These should be single value types
        REQUIRE(vtInteger.isScalar());
        REQUIRE(vtBoolean.isScalar());
        REQUIRE(vtFloat.isScalar());
        REQUIRE(vtText.isScalar());
        REQUIRE(vtDate.isScalar());
        REQUIRE(vtTime.isScalar());
        REQUIRE(vtDateTime.isScalar());
        REQUIRE(vtBytes.isScalar());
        REQUIRE(vtTimeDelta.isScalar());
        REQUIRE(vtRegEx.isScalar());

        // These should not be single value types
        REQUIRE_FALSE(vtValueList.isScalar());
        REQUIRE_FALSE(vtIntermediateSection.isScalar());
    }

    void testToText() {
        ValueType vtUndefined(ValueType::Undefined);
        ValueType vtInteger(ValueType::Integer);
        ValueType vtBoolean(ValueType::Boolean);
        ValueType vtFloat(ValueType::Float);
        ValueType vtText(ValueType::Text);
        ValueType vtDate(ValueType::Date);
        ValueType vtTime(ValueType::Time);
        ValueType vtDateTime(ValueType::DateTime);
        ValueType vtBytes(ValueType::Bytes);
        ValueType vtTimeDelta(ValueType::TimeDelta);
        ValueType vtRegEx(ValueType::RegEx);
        ValueType vtValueList(ValueType::ValueList);
        ValueType vtSectionList(ValueType::SectionList);
        ValueType vtIntermediateSection(ValueType::IntermediateSection);
        ValueType vtSectionWithNames(ValueType::SectionWithNames);
        ValueType vtSectionWithTexts(ValueType::SectionWithTexts);

        // Verify toText() returns correct string representations
        REQUIRE(vtUndefined.toText() == "Undefined"_el);
        REQUIRE(vtInteger.toText() == "Integer"_el);
        REQUIRE(vtBoolean.toText() == "Boolean"_el);
        REQUIRE(vtFloat.toText() == "Float"_el);
        REQUIRE(vtText.toText() == "Text"_el);
        REQUIRE(vtDate.toText() == "Date"_el);
        REQUIRE(vtTime.toText() == "Time"_el);
        REQUIRE(vtDateTime.toText() == "DateTime"_el);
        REQUIRE(vtBytes.toText() == "Bytes"_el);
        REQUIRE(vtTimeDelta.toText() == "TimeDelta"_el);
        REQUIRE(vtRegEx.toText() == "RegEx"_el);
        REQUIRE(vtValueList.toText() == "ValueList"_el);
        REQUIRE(vtSectionList.toText() == "SectionList"_el);
        REQUIRE(vtIntermediateSection.toText() == "IntermediateSection"_el);
        REQUIRE(vtSectionWithNames.toText() == "SectionWithNames"_el);
        REQUIRE(vtSectionWithTexts.toText() == "SectionWithTexts"_el);
    }

    // Test the hash specialization by using ValueType as a key in unordered_map
    void testHashSpecialization() {
        std::unordered_map<ValueType, std::string> vtMap;

        // Insert all ValueType enums with their toText() as values
        for (const auto &enumVal : ValueType::all()) {
            ValueType vt(enumVal);
            vtMap[vt] = el::text::StringConverter{vt.toText()}.toStdString();
        }

        // Verify that all inserted keys are present with correct values
        for (const auto &enumVal : ValueType::all()) {
            ValueType vt(enumVal);
            auto it = vtMap.find(vt);
            REQUIRE(it != vtMap.end());
            REQUIRE(it->second == el::text::StringConverter{vt.toText()}.toStdString());
        }

        // Verify specific entries
        ValueType vtInteger(ValueType::Integer);
        REQUIRE(vtMap.find(vtInteger) != vtMap.end());
        REQUIRE(vtMap[vtInteger] == "Integer");

        ValueType vtSectionWithTexts(ValueType::SectionWithTexts);
        REQUIRE(vtMap.find(vtSectionWithTexts) != vtMap.end());
        REQUIRE(vtMap[vtSectionWithTexts] == "SectionWithTexts");
    }

    void testEnumerationCompleteness() {
        constexpr std::array expectedEnums = {
            ValueType::Undefined,
            ValueType::Integer,
            ValueType::Boolean,
            ValueType::Float,
            ValueType::Text,
            ValueType::Date,
            ValueType::Time,
            ValueType::DateTime,
            ValueType::Bytes,
            ValueType::TimeDelta,
            ValueType::RegEx,
            ValueType::ValueList,
            ValueType::SectionList,
            ValueType::IntermediateSection,
            ValueType::SectionWithNames,
            ValueType::SectionWithTexts};

        // Verify that each enum is handled correctly
        for (const auto &enumVal : expectedEnums) {
            ValueType const vt(enumVal);
            REQUIRE_FALSE(vt.toText().isEmpty());
        }
    }

    void testDeriveFromNativeType() {
        ValueType vt;
        vt = ValueType::from<int8_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<int16_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<int32_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<int64_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<uint8_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<uint16_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<uint32_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<uint64_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<float>();
        REQUIRE_EQUAL(vt, ValueType::Float);
        vt = ValueType::from<double>();
        REQUIRE_EQUAL(vt, ValueType::Float);
        vt = ValueType::from<bool>();
        REQUIRE_EQUAL(vt, ValueType::Boolean);
        vt = ValueType::from<el::text::String>();
        REQUIRE_EQUAL(vt, ValueType::Text);
        vt = ValueType::from<el::time::Date>();
        REQUIRE_EQUAL(vt, ValueType::Date);
        vt = ValueType::from<el::time::TimeWithZone>();
        REQUIRE_EQUAL(vt, ValueType::Time);
        vt = ValueType::from<el::time::DateTime>();
        REQUIRE_EQUAL(vt, ValueType::DateTime);
        vt = ValueType::from<el::mem::ByteBlock>();
        REQUIRE_EQUAL(vt, ValueType::Bytes);
        vt = ValueType::from<el::time::CalendarDelta>();
        REQUIRE_EQUAL(vt, ValueType::TimeDelta);

        vt = ValueType::from<const int8_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const int16_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const int32_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const int64_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const uint8_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const uint16_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const uint32_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const uint64_t>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const float>();
        REQUIRE_EQUAL(vt, ValueType::Float);
        vt = ValueType::from<const double>();
        REQUIRE_EQUAL(vt, ValueType::Float);
        vt = ValueType::from<const bool>();
        REQUIRE_EQUAL(vt, ValueType::Boolean);
        vt = ValueType::from<const el::text::String>();
        REQUIRE_EQUAL(vt, ValueType::Text);
        vt = ValueType::from<const el::time::Date>();
        REQUIRE_EQUAL(vt, ValueType::Date);
        vt = ValueType::from<const el::time::TimeWithZone>();
        REQUIRE_EQUAL(vt, ValueType::Time);
        vt = ValueType::from<const el::time::DateTime>();
        REQUIRE_EQUAL(vt, ValueType::DateTime);
        vt = ValueType::from<const el::mem::ByteBlock>();
        REQUIRE_EQUAL(vt, ValueType::Bytes);
        vt = ValueType::from<const el::time::CalendarDelta>();
        REQUIRE_EQUAL(vt, ValueType::TimeDelta);

        vt = ValueType::from<const int8_t &>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const int16_t &>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const int32_t &>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const int64_t &>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const uint8_t &>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const uint16_t &>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const uint32_t &>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const uint64_t &>();
        REQUIRE_EQUAL(vt, ValueType::Integer);
        vt = ValueType::from<const float &>();
        REQUIRE_EQUAL(vt, ValueType::Float);
        vt = ValueType::from<const double &>();
        REQUIRE_EQUAL(vt, ValueType::Float);
        vt = ValueType::from<const bool &>();
        REQUIRE_EQUAL(vt, ValueType::Boolean);
        vt = ValueType::from<const el::text::String &>();
        REQUIRE_EQUAL(vt, ValueType::Text);
        vt = ValueType::from<const el::time::Date &>();
        REQUIRE_EQUAL(vt, ValueType::Date);
        vt = ValueType::from<const el::time::TimeWithZone &>();
        REQUIRE_EQUAL(vt, ValueType::Time);
        vt = ValueType::from<const el::time::DateTime &>();
        REQUIRE_EQUAL(vt, ValueType::DateTime);
        vt = ValueType::from<const el::mem::ByteBlock &>();
        REQUIRE_EQUAL(vt, ValueType::Bytes);
        vt = ValueType::from<const el::time::CalendarDelta &>();
        REQUIRE_EQUAL(vt, ValueType::TimeDelta);
    }
};
