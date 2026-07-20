// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormatForConf.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Document Value)
class ValueGetMethodsTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    template <typename T, typename U>
    void requireFail(
        T (Value::*defaultFn)(const NamePathLike &, U) const,
        T (Value::*throwFn)(const NamePathLike &) const,
        U defaultValue) {

        const auto namePath = NamePath::fromText("main.value"_el);
        T result;
        REQUIRE_NOTHROW(result = (doc.get()->*defaultFn)(namePath, defaultValue));
        REQUIRE_EQUAL(result, defaultValue);
        REQUIRE_NOTHROW(result = doc->get<T>(namePath, defaultValue));
        REQUIRE_EQUAL(result, defaultValue);
        const auto name = NamePath::fromText("value"_el);
        const auto mainValue = doc->valueOrThrow(el::text::String{"main"_el});
        REQUIRE_NOTHROW(result = (mainValue.get()->*defaultFn)(name, defaultValue));
        REQUIRE_EQUAL(result, defaultValue);
        REQUIRE_NOTHROW(result = mainValue->get<T>(name, defaultValue));
        REQUIRE_EQUAL(result, defaultValue);
        try {
            result = (doc.get()->*throwFn)(namePath);
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::TypeMismatch);
        }
        try {
            result = doc->getOrThrow<T>(namePath);
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::TypeMismatch);
        }
    }

    void requireGetValueListFail() {
        const auto namePath = NamePath::fromText("main.value"_el);
        ValueList result;
        REQUIRE_NOTHROW(result = doc->getValueList(namePath));
        REQUIRE_EQUAL(result, ValueList{});
        try {
            result = doc->getValueListOrThrow(namePath);
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::TypeMismatch);
        }
        const auto name = NamePath::fromText("value"_el);
        const auto mainValue = doc->valueOrThrow(el::text::String{"main"_el});
        REQUIRE_NOTHROW(result = mainValue->getValueList(name));
        REQUIRE_EQUAL(result, ValueList{});
        try {
            result = mainValue->getValueListOrThrow(name);
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), ConfErrorCategory::TypeMismatch);
        }
    }

    TESTED_TARGETS(asInteger)
    void testGetInteger() {
        WITH_CONTEXT(setupTemplate3("123"));
        // valid conversion
        const auto namePath = NamePath::fromText("main.value"_el);
        REQUIRE_EQUAL(doc->getInteger(namePath), 123);
        REQUIRE_EQUAL(doc->getIntegerOrThrow(namePath), 123);
        REQUIRE_EQUAL(doc->get<uint8_t>(namePath), 123U);
        REQUIRE_EQUAL(doc->getOrThrow<uint8_t>(namePath), 123U);
        REQUIRE_EQUAL(doc->get<int8_t>(namePath), 123);
        REQUIRE_EQUAL(doc->getOrThrow<int8_t>(namePath), 123);
        REQUIRE_EQUAL(doc->get<uint16_t>(namePath), 123U);
        REQUIRE_EQUAL(doc->getOrThrow<uint16_t>(namePath), 123U);
        REQUIRE_EQUAL(doc->get<int16_t>(namePath), 123);
        REQUIRE_EQUAL(doc->getOrThrow<int16_t>(namePath), 123);
        REQUIRE_EQUAL(doc->get<uint32_t>(namePath), 123U);
        REQUIRE_EQUAL(doc->getOrThrow<uint32_t>(namePath), 123U);
        REQUIRE_EQUAL(doc->get<int32_t>(namePath), 123);
        REQUIRE_EQUAL(doc->getOrThrow<int32_t>(namePath), 123);
        REQUIRE_EQUAL(doc->get<uint64_t>(namePath), 123U);
        REQUIRE_EQUAL(doc->getOrThrow<uint64_t>(namePath), 123U);
        REQUIRE_EQUAL(doc->get<int64_t>(namePath), 123);
        REQUIRE_EQUAL(doc->getOrThrow<int64_t>(el::text::String{"main.value"_el}), 123);
        // all other types must fail.
        // WITH_CONTEXT(requireFail<Integer, Integer>(&Value::asInteger, &Value::asIntegerOrThrow, 789));
        WITH_CONTEXT(requireFail<bool, bool>(&Value::getBoolean, &Value::getBooleanOrThrow, true));
        WITH_CONTEXT(requireFail<Float, Float>(&Value::getFloat, &Value::getFloatOrThrow, 987.654));
        WITH_CONTEXT(
            requireFail<el::text::String, el::text::String>(
                &Value::getText, &Value::getTextOrThrow, el::text::String{"hello"_el}));
        WITH_CONTEXT(
            requireFail<el::time::Date, const el::time::Date &>(
                &Value::getDate, &Value::getDateOrThrow, makeDate(2024, 11, 10)));
        WITH_CONTEXT(
            requireFail<el::time::TimeWithZone, const el::time::TimeWithZone &>(
                &Value::getTimeWithZone, &Value::getTimeWithZoneOrThrow, makeTime(17, 22, 33, 100)));
        WITH_CONTEXT(
            requireFail<el::time::DateTime, const el::time::DateTime &>(
                &Value::getDateTime,
                &Value::getDateTimeOrThrow,
                el::time::DateTime(makeDate(2024, 11, 10), makeTime(17, 22, 33, 100))));
        WITH_CONTEXT(
            requireFail<el::mem::ByteBlock, const el::mem::ByteBlock &>(
                &Value::getBytes, &Value::getBytesOrThrow, bytesFromHex("ffeedd"_el)));
        WITH_CONTEXT(
            requireFail<el::time::CalendarDelta, const el::time::CalendarDelta &>(
                &Value::getCalendarDelta,
                &Value::getCalendarDeltaOrThrow,
                el::time::CalendarDelta{el::time::Minutes{55}}));
        WITH_CONTEXT(
            requireFail<el::re::RegExPtr, const el::re::RegExPtr &>(
                &Value::getRegEx, &Value::getRegExOrThrow, el::re::RegEx::compile("other"_el)));
        WITH_CONTEXT(requireGetValueListFail());
    }

    TESTED_TARGETS(get getOrThrow)
    void testGetIntegerRangeConversion() {
        const auto namePath = NamePath::fromText("main.value"_el);

        WITH_CONTEXT(setupTemplate3("1000"));
        REQUIRE_EQUAL(doc->get<std::uint8_t>(namePath), std::numeric_limits<std::uint8_t>::max());
        REQUIRE_EQUAL(doc->get<std::int8_t>(namePath), std::numeric_limits<std::int8_t>::max());
        try {
            (void)doc->getOrThrow<std::uint8_t>(namePath);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::TypeMismatch);
        }
        try {
            (void)doc->getOrThrow<std::int8_t>(namePath);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::TypeMismatch);
        }

        WITH_CONTEXT(setupTemplate3("-1000"));
        REQUIRE_EQUAL(doc->get<std::uint8_t>(namePath), std::numeric_limits<std::uint8_t>::min());
        REQUIRE_EQUAL(doc->get<std::int8_t>(namePath), std::numeric_limits<std::int8_t>::min());
        try {
            (void)doc->getOrThrow<std::uint8_t>(namePath);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::TypeMismatch);
        }
        try {
            (void)doc->getOrThrow<std::int8_t>(namePath);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::TypeMismatch);
        }
    }

    TESTED_TARGETS(getBoolean)
    void testGetBoolean() {
        WITH_CONTEXT(setupTemplate3("true"));
        const auto path = NamePath::fromText("main.value"_el);
        REQUIRE_EQUAL(doc->getBoolean(path), true);
        REQUIRE_EQUAL(doc->getBooleanOrThrow(path), true);
        REQUIRE_EQUAL(doc->get<bool>(path, false), true);
        REQUIRE_EQUAL(doc->getOrThrow<bool>(path), true);
        WITH_CONTEXT(requireFail<Integer, Integer>(&Value::getInteger, &Value::getIntegerOrThrow, 0));
        // WITH_CONTEXT(requireFail<bool, bool>(&Value::getBoolean, &Value::getBooleanOrThrow, false));
        WITH_CONTEXT(requireFail<Float, Float>(&Value::getFloat, &Value::getFloatOrThrow, 0.0));
        WITH_CONTEXT(
            requireFail<el::text::String, el::text::String>(
                &Value::getText, &Value::getTextOrThrow, el::text::String{""_el}));
        WITH_CONTEXT(
            requireFail<el::time::Date, const el::time::Date &>(
                &Value::getDate, &Value::getDateOrThrow, el::time::Date{}));
        WITH_CONTEXT(
            requireFail<el::time::TimeWithZone, const el::time::TimeWithZone &>(
                &Value::getTimeWithZone, &Value::getTimeWithZoneOrThrow, el::time::TimeWithZone{}));
        WITH_CONTEXT(
            requireFail<el::time::DateTime, const el::time::DateTime &>(
                &Value::getDateTime, &Value::getDateTimeOrThrow, el::time::DateTime{}));
        WITH_CONTEXT(
            requireFail<el::mem::ByteBlock, const el::mem::ByteBlock &>(
                &Value::getBytes, &Value::getBytesOrThrow, el::mem::ByteBlock{}));
        WITH_CONTEXT(
            requireFail<el::time::CalendarDelta, const el::time::CalendarDelta &>(
                &Value::getCalendarDelta, &Value::getCalendarDeltaOrThrow, el::time::CalendarDelta{}));
        WITH_CONTEXT(
            requireFail<el::re::RegExPtr, const el::re::RegExPtr &>(
                &Value::getRegEx, &Value::getRegExOrThrow, el::re::RegExPtr{}));
        WITH_CONTEXT(requireGetValueListFail());
    }

    TESTED_TARGETS(getFloat)
    void testGetFloat() {
        WITH_CONTEXT(setupTemplate3("123.5"));
        const auto path = NamePath::fromText("main.value"_el);
        REQUIRE_LESS(std::abs(doc->getFloat(path, 0.0) - 123.5), std::numeric_limits<double>::epsilon());
        REQUIRE_LESS(std::abs(doc->getFloatOrThrow(path) - 123.5), std::numeric_limits<double>::epsilon());
        REQUIRE_LESS(std::abs(doc->get<double>(path, 0.0) - 123.5), std::numeric_limits<double>::epsilon());
        REQUIRE_LESS(std::abs(doc->getOrThrow<double>(path) - 123.5), std::numeric_limits<double>::epsilon());
        WITH_CONTEXT(requireFail<Integer, Integer>(&Value::getInteger, &Value::getIntegerOrThrow, 0));
        WITH_CONTEXT(requireFail<bool, bool>(&Value::getBoolean, &Value::getBooleanOrThrow, false));
        // WITH_CONTEXT(requireFail<Float, Float>(&Value::getFloat, &Value::getFloatOrThrow, 0.0));
        WITH_CONTEXT(
            requireFail<el::text::String, el::text::String>(
                &Value::getText, &Value::getTextOrThrow, el::text::String{""_el}));
        WITH_CONTEXT(
            requireFail<el::time::Date, const el::time::Date &>(
                &Value::getDate, &Value::getDateOrThrow, el::time::Date{}));
        WITH_CONTEXT(
            requireFail<el::time::TimeWithZone, const el::time::TimeWithZone &>(
                &Value::getTimeWithZone, &Value::getTimeWithZoneOrThrow, el::time::TimeWithZone{}));
        WITH_CONTEXT(
            requireFail<el::time::DateTime, const el::time::DateTime &>(
                &Value::getDateTime, &Value::getDateTimeOrThrow, el::time::DateTime{}));
        WITH_CONTEXT(
            requireFail<el::mem::ByteBlock, const el::mem::ByteBlock &>(
                &Value::getBytes, &Value::getBytesOrThrow, el::mem::ByteBlock{}));
        WITH_CONTEXT(
            requireFail<el::time::CalendarDelta, const el::time::CalendarDelta &>(
                &Value::getCalendarDelta, &Value::getCalendarDeltaOrThrow, el::time::CalendarDelta{}));
        WITH_CONTEXT(
            requireFail<el::re::RegExPtr, const el::re::RegExPtr &>(
                &Value::getRegEx, &Value::getRegExOrThrow, el::re::RegExPtr{}));
        WITH_CONTEXT(requireGetValueListFail());
    }

    TESTED_TARGETS(getText)
    void testGetText() {
        WITH_CONTEXT(setupTemplate3("\"text\""));
        const auto path = NamePath::fromText("main.value"_el);
        REQUIRE_EQUAL(doc->getText(path, el::text::String{}), el::text::String{"text"_el});
        REQUIRE_EQUAL(doc->getTextOrThrow(path), el::text::String{"text"_el});
        REQUIRE_EQUAL(doc->get<el::text::String>(path, el::text::String{}), el::text::String{"text"_el});
        REQUIRE_EQUAL(doc->getOrThrow<el::text::String>(path), el::text::String{"text"_el});
        WITH_CONTEXT(requireFail<Integer, Integer>(&Value::getInteger, &Value::getIntegerOrThrow, 0));
        WITH_CONTEXT(requireFail<bool, bool>(&Value::getBoolean, &Value::getBooleanOrThrow, false));
        WITH_CONTEXT(requireFail<Float, Float>(&Value::getFloat, &Value::getFloatOrThrow, 0.0));
        // WITH_CONTEXT(requireFail<el::text::String, const el::text::String&>(&Value::getText, &Value::getTextOrThrow,
        // el::text::String{}));
        WITH_CONTEXT(
            requireFail<el::time::Date, const el::time::Date &>(
                &Value::getDate, &Value::getDateOrThrow, el::time::Date{}));
        WITH_CONTEXT(
            requireFail<el::time::TimeWithZone, const el::time::TimeWithZone &>(
                &Value::getTimeWithZone, &Value::getTimeWithZoneOrThrow, el::time::TimeWithZone{}));
        WITH_CONTEXT(
            requireFail<el::time::DateTime, const el::time::DateTime &>(
                &Value::getDateTime, &Value::getDateTimeOrThrow, el::time::DateTime{}));
        WITH_CONTEXT(
            requireFail<el::mem::ByteBlock, const el::mem::ByteBlock &>(
                &Value::getBytes, &Value::getBytesOrThrow, el::mem::ByteBlock{}));
        WITH_CONTEXT(
            requireFail<el::time::CalendarDelta, const el::time::CalendarDelta &>(
                &Value::getCalendarDelta, &Value::getCalendarDeltaOrThrow, el::time::CalendarDelta{}));
        WITH_CONTEXT(
            requireFail<el::re::RegExPtr, const el::re::RegExPtr &>(
                &Value::getRegEx, &Value::getRegExOrThrow, el::re::RegExPtr{}));
        WITH_CONTEXT(requireGetValueListFail());
    }

    TESTED_TARGETS(getDate)
    void testGetDate() {
        WITH_CONTEXT(setupTemplate3("2025-01-20"));
        const auto path = NamePath::fromText("main.value"_el);
        const auto expected = makeDate(2025, 1, 20);
        REQUIRE_EQUAL(doc->getDate(path, el::time::Date{}), expected);
        REQUIRE_EQUAL(doc->getDateOrThrow(path), expected);
        REQUIRE_EQUAL(doc->get<el::time::Date>(path, el::time::Date{}), expected);
        REQUIRE_EQUAL(doc->getOrThrow<el::time::Date>(path), expected);
        WITH_CONTEXT(requireFail<Integer, Integer>(&Value::getInteger, &Value::getIntegerOrThrow, 0));
        WITH_CONTEXT(requireFail<bool, bool>(&Value::getBoolean, &Value::getBooleanOrThrow, false));
        WITH_CONTEXT(requireFail<Float, Float>(&Value::getFloat, &Value::getFloatOrThrow, 0.0));
        WITH_CONTEXT(
            requireFail<el::text::String, el::text::String>(
                &Value::getText, &Value::getTextOrThrow, el::text::String{}));
        WITH_CONTEXT(
            requireFail<el::time::TimeWithZone, const el::time::TimeWithZone &>(
                &Value::getTimeWithZone, &Value::getTimeWithZoneOrThrow, el::time::TimeWithZone{}));
        // WITH_CONTEXT(requireFail<el::time::Date, const el::time::Date&>(&Value::getDate, &Value::getDateOrThrow,
        // el::time::Date{}));
        WITH_CONTEXT(
            requireFail<el::time::DateTime, const el::time::DateTime &>(
                &Value::getDateTime, &Value::getDateTimeOrThrow, el::time::DateTime{}));
        WITH_CONTEXT(
            requireFail<el::mem::ByteBlock, const el::mem::ByteBlock &>(
                &Value::getBytes, &Value::getBytesOrThrow, el::mem::ByteBlock{}));
        WITH_CONTEXT(
            requireFail<el::time::CalendarDelta, const el::time::CalendarDelta &>(
                &Value::getCalendarDelta, &Value::getCalendarDeltaOrThrow, el::time::CalendarDelta{}));
        WITH_CONTEXT(
            requireFail<el::re::RegExPtr, const el::re::RegExPtr &>(
                &Value::getRegEx, &Value::getRegExOrThrow, el::re::RegExPtr{}));
        WITH_CONTEXT(requireGetValueListFail());
    }

    TESTED_TARGETS(getTime getTimeWithZone)
    void testGetTime() {
        WITH_CONTEXT(setupTemplate3("14:08:32"));
        const auto path = NamePath::fromText("main.value"_el);
        const auto expected = makeTime(14, 8, 32, 0);
        REQUIRE_EQUAL(doc->getTime(path), expected);
        REQUIRE_EQUAL(doc->getTimeOrThrow(path), expected);
        REQUIRE_EQUAL(doc->get<el::time::Time>(path), expected);
        REQUIRE_EQUAL(doc->getOrThrow<el::time::Time>(path), expected);
        const auto expectedWithZone = el::time::TimeWithZone{expected, el::time::TimeZone::local()};
        REQUIRE_EQUAL(doc->getTimeWithZone(path), expectedWithZone);
        REQUIRE_EQUAL(doc->getTimeWithZoneOrThrow(path), expectedWithZone);
        REQUIRE_EQUAL(doc->get<el::time::TimeWithZone>(path), expectedWithZone);
        REQUIRE_EQUAL(doc->getOrThrow<el::time::TimeWithZone>(path), expectedWithZone);
        WITH_CONTEXT(requireFail<Integer, Integer>(&Value::getInteger, &Value::getIntegerOrThrow, 0));
        WITH_CONTEXT(requireFail<bool, bool>(&Value::getBoolean, &Value::getBooleanOrThrow, false));
        WITH_CONTEXT(requireFail<Float, Float>(&Value::getFloat, &Value::getFloatOrThrow, 0.0));
        WITH_CONTEXT(
            requireFail<el::text::String, el::text::String>(
                &Value::getText, &Value::getTextOrThrow, el::text::String{}));
        WITH_CONTEXT(
            requireFail<el::time::Date, const el::time::Date &>(
                &Value::getDate, &Value::getDateOrThrow, el::time::Date{}));
        // WITH_CONTEXT(requireFail<el::time::TimeWithZone, const
        // el::time::TimeWithZone&>(&Value::getTimeWithZone, &Value::getTimeWithZoneOrThrow,
        // el::time::TimeWithZone{}));
        WITH_CONTEXT(
            requireFail<el::time::DateTime, const el::time::DateTime &>(
                &Value::getDateTime, &Value::getDateTimeOrThrow, el::time::DateTime{}));
        WITH_CONTEXT(
            requireFail<el::mem::ByteBlock, const el::mem::ByteBlock &>(
                &Value::getBytes, &Value::getBytesOrThrow, el::mem::ByteBlock{}));
        WITH_CONTEXT(
            requireFail<el::time::CalendarDelta, const el::time::CalendarDelta &>(
                &Value::getCalendarDelta, &Value::getCalendarDeltaOrThrow, el::time::CalendarDelta{}));
        WITH_CONTEXT(
            requireFail<el::re::RegExPtr, const el::re::RegExPtr &>(
                &Value::getRegEx, &Value::getRegExOrThrow, el::re::RegExPtr{}));
        WITH_CONTEXT(requireGetValueListFail());
    }

    TESTED_TARGETS(getDateTime)
    void testGetDateTime() {
        WITH_CONTEXT(setupTemplate3("2025-01-20 14:08:32"));
        const auto path = NamePath::fromText("main.value"_el);
        const auto expected = makeDateTime(2025, 1, 20, 14, 8, 32, 0);
        REQUIRE_EQUAL(doc->getDateTime(path, el::time::DateTime{}), expected);
        REQUIRE_EQUAL(doc->getDateTimeOrThrow(path), expected);
        REQUIRE_EQUAL(doc->get<el::time::DateTime>(path, el::time::DateTime{}), expected);
        REQUIRE_EQUAL(doc->getOrThrow<el::time::DateTime>(path), expected);
        WITH_CONTEXT(requireFail<Integer, Integer>(&Value::getInteger, &Value::getIntegerOrThrow, 0));
        WITH_CONTEXT(requireFail<bool, bool>(&Value::getBoolean, &Value::getBooleanOrThrow, false));
        WITH_CONTEXT(requireFail<Float, Float>(&Value::getFloat, &Value::getFloatOrThrow, 0.0));
        WITH_CONTEXT(
            requireFail<el::text::String, el::text::String>(
                &Value::getText, &Value::getTextOrThrow, el::text::String{}));
        WITH_CONTEXT(
            requireFail<el::time::Date, const el::time::Date &>(
                &Value::getDate, &Value::getDateOrThrow, el::time::Date{}));
        WITH_CONTEXT(
            requireFail<el::time::TimeWithZone, const el::time::TimeWithZone &>(
                &Value::getTimeWithZone, &Value::getTimeWithZoneOrThrow, el::time::TimeWithZone{}));
        // WITH_CONTEXT(requireFail<el::time::DateTime, const el::time::DateTime&>(&Value::getDateTime,
        // &Value::getDateTimeOrThrow, el::time::DateTime{}));
        WITH_CONTEXT(
            requireFail<el::mem::ByteBlock, const el::mem::ByteBlock &>(
                &Value::getBytes, &Value::getBytesOrThrow, el::mem::ByteBlock{}));
        WITH_CONTEXT(
            requireFail<el::time::CalendarDelta, const el::time::CalendarDelta &>(
                &Value::getCalendarDelta, &Value::getCalendarDeltaOrThrow, el::time::CalendarDelta{}));
        WITH_CONTEXT(
            requireFail<el::re::RegExPtr, const el::re::RegExPtr &>(
                &Value::getRegEx, &Value::getRegExOrThrow, el::re::RegExPtr{}));
        WITH_CONTEXT(requireGetValueListFail());
    }

    TESTED_TARGETS(getBytes)
    void testGetBytes() {
        WITH_CONTEXT(setupTemplate3("<01 02 03>"));
        const auto path = NamePath::fromText("main.value"_el);
        const auto expected = bytesFromHex("010203"_el);
        REQUIRE_EQUAL(doc->getBytes(path, el::mem::ByteBlock{}), expected);
        REQUIRE_EQUAL(doc->getBytesOrThrow(path), expected);
        REQUIRE_EQUAL(doc->get<el::mem::ByteBlock>(path, el::mem::ByteBlock{}), expected);
        REQUIRE_EQUAL(doc->getOrThrow<el::mem::ByteBlock>(path), expected);
        WITH_CONTEXT(requireFail<Integer, Integer>(&Value::getInteger, &Value::getIntegerOrThrow, 0));
        WITH_CONTEXT(requireFail<bool, bool>(&Value::getBoolean, &Value::getBooleanOrThrow, false));
        WITH_CONTEXT(requireFail<Float, Float>(&Value::getFloat, &Value::getFloatOrThrow, 0.0));
        WITH_CONTEXT(
            requireFail<el::text::String, el::text::String>(
                &Value::getText, &Value::getTextOrThrow, el::text::String{}));
        WITH_CONTEXT(
            requireFail<el::time::Date, const el::time::Date &>(
                &Value::getDate, &Value::getDateOrThrow, el::time::Date{}));
        WITH_CONTEXT(
            requireFail<el::time::TimeWithZone, const el::time::TimeWithZone &>(
                &Value::getTimeWithZone, &Value::getTimeWithZoneOrThrow, el::time::TimeWithZone{}));
        WITH_CONTEXT(
            requireFail<el::time::DateTime, const el::time::DateTime &>(
                &Value::getDateTime, &Value::getDateTimeOrThrow, el::time::DateTime{}));
        // WITH_CONTEXT(requireFail<el::mem::ByteBlock, const el::mem::ByteBlock&>(&Value::getBytes,
        // &Value::getBytesOrThrow, el::mem::ByteBlock{}));
        WITH_CONTEXT(
            requireFail<el::time::CalendarDelta, const el::time::CalendarDelta &>(
                &Value::getCalendarDelta, &Value::getCalendarDeltaOrThrow, el::time::CalendarDelta{}));
        WITH_CONTEXT(
            requireFail<el::re::RegExPtr, const el::re::RegExPtr &>(
                &Value::getRegEx, &Value::getRegExOrThrow, el::re::RegExPtr{}));
        WITH_CONTEXT(requireGetValueListFail());
    }

    TESTED_TARGETS(getCalendarDelta)
    void testGetTimeDelta() {
        WITH_CONTEXT(setupTemplate3("10 weeks"));
        const auto path = NamePath::fromText("main.value"_el);
        const auto expected = el::time::CalendarDelta{el::time::Weeks{10}};
        REQUIRE_EQUAL(doc->getCalendarDelta(path, el::time::CalendarDelta{}), expected);
        REQUIRE_EQUAL(doc->getCalendarDeltaOrThrow(path), expected);
        REQUIRE_EQUAL(doc->get<el::time::CalendarDelta>(path, el::time::CalendarDelta{}), expected);
        REQUIRE_EQUAL(doc->getOrThrow<el::time::CalendarDelta>(path), expected);
        WITH_CONTEXT(requireFail<Integer, Integer>(&Value::getInteger, &Value::getIntegerOrThrow, 0));
        WITH_CONTEXT(requireFail<bool, bool>(&Value::getBoolean, &Value::getBooleanOrThrow, false));
        WITH_CONTEXT(requireFail<Float, Float>(&Value::getFloat, &Value::getFloatOrThrow, 0.0));
        WITH_CONTEXT(
            requireFail<el::text::String, el::text::String>(
                &Value::getText, &Value::getTextOrThrow, el::text::String{}));
        WITH_CONTEXT(
            requireFail<el::time::Date, const el::time::Date &>(
                &Value::getDate, &Value::getDateOrThrow, el::time::Date{}));
        WITH_CONTEXT(
            requireFail<el::time::TimeWithZone, const el::time::TimeWithZone &>(
                &Value::getTimeWithZone, &Value::getTimeWithZoneOrThrow, el::time::TimeWithZone{}));
        WITH_CONTEXT(
            requireFail<el::time::DateTime, const el::time::DateTime &>(
                &Value::getDateTime, &Value::getDateTimeOrThrow, el::time::DateTime{}));
        WITH_CONTEXT(
            requireFail<el::mem::ByteBlock, const el::mem::ByteBlock &>(
                &Value::getBytes, &Value::getBytesOrThrow, el::mem::ByteBlock{}));
        // WITH_CONTEXT(requireFail<el::time::CalendarDelta, const el::time::CalendarDelta&>(&Value::getCalendarDelta,
        // &Value::getCalendarDeltaOrThrow, el::time::CalendarDelta{}));
        WITH_CONTEXT(
            requireFail<el::re::RegExPtr, const el::re::RegExPtr &>(
                &Value::getRegEx, &Value::getRegExOrThrow, el::re::RegExPtr{}));
        WITH_CONTEXT(requireGetValueListFail());
    }

    TESTED_TARGETS(getRegEx)
    void testGetRegEx() {
        WITH_CONTEXT(setupTemplate3("/regex/"));
        const auto path = NamePath::fromText("main.value"_el);
        const auto expected = el::re::RegEx::compile("regex"_el);
        REQUIRE_EQUAL(doc->getRegEx(path, el::re::RegExPtr{})->pattern(), expected->pattern());
        REQUIRE_EQUAL(doc->getRegExOrThrow(path)->pattern(), expected->pattern());
        REQUIRE_EQUAL(doc->get<el::re::RegExPtr>(path, el::re::RegExPtr{})->pattern(), expected->pattern());
        REQUIRE_EQUAL(doc->getOrThrow<el::re::RegExPtr>(path)->pattern(), expected->pattern());
        WITH_CONTEXT(requireFail<Integer, Integer>(&Value::getInteger, &Value::getIntegerOrThrow, 0));
        WITH_CONTEXT(requireFail<bool, bool>(&Value::getBoolean, &Value::getBooleanOrThrow, false));
        WITH_CONTEXT(requireFail<Float, Float>(&Value::getFloat, &Value::getFloatOrThrow, 0.0));
        WITH_CONTEXT(
            requireFail<el::text::String, el::text::String>(
                &Value::getText, &Value::getTextOrThrow, el::text::String{}));
        WITH_CONTEXT(
            requireFail<el::time::Date, const el::time::Date &>(
                &Value::getDate, &Value::getDateOrThrow, el::time::Date{}));
        WITH_CONTEXT(
            requireFail<el::time::TimeWithZone, const el::time::TimeWithZone &>(
                &Value::getTimeWithZone, &Value::getTimeWithZoneOrThrow, el::time::TimeWithZone{}));
        WITH_CONTEXT(
            requireFail<el::time::DateTime, const el::time::DateTime &>(
                &Value::getDateTime, &Value::getDateTimeOrThrow, el::time::DateTime{}));
        WITH_CONTEXT(
            requireFail<el::mem::ByteBlock, const el::mem::ByteBlock &>(
                &Value::getBytes, &Value::getBytesOrThrow, el::mem::ByteBlock{}));
        WITH_CONTEXT(
            requireFail<el::time::CalendarDelta, const el::time::CalendarDelta &>(
                &Value::getCalendarDelta, &Value::getCalendarDeltaOrThrow, el::time::CalendarDelta{}));
        // WITH_CONTEXT(requireFail<el::re::RegExPtr, const el::re::RegExPtr&>(&Value::getRegEx,
        // &Value::getRegExOrThrow, el::re::RegExPtr{}));
        WITH_CONTEXT(requireGetValueListFail());
    }

    TESTED_TARGETS(getValueList)
    void testGetValueList() {
        WITH_CONTEXT(setupTemplate3("1, 2, 3"));
        const auto path = NamePath::fromText("main.value"_el);
        const auto list = doc->getValueList(path);
        REQUIRE_EQUAL(list.size(), 3U);
        REQUIRE_EQUAL(list[0]->asInteger(), 1);
        REQUIRE_EQUAL(list[1]->asInteger(), 2);
        REQUIRE_EQUAL(list[2]->asInteger(), 3);
        REQUIRE_EQUAL(doc->getValueListOrThrow(path).size(), 3U);
    }

    TESTED_TARGETS(getSectionList)
    void testGetSectionList() {
        WITH_CONTEXT(setupTemplate1("1"));
        const auto path = NamePath::fromText("list"_el);
        const auto sec = doc->getSectionList(path);
        REQUIRE_EQUAL(sec->size(), 3U);
        REQUIRE_EQUAL(doc->getSectionListOrThrow(path)->size(), 3U);
    }

    TESTED_TARGETS(getSectionWithNames)
    void testGetSectionWithNames() {
        WITH_CONTEXT(setupTemplate1("1"));
        const auto path = NamePath::fromText("main"_el);
        const auto sec = doc->getSectionWithNames(path);
        REQUIRE_EQUAL(sec->size(), 11U);
        REQUIRE_EQUAL(doc->getSectionWithNamesOrThrow(path)->size(), 11U);
    }

    TESTED_TARGETS(getSectionWithTexts)
    void testGetSectionWithTexts() {
        WITH_CONTEXT(setupTemplate1("1"));
        const auto path = NamePath::fromText("main.text"_el);
        const auto sec = doc->getSectionWithTexts(path);
        REQUIRE_EQUAL(sec->size(), 3U);
        REQUIRE_EQUAL(doc->getSectionWithTextsOrThrow(path)->size(), 3U);

        const auto invalidPath = NamePath::fromText("main.value1"_el);
        ValuePtr invalidSec;
        REQUIRE_NOTHROW(invalidSec = doc->getSectionWithTexts(invalidPath));
        REQUIRE_EQUAL(invalidSec, nullptr);
    }
};
