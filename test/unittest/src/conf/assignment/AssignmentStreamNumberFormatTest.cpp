// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AssignmentStreamHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(AssignmentStream)
class AssignmentStreamNumberFormatTest final : public UNITTEST_SUBCLASS(AssignmentStreamHelper) {
public:
    void testIntegerFormats() {
        WITH_CONTEXT(setupAssignmentStream("integer_formats.elcl"));
        WITH_CONTEXT(requireSectionMap("decimal"_el));
        WITH_CONTEXT(requireValue("decimal.value_1"_el, ValueType::Integer, 0));
        WITH_CONTEXT(requireValue("decimal.value_2"_el, ValueType::Integer, 1));
        WITH_CONTEXT(requireValue("decimal.value_3"_el, ValueType::Integer, 1));
        WITH_CONTEXT(requireValue("decimal.value_4"_el, ValueType::Integer, -1));
        WITH_CONTEXT(requireValue("decimal.value_5"_el, ValueType::Integer, 100'000));
        WITH_CONTEXT(requireValue("decimal.value_6"_el, ValueType::Integer, 1'000'000));
        WITH_CONTEXT(requireValue("decimal.value_7"_el, ValueType::Integer, std::numeric_limits<int64_t>::min()));
        WITH_CONTEXT(requireValue("decimal.value_8"_el, ValueType::Integer, std::numeric_limits<int64_t>::max()));

        WITH_CONTEXT(requireSectionMap("hexadecimal"_el));
        WITH_CONTEXT(requireValue("hexadecimal.value_1"_el, ValueType::Integer, 0));
        WITH_CONTEXT(requireValue("hexadecimal.value_2"_el, ValueType::Integer, 1));
        WITH_CONTEXT(requireValue("hexadecimal.value_3"_el, ValueType::Integer, 1));
        WITH_CONTEXT(requireValue("hexadecimal.value_4"_el, ValueType::Integer, -1));
        WITH_CONTEXT(requireValue("hexadecimal.value_5"_el, ValueType::Integer, 0x1234ABCD));
        WITH_CONTEXT(requireValue("hexadecimal.value_6"_el, ValueType::Integer, std::numeric_limits<int64_t>::min()));
        WITH_CONTEXT(requireValue("hexadecimal.value_7"_el, ValueType::Integer, std::numeric_limits<int64_t>::max()));

        WITH_CONTEXT(requireSectionMap("binary"_el));
        WITH_CONTEXT(requireValue("binary.value_1"_el, ValueType::Integer, 0));
        WITH_CONTEXT(requireValue("binary.value_2"_el, ValueType::Integer, 1));
        WITH_CONTEXT(requireValue("binary.value_3"_el, ValueType::Integer, 1));
        WITH_CONTEXT(requireValue("binary.value_4"_el, ValueType::Integer, -1));
        WITH_CONTEXT(requireValue("binary.value_5"_el, ValueType::Integer, 0b1000000111111010));
        WITH_CONTEXT(requireValue("binary.value_6"_el, ValueType::Integer, std::numeric_limits<int64_t>::min()));
        WITH_CONTEXT(requireValue("binary.value_7"_el, ValueType::Integer, std::numeric_limits<int64_t>::max()));

        WITH_CONTEXT(requireSectionMap("byte_counts"_el));
        WITH_CONTEXT(requireValue("byte_counts.value_1"_el, ValueType::Integer, 5'000));
        WITH_CONTEXT(requireValue("byte_counts.value_2"_el, ValueType::Integer, 5'120));
        WITH_CONTEXT(requireValue("byte_counts.value_3"_el, ValueType::Integer, 10'000'000));
        WITH_CONTEXT(requireValue("byte_counts.value_4"_el, ValueType::Integer, 1'000'000'000));
        WITH_CONTEXT(requireValue("byte_counts.value_5"_el, ValueType::Integer, 1'000'000'000'000));
        WITH_CONTEXT(requireValue("byte_counts.value_6"_el, ValueType::Integer, 1'000'000'000'000'000));
    }

    void testFloatFormats() {
        WITH_CONTEXT(setupAssignmentStream("float_formats.elcl"));
        WITH_CONTEXT(requireSectionMap("float"_el));
        WITH_CONTEXT(requireFloat("float.value_1"_el, 0.0));
        WITH_CONTEXT(requireFloat("float.value_2"_el, 0.0));
        WITH_CONTEXT(requireFloat("float.value_3"_el, 1.0));
        WITH_CONTEXT(requireFloat("float.value_4"_el, -1.0));
        WITH_CONTEXT(requireFloat("float.value_5"_el, std::numeric_limits<double>::quiet_NaN()));
        WITH_CONTEXT(requireFloat("float.value_6"_el, std::numeric_limits<double>::infinity()));
        WITH_CONTEXT(requireFloat("float.value_7"_el, -std::numeric_limits<double>::infinity()));
        WITH_CONTEXT(requireFloat("float.value_8"_el, std::numeric_limits<double>::infinity()));
        WITH_CONTEXT(requireFloat("float.value_9"_el, 2937.28301));
        WITH_CONTEXT(requireFloat("float.value_10"_el, 5e-12));
        WITH_CONTEXT(requireFloat("float.value_11"_el, 0.02e+8));
        WITH_CONTEXT(requireFloat("float.value_12"_el, 12e+10));
        WITH_CONTEXT(requireFloat("float.value_13"_el, -12.9));
        WITH_CONTEXT(requireFloat("float.value_14"_el, -8283.9e-5));
        WITH_CONTEXT(requireFloat("float.value_15"_el, 1000000.000001));
    }
};
