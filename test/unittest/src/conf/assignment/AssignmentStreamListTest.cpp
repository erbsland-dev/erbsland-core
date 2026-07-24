// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssignmentStreamHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(AssignmentStream)
class AssignmentStreamListTest final : public UNITTEST_SUBCLASS(AssignmentStreamHelper) {
public:
    void testValueLists() {
        WITH_CONTEXT(setupAssignmentStream("value_lists.elcl"));
        WITH_CONTEXT(requireSectionMap("main"_el));
        const auto value1_expected = std::vector<ExpectedListEntry>{
            {ValueType::Integer, 1},
            {ValueType::Integer, 2},
            {ValueType::Integer, 3},
            {ValueType::Integer, 4},
            {ValueType::Integer, 5},
        };
        WITH_CONTEXT(requireList("main.value_1"_el, value1_expected));
        const auto value2_expected = std::vector<ExpectedListEntry>{
            {ValueType::Text, el::text::String{"one"_el}},
            {ValueType::Text, el::text::String{"two"_el}},
            {ValueType::Text, el::text::String{"three"_el}},
            {ValueType::Text, el::text::String{"four"_el}},
            {ValueType::Text, el::text::String{"five"_el}},
        };
        WITH_CONTEXT(requireList("main.value_2"_el, value2_expected));
        const auto value3_expected = std::vector<ExpectedListEntry>{
            {ValueType::Integer, 98765},
            {ValueType::Boolean, false},
            {ValueType::Float, 98.76},
            {ValueType::Text, el::text::String{"-text-"_el}},
            {ValueType::Text, el::text::String{"{code}"_el}},
            {ValueType::Date, makeDate(2028, 1, 30)},
            {ValueType::Bytes, bytesFromHex("a1b2c3"_el)}};
        WITH_CONTEXT(requireList("main.value_3"_el, value3_expected));
        const auto value4_expected = std::vector<ExpectedListEntry>{
            {ValueType::Text, el::text::String{"-text-"_el}},
            {ValueType::Integer, 4567},
        };
        WITH_CONTEXT(requireList("main.value_4"_el, value4_expected));
        const auto value5_expected = std::vector<ExpectedListEntry>{
            {ValueType::Integer, 111},
            {ValueType::Integer, 222},
            {ValueType::Integer, 333},
            {ValueType::Integer, 444},
            {ValueType::Integer, 555},
        };
        WITH_CONTEXT(requireList("main.value_5"_el, value5_expected));
        const auto value6_expected = std::vector<ExpectedListEntry>{
            {ValueType::Text, el::text::String{"😀"_el}},
            {ValueType::Integer, 34566},
            {ValueType::Float, 77.77},
            {ValueType::Text, el::text::String{"a = \"😆\""_el}},
            {ValueType::Boolean, false},
        };
        WITH_CONTEXT(requireList("main.value_6"_el, value6_expected));
        requireAssignment();
        REQUIRE_EQUAL(assignment.type(), AssignmentType::Value);
        REQUIRE_EQUAL(assignment.value()->type(), ValueType::ValueList);
        const auto valueList = assignment.value()->asValueList();
        REQUIRE_EQUAL(valueList.size(), 8);
        const auto expectedValues = std::array<std::array<int, 8>, 8>{
            std::array{1, 3, 6, 10, 15, 21, 28, 36},
            std::array{2, 5, 9, 14, 20, 27, 35, 44},
            std::array{3, 7, 12, 18, 25, 33, 42, 52},
            std::array{4, 9, 15, 22, 30, 39, 49, 60},
            std::array{5, 11, 18, 26, 35, 45, 56, 68},
            std::array{6, 13, 21, 30, 40, 51, 63, 76},
            std::array{7, 15, 24, 34, 45, 57, 70, 84},
            std::array{8, 17, 27, 38, 50, 63, 77, 92},
        };
        for (std::size_t i = 0; i < valueList.size(); ++i) {
            const auto &rowValue = valueList[i];
            REQUIRE_EQUAL(rowValue->type(), ValueType::ValueList);
            const auto row = rowValue->asValueList();
            REQUIRE_EQUAL(row.size(), 8);
            for (std::size_t j = 0; j < valueList.size(); ++j) {
                const auto &value = row[j];
                REQUIRE_EQUAL(value->type(), ValueType::Integer);
                REQUIRE_EQUAL(value->asInteger(), expectedValues[i][j]);
            }
        }
        WITH_CONTEXT(requireEnd());
    }
};
