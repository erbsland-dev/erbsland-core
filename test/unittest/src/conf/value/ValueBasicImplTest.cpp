// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/value/Value.hpp>
#include <erbsland/conf/Location.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <vector>

using namespace el::conf;
using namespace el::text::literals;

struct DoesToMethodReturnDefault {
    virtual ~DoesToMethodReturnDefault() = default;
    [[nodiscard]] virtual auto isDefault(el::conf::impl::ValuePtr value) const -> bool = 0;
};

template <typename Type>
using ValueToMethod = Type (el::conf::Value::*)() const;
template <typename Type, ValueToMethod<Type> tMethod>
struct DoesToMethodReturnDefaultT : DoesToMethodReturnDefault {
    [[nodiscard]] auto isDefault(el::conf::impl::ValuePtr value) const -> bool override {
        return (value.get()->*tMethod)() == Type{};
    }
};

TESTED_TARGETS(Value ValueType)
class ValueBasicImplTest final : public el::UnitTest {
public:
    const std::vector<std::pair<ValueType, std::shared_ptr<DoesToMethodReturnDefault>>> cDoesReturnDefault = {
        std::make_pair(
            ValueType::Integer, std::make_shared<DoesToMethodReturnDefaultT<int64_t, &el::conf::Value::asInteger>>()),
        std::make_pair(
            ValueType::Boolean, std::make_shared<DoesToMethodReturnDefaultT<bool, &el::conf::Value::asBoolean>>()),
        std::make_pair(
            ValueType::Float, std::make_shared<DoesToMethodReturnDefaultT<double, &el::conf::Value::asFloat>>()),
        std::make_pair(
            ValueType::Text,
            std::make_shared<DoesToMethodReturnDefaultT<el::text::String, &el::conf::Value::asText>>()),
        std::make_pair(
            ValueType::Date, std::make_shared<DoesToMethodReturnDefaultT<el::time::Date, &el::conf::Value::asDate>>()),
        std::make_pair(
            ValueType::Time,
            std::make_shared<DoesToMethodReturnDefaultT<el::time::TimeWithZone, &el::conf::Value::asTimeWithZone>>()),
        std::make_pair(
            ValueType::DateTime,
            std::make_shared<DoesToMethodReturnDefaultT<el::time::DateTime, &el::conf::Value::asDateTime>>()),
        std::make_pair(
            ValueType::Bytes,
            std::make_shared<DoesToMethodReturnDefaultT<el::mem::ByteBlock, &el::conf::Value::asBytes>>()),
        std::make_pair(
            ValueType::TimeDelta,
            std::make_shared<DoesToMethodReturnDefaultT<el::time::CalendarDelta, &el::conf::Value::asCalendarDelta>>()),
        std::make_pair(
            ValueType::RegEx,
            std::make_shared<DoesToMethodReturnDefaultT<el::re::RegExPtr, &el::conf::Value::asRegEx>>()),
    };

    el::conf::impl::ValuePtr value;

    void requireDefaults(ValueType valueType) {
        REQUIRE(value);
        REQUIRE_EQUAL(value->hasParent(), false);
        REQUIRE_FALSE(value->parent());
        REQUIRE_EQUAL(value->type(), valueType);
        REQUIRE_EQUAL(value->hasLocation(), false);
        REQUIRE(value->location().isUndefined());
        REQUIRE_EQUAL(value->size(), 0);
        REQUIRE_FALSE(value->value(0U));
        // REQUIRE_FALSE(value->value(el::text::String{"test"_el}));
        REQUIRE_EQUAL(value->begin(), value->end());
        for (auto const &[type, op] : cDoesReturnDefault) {
            if (type != valueType) {
                runWithContext(
                    SOURCE_LOCATION(),
                    [&]() { REQUIRE_EQUAL(op->isDefault(value), true); },
                    [&]() {
                        return std::format(
                            "Tested type = {}, failed default type = {}",
                            el::text::StringConverter{valueType.toText()}.toStdString(),
                            el::text::StringConverter{type.toText()}.toStdString());
                    });
            }
        }
    }

    void testValueTypes() {
        value = el::conf::impl::Value::createInteger(70ll);
        WITH_CONTEXT(requireDefaults(ValueType::Integer));
        REQUIRE_EQUAL(value->asInteger(), 70ll);
        REQUIRE_EQUAL(value->toTextRepresentation(), "70"_el);
        value = el::conf::impl::Value::createInteger(0x1234'5678'abcd'ef01ll); // make sure 64bit are actually stored.
        REQUIRE_EQUAL(value->asInteger(), 0x1234'5678'abcd'ef01ll);
        REQUIRE_EQUAL(value->toTextRepresentation(), "1311768467750121217"_el);
        value = el::conf::impl::Value::createBoolean(true);
        WITH_CONTEXT(requireDefaults(ValueType::Boolean));
        REQUIRE_EQUAL(value->asBoolean(), true);
        REQUIRE_EQUAL(value->toTextRepresentation(), "true"_el);
        value = el::conf::impl::Value::createBoolean(false);
        REQUIRE_EQUAL(value->asBoolean(), false);
        REQUIRE_EQUAL(value->toTextRepresentation(), "false"_el);
        value = el::conf::impl::Value::createFloat(29.18e+20);
        WITH_CONTEXT(requireDefaults(ValueType::Float));
        REQUIRE_LESS(std::abs(value->asFloat() - 29.18e+20), 1e-10);
        value = el::conf::impl::Value::createText("→ Text ←"_el);
        WITH_CONTEXT(requireDefaults(ValueType::Text));
        REQUIRE_EQUAL(value->asText(), "→ Text ←"_el);
        REQUIRE_EQUAL(value->toTextRepresentation(), "→ Text ←"_el);
        value = el::conf::impl::Value::createDate(makeDate(2024, 8, 21));
        WITH_CONTEXT(requireDefaults(ValueType::Date));
        REQUIRE_EQUAL(value->asDate(), makeDate(2024, 8, 21));
        REQUIRE_EQUAL(value->toTextRepresentation(), "2024-08-21"_el);
        value = el::conf::impl::Value::createTime(makeTime(23, 19, 27));
        WITH_CONTEXT(requireDefaults(ValueType::Time));
        REQUIRE_EQUAL(value->asTime(), makeTime(23, 19, 27));
        REQUIRE(value->asTimeWithZone().timeZone().isLocalTime());
        REQUIRE_EQUAL(value->toTextRepresentation(), "23:19:27"_el);
        const auto copiedTime = value->deepCopy();
        REQUIRE_NOT_EQUAL(copiedTime, value);
        REQUIRE_EQUAL(copiedTime->asTime(), makeTime(23, 19, 27));
        value = el::conf::impl::Value::createTimeWithZone(makeTimeWithZone(23, 19, 27));
        WITH_CONTEXT(requireDefaults(ValueType::Time));
        REQUIRE_EQUAL(value->asTime(), makeTime(23, 19, 27));
        REQUIRE_EQUAL(value->asTimeWithZone(), makeTimeWithZone(23, 19, 27));
        REQUIRE_EQUAL(value->toTextRepresentation(), "23:19:27Z"_el);
        value = el::conf::impl::Value::createDateTime(makeDateTime(2024, 8, 21, 23, 19, 27, 0, 0));
        WITH_CONTEXT(requireDefaults(ValueType::DateTime));
        REQUIRE_EQUAL(value->asDateTime(), makeDateTime(2024, 8, 21, 23, 19, 27, 0, 0));
        REQUIRE_EQUAL(value->toTextRepresentation(), "2024-08-21 23:19:27Z"_el);
        value = el::conf::impl::Value::createBytes(bytesFromHex("0102ff00"_el));
        WITH_CONTEXT(requireDefaults(ValueType::Bytes));
        REQUIRE_EQUAL(value->asBytes(), bytesFromHex("0102ff00"_el));
        REQUIRE_EQUAL(value->toTextRepresentation(), "0102ff00"_el);
        value = el::conf::impl::Value::createCalendarDelta(el::time::CalendarDelta{el::time::Hours{18}});
        WITH_CONTEXT(requireDefaults(ValueType::TimeDelta));
        REQUIRE_EQUAL(value->asCalendarDelta(), el::time::CalendarDelta{el::time::Hours{18}});
        REQUIRE_EQUAL(value->toTextRepresentation(), "18h"_el);
        const auto compiledRegEx = el::re::RegEx::compile("^\\d+$"_el);
        value = el::conf::impl::Value::createRegEx(compiledRegEx);
        WITH_CONTEXT(requireDefaults(ValueType::RegEx));
        REQUIRE(value->asRegEx());
        REQUIRE_EQUAL(value->asRegEx()->pattern(), "^\\d+$"_el);
        REQUIRE_EQUAL(value->toTextRepresentation(), "^\\d+$"_el);
        const auto copiedRegExValue = value->deepCopy();
        REQUIRE_NOT_EQUAL(copiedRegExValue, value);
        REQUIRE_EQUAL(copiedRegExValue->asRegEx(), compiledRegEx);
        value = el::conf::impl::Value::createValueList({});
        WITH_CONTEXT(requireDefaults(ValueType::ValueList));
        REQUIRE_EQUAL(value->asValueList().empty(), true);
        REQUIRE(value->toTextRepresentation().isEmpty());
        value = el::conf::impl::Value::createSectionList();
        WITH_CONTEXT(requireDefaults(ValueType::SectionList));
        REQUIRE(value->toTextRepresentation().isEmpty());
        value = el::conf::impl::Value::createIntermediateSection();
        WITH_CONTEXT(requireDefaults(ValueType::IntermediateSection));
        REQUIRE(value->toTextRepresentation().isEmpty());
        value = el::conf::impl::Value::createSectionWithNames();
        WITH_CONTEXT(requireDefaults(ValueType::SectionWithNames));
        REQUIRE(value->toTextRepresentation().isEmpty());
        value = el::conf::impl::Value::createSectionWithTexts();
        WITH_CONTEXT(requireDefaults(ValueType::SectionWithTexts));
        REQUIRE(value->toTextRepresentation().isEmpty());
    }

    void testLocation() {
        value = el::conf::impl::Value::createInteger(1);
        REQUIRE_EQUAL(value->hasLocation(), false);
        auto sourceIdentifier = SourceIdentifier::createForFile("main.elcl"_el);
        value->setLocation(
            Location(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{9U}, el::unit::ColumnIndex{4U}}));
        REQUIRE_EQUAL(value->hasLocation(), true);
        REQUIRE(
            value->location() ==
            Location(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{9U}, el::unit::ColumnIndex{4U}}));
        auto sourceIdentifier2 = SourceIdentifier::createForFile("another.elcl"_el);
        value->setLocation(
            Location(sourceIdentifier2, el::unit::CodeLocation{el::unit::LineIndex{6U}, el::unit::ColumnIndex{8U}}));
        REQUIRE_EQUAL(value->hasLocation(), true);
        REQUIRE(
            value->location() ==
            Location(sourceIdentifier2, el::unit::CodeLocation{el::unit::LineIndex{6U}, el::unit::ColumnIndex{8U}}));
        value->setLocation({});
        REQUIRE_EQUAL(value->hasLocation(), false);
        REQUIRE_EQUAL(value->location(), Location());
    }
};
