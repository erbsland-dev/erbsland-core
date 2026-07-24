// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Document Value)
class ValueAsMethodsTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    template <typename T>
    void requireFail(T (Value::*defaultFn)() const, T (Value::*throwFn)() const) {

        T result;
        REQUIRE_NOTHROW(result = (value.get()->*defaultFn)());
        REQUIRE_EQUAL(result, T{});
        REQUIRE_NOTHROW(result = value->asType<T>());
        REQUIRE_EQUAL(result, T{});
        try {
            result = (value.get()->*throwFn)();
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::TypeMismatch);
        }
        try {
            result = value->asTypeOrThrow<T>();
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::TypeMismatch);
        }
    }

    TESTED_TARGETS(asInteger)
    void testAsInteger() {
        WITH_CONTEXT(setupTemplate2("123"));
        // valid conversion
        REQUIRE_EQUAL(value->asInteger(), 123);
        REQUIRE_EQUAL(value->asIntegerOrThrow(), 123);
        REQUIRE_EQUAL(value->asType<uint8_t>(), 123U);
        REQUIRE_EQUAL(value->asType<int8_t>(), 123);
        REQUIRE_EQUAL(value->asType<uint16_t>(), 123U);
        REQUIRE_EQUAL(value->asType<int16_t>(), 123);
        REQUIRE_EQUAL(value->asType<uint32_t>(), 123U);
        REQUIRE_EQUAL(value->asType<int32_t>(), 123);
        REQUIRE_EQUAL(value->asType<uint64_t>(), 123U);
        REQUIRE_EQUAL(value->asType<int64_t>(), 123);
        // all other types must fail.
        // WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDate, &Value::asDateOrThrow));
        WITH_CONTEXT(requireFail(&Value::asTimeWithZone, &Value::asTimeWithZoneOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDateTime, &Value::asDateTimeOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBytes, &Value::asBytesOrThrow));
        WITH_CONTEXT(requireFail(&Value::asCalendarDelta, &Value::asCalendarDeltaOrThrow));
        WITH_CONTEXT(requireFail(&Value::asRegEx, &Value::asRegExOrThrow));
        WITH_CONTEXT(requireFail(&Value::asValueList, &Value::asValueListOrThrow));
    }

    TESTED_TARGETS(asBoolean)
    void testAsBoolean() {
        setupTemplate2("true");
        // valid conversion
        REQUIRE_EQUAL(value->asBoolean(), true);
        REQUIRE_EQUAL(value->asBooleanOrThrow(), true);
        REQUIRE_EQUAL(value->asType<bool>(), true);
        REQUIRE_EQUAL(value->asTypeOrThrow<bool>(), true);
        // all other types must fail.
        WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        // WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDate, &Value::asDateOrThrow));
        WITH_CONTEXT(requireFail(&Value::asTimeWithZone, &Value::asTimeWithZoneOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDateTime, &Value::asDateTimeOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBytes, &Value::asBytesOrThrow));
        WITH_CONTEXT(requireFail(&Value::asCalendarDelta, &Value::asCalendarDeltaOrThrow));
        WITH_CONTEXT(requireFail(&Value::asRegEx, &Value::asRegExOrThrow));
        WITH_CONTEXT(requireFail(&Value::asValueList, &Value::asValueListOrThrow));
    }

    TESTED_TARGETS(asFloat)
    void testAsFloat() {
        setupTemplate2("123.5");
        // valid conversion
        REQUIRE_LESS(std::abs(value->asFloat() - 123.5), std::numeric_limits<double>::epsilon());
        REQUIRE_LESS(std::abs(value->asFloatOrThrow() - 123.5), std::numeric_limits<double>::epsilon());
        REQUIRE_LESS(std::abs(value->asType<double>() - 123.5), std::numeric_limits<double>::epsilon());
        REQUIRE_LESS(std::abs(value->asType<float>() - 123.5), std::numeric_limits<float>::epsilon());
        REQUIRE_LESS(std::abs(value->asTypeOrThrow<double>() - 123.5), std::numeric_limits<double>::epsilon());
        REQUIRE_LESS(std::abs(value->asTypeOrThrow<float>() - 123.5), std::numeric_limits<float>::epsilon());
        // all other types must fail.
        WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        // WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDate, &Value::asDateOrThrow));
        WITH_CONTEXT(requireFail(&Value::asTimeWithZone, &Value::asTimeWithZoneOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDateTime, &Value::asDateTimeOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBytes, &Value::asBytesOrThrow));
        WITH_CONTEXT(requireFail(&Value::asCalendarDelta, &Value::asCalendarDeltaOrThrow));
        WITH_CONTEXT(requireFail(&Value::asRegEx, &Value::asRegExOrThrow));
        WITH_CONTEXT(requireFail(&Value::asValueList, &Value::asValueListOrThrow));
    }

    TESTED_TARGETS(asText)
    void testAsText() {
        setupTemplate2("\"text\"");
        // valid conversion
        REQUIRE_EQUAL(value->asText(), el::text::String{"text"_el});
        REQUIRE_EQUAL(value->asTextOrThrow(), el::text::String{"text"_el});
        REQUIRE_EQUAL(value->asType<el::text::String>(), el::text::String{"text"_el});
        REQUIRE_EQUAL(value->asTypeOrThrow<el::text::String>(), el::text::String{"text"_el});
        // all other types must fail.
        WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        // WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDate, &Value::asDateOrThrow));
        WITH_CONTEXT(requireFail(&Value::asTimeWithZone, &Value::asTimeWithZoneOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDateTime, &Value::asDateTimeOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBytes, &Value::asBytesOrThrow));
        WITH_CONTEXT(requireFail(&Value::asCalendarDelta, &Value::asCalendarDeltaOrThrow));
        WITH_CONTEXT(requireFail(&Value::asRegEx, &Value::asRegExOrThrow));
        WITH_CONTEXT(requireFail(&Value::asValueList, &Value::asValueListOrThrow));
    }

    TESTED_TARGETS(asDate)
    void testAsDate() {
        setupTemplate2("2025-01-20");
        const auto expected = makeDate(2025, 1, 20);
        // valid conversion
        REQUIRE_EQUAL(value->asDate(), expected);
        REQUIRE_EQUAL(value->asDateOrThrow(), expected);
        REQUIRE_EQUAL(value->asType<el::time::Date>(), expected);
        REQUIRE_EQUAL(value->asTypeOrThrow<el::time::Date>(), expected);
        // all other types must fail.
        WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asTimeWithZone, &Value::asTimeWithZoneOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDateTime, &Value::asDateTimeOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBytes, &Value::asBytesOrThrow));
        WITH_CONTEXT(requireFail(&Value::asCalendarDelta, &Value::asCalendarDeltaOrThrow));
        WITH_CONTEXT(requireFail(&Value::asRegEx, &Value::asRegExOrThrow));
        WITH_CONTEXT(requireFail(&Value::asValueList, &Value::asValueListOrThrow));
    }

    TESTED_TARGETS(asTime asTimeWithZone)
    void testAsTime() {
        setupTemplate2("14:08:32");
        const auto expected = makeTime(14, 8, 32, 0);
        // valid conversion
        REQUIRE_EQUAL(value->asTime(), expected);
        REQUIRE_EQUAL(value->asTimeOrThrow(), expected);
        REQUIRE_EQUAL(value->asType<el::time::Time>(), expected);
        REQUIRE_EQUAL(value->asTypeOrThrow<el::time::Time>(), expected);
        const auto expectedWithZone = el::time::TimeWithZone{expected, el::time::TimeZone::local()};
        REQUIRE_EQUAL(value->asTimeWithZone(), expectedWithZone);
        REQUIRE_EQUAL(value->asTimeWithZoneOrThrow(), expectedWithZone);
        REQUIRE_EQUAL(value->asType<el::time::TimeWithZone>(), expectedWithZone);
        REQUIRE_EQUAL(value->asTypeOrThrow<el::time::TimeWithZone>(), expectedWithZone);

        setupTemplate2("14:08:32+02:00");
        const auto explicitZone = makeTimeWithZone(14, 8, 32, 0, 7200);
        REQUIRE_EQUAL(value->asTime(), expected);
        REQUIRE_EQUAL(value->asTimeWithZone(), explicitZone);
        // all other types must fail.
        WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDate, &Value::asDateOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDateTime, &Value::asDateTimeOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBytes, &Value::asBytesOrThrow));
        WITH_CONTEXT(requireFail(&Value::asCalendarDelta, &Value::asCalendarDeltaOrThrow));
        WITH_CONTEXT(requireFail(&Value::asRegEx, &Value::asRegExOrThrow));
        WITH_CONTEXT(requireFail(&Value::asValueList, &Value::asValueListOrThrow));
    }

    TESTED_TARGETS(asDateTime)
    void testAsDateTime() {
        setupTemplate2("2025-01-20 14:08:32");
        const auto expected = makeDateTime(2025, 1, 20, 14, 8, 32, 0);
        // valid conversion
        REQUIRE_EQUAL(value->asDateTime(), expected);
        REQUIRE_EQUAL(value->asDateTimeOrThrow(), expected);
        REQUIRE_EQUAL(value->asType<el::time::DateTime>(), expected);
        REQUIRE_EQUAL(value->asTypeOrThrow<el::time::DateTime>(), expected);
        // all other types must fail.
        WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDate, &Value::asDateOrThrow));
        WITH_CONTEXT(requireFail(&Value::asTimeWithZone, &Value::asTimeWithZoneOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBytes, &Value::asBytesOrThrow));
        WITH_CONTEXT(requireFail(&Value::asCalendarDelta, &Value::asCalendarDeltaOrThrow));
        WITH_CONTEXT(requireFail(&Value::asRegEx, &Value::asRegExOrThrow));
        WITH_CONTEXT(requireFail(&Value::asValueList, &Value::asValueListOrThrow));
    }

    TESTED_TARGETS(asBytes)
    void testAsBytes() {
        setupTemplate2("<01 02 03>");
        const auto expected = bytesFromHex("010203"_el);
        // valid conversion
        REQUIRE_EQUAL(value->asBytes(), expected);
        REQUIRE_EQUAL(value->asBytesOrThrow(), expected);
        REQUIRE_EQUAL(value->asType<el::mem::ByteBlock>(), expected);
        REQUIRE_EQUAL(value->asTypeOrThrow<el::mem::ByteBlock>(), expected);
        // all other types must fail.
        WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDate, &Value::asDateOrThrow));
        WITH_CONTEXT(requireFail(&Value::asTimeWithZone, &Value::asTimeWithZoneOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDateTime, &Value::asDateTimeOrThrow));
        WITH_CONTEXT(requireFail(&Value::asCalendarDelta, &Value::asCalendarDeltaOrThrow));
        WITH_CONTEXT(requireFail(&Value::asRegEx, &Value::asRegExOrThrow));
        WITH_CONTEXT(requireFail(&Value::asValueList, &Value::asValueListOrThrow));
    }

    TESTED_TARGETS(asCalendarDelta)
    void testAsTimeDelta() {
        setupTemplate2("10 weeks");
        const auto expected = el::time::CalendarDelta{el::time::Weeks{10}};
        // valid conversion
        REQUIRE_EQUAL(value->asCalendarDelta(), expected);
        REQUIRE_EQUAL(value->asCalendarDeltaOrThrow(), expected);
        REQUIRE_EQUAL(value->asType<el::time::CalendarDelta>(), expected);
        REQUIRE_EQUAL(value->asTypeOrThrow<el::time::CalendarDelta>(), expected);
        // all other types must fail.
        WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDate, &Value::asDateOrThrow));
        WITH_CONTEXT(requireFail(&Value::asTimeWithZone, &Value::asTimeWithZoneOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDateTime, &Value::asDateTimeOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBytes, &Value::asBytesOrThrow));
        WITH_CONTEXT(requireFail(&Value::asRegEx, &Value::asRegExOrThrow));
        WITH_CONTEXT(requireFail(&Value::asValueList, &Value::asValueListOrThrow));
    }

    TESTED_TARGETS(asRegEx)
    void testAsRegEx() {
        setupTemplate2("/regex/");
        const auto expected = el::re::RegEx::compile("regex"_el);
        // valid conversion
        REQUIRE(value->asRegEx() != nullptr);
        REQUIRE_EQUAL(value->asRegEx()->pattern(), expected->pattern());
        REQUIRE_EQUAL(value->asRegExOrThrow()->pattern(), expected->pattern());
        REQUIRE_EQUAL(value->asType<el::re::RegExPtr>()->pattern(), expected->pattern());
        REQUIRE_EQUAL(value->asTypeOrThrow<el::re::RegExPtr>()->pattern(), expected->pattern());
        // all other types must fail.
        WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDate, &Value::asDateOrThrow));
        WITH_CONTEXT(requireFail(&Value::asTimeWithZone, &Value::asTimeWithZoneOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDateTime, &Value::asDateTimeOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBytes, &Value::asBytesOrThrow));
        WITH_CONTEXT(requireFail(&Value::asCalendarDelta, &Value::asCalendarDeltaOrThrow));
        WITH_CONTEXT(requireFail(&Value::asValueList, &Value::asValueListOrThrow));
    }

    TESTED_TARGETS(asValueList)
    void testAsValueList() {
        setupTemplate2("1, 2, 3");
        const auto list = value->asValueList();
        // valid conversion
        REQUIRE_EQUAL(list.size(), 3U);
        REQUIRE_EQUAL(list[0]->asInteger(), 1);
        REQUIRE_EQUAL(list[1]->asInteger(), 2);
        REQUIRE_EQUAL(list[2]->asInteger(), 3);
        REQUIRE_EQUAL(value->asValueListOrThrow().size(), 3U);
        // all other types must fail.
        WITH_CONTEXT(requireFail(&Value::asInteger, &Value::asIntegerOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBoolean, &Value::asBooleanOrThrow));
        WITH_CONTEXT(requireFail(&Value::asFloat, &Value::asFloatOrThrow));
        WITH_CONTEXT(requireFail(&Value::asText, &Value::asTextOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDate, &Value::asDateOrThrow));
        WITH_CONTEXT(requireFail(&Value::asTimeWithZone, &Value::asTimeWithZoneOrThrow));
        WITH_CONTEXT(requireFail(&Value::asDateTime, &Value::asDateTimeOrThrow));
        WITH_CONTEXT(requireFail(&Value::asBytes, &Value::asBytesOrThrow));
        WITH_CONTEXT(requireFail(&Value::asCalendarDelta, &Value::asCalendarDeltaOrThrow));
        WITH_CONTEXT(requireFail(&Value::asRegEx, &Value::asRegExOrThrow));
    }

    void testIntegerRanges() {
        setupTemplate2("-0x7000'0000'0000'0000");
        REQUIRE_EQUAL(value->asType<Integer>(), -0x7000'0000'0000'0000LL);
        REQUIRE_EQUAL(value->asType<int64_t>(), -0x7000'0000'0000'0000LL);
        REQUIRE_EQUAL(value->asType<uint64_t>(), 0);
        REQUIRE_EQUAL(value->asType<int32_t>(), std::numeric_limits<int32_t>::min());
        REQUIRE_EQUAL(value->asType<uint32_t>(), 0);
        REQUIRE_EQUAL(value->asType<int16_t>(), std::numeric_limits<int16_t>::min());
        REQUIRE_EQUAL(value->asType<uint16_t>(), 0);
        REQUIRE_EQUAL(value->asType<int8_t>(), std::numeric_limits<int8_t>::min());
        REQUIRE_EQUAL(value->asType<uint8_t>(), 0);
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<int32_t>());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<uint32_t>());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<int16_t>());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<uint16_t>());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<int8_t>());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<uint8_t>());
        setupTemplate2("0x7000'0000'0000'0000");
        REQUIRE_EQUAL(value->asType<Integer>(), 0x7000'0000'0000'0000LL);
        REQUIRE_EQUAL(value->asType<int64_t>(), 0x7000'0000'0000'0000LL);
        REQUIRE_EQUAL(value->asType<uint64_t>(), 0x7000'0000'0000'0000ULL);
        REQUIRE_EQUAL(value->asType<int32_t>(), std::numeric_limits<int32_t>::max());
        REQUIRE_EQUAL(value->asType<uint32_t>(), std::numeric_limits<uint32_t>::max());
        REQUIRE_EQUAL(value->asType<int16_t>(), std::numeric_limits<int16_t>::max());
        REQUIRE_EQUAL(value->asType<uint16_t>(), std::numeric_limits<uint16_t>::max());
        REQUIRE_EQUAL(value->asType<int8_t>(), std::numeric_limits<int8_t>::max());
        REQUIRE_EQUAL(value->asType<uint8_t>(), std::numeric_limits<uint8_t>::max());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<int32_t>());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<uint32_t>());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<int16_t>());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<uint16_t>());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<int8_t>());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<uint8_t>());
    }

    template <typename T>
    auto compareFloat(const T a, const T b) -> bool {
        return std::abs(a - b) < std::numeric_limits<T>::epsilon();
    }

    void testFloatRanges() {
        setupTemplate2("-1.0e+300");
        REQUIRE(compareFloat(value->asType<Float>(), -1.0e+300));
        REQUIRE(compareFloat(value->asType<double>(), -1.0e+300));
        REQUIRE(compareFloat(value->asType<float>(), std::numeric_limits<float>::lowest()));
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<float>());
        setupTemplate2("1.0e+300");
        REQUIRE_EQUAL(value->asType<Float>(), 1.0e+300);
        REQUIRE_EQUAL(value->asType<double>(), 1.0e+300);
        REQUIRE_EQUAL(value->asType<float>(), std::numeric_limits<float>::max());
        REQUIRE_THROWS_AS(ConfError, value->asTypeOrThrow<float>());
    }

    void testAsList() {
        setupTemplate2("1, 2, 3");
        REQUIRE_EQUAL(value->asList<int>(), std::vector<int>({1, 2, 3}));
        REQUIRE_EQUAL(value->asList<uint64_t>(), std::vector<uint64_t>({1U, 2U, 3U}));
        REQUIRE_EQUAL(value->asListOrThrow<int>(), std::vector<int>({1, 2, 3}));
        REQUIRE_EQUAL(value->asListOrThrow<uint64_t>(), std::vector<uint64_t>({1U, 2U, 3U}));
        REQUIRE(value->asList<el::text::String>().empty());
        REQUIRE_THROWS_AS(ConfError, value->asListOrThrow<el::text::String>());
        setupTemplate2("1, 2, true");
        REQUIRE(value->asList<int>().empty());
        REQUIRE_THROWS_AS(ConfError, value->asListOrThrow<int>());
    }
};
