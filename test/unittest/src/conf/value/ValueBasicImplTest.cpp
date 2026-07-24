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
    [[nodiscard]] virtual auto isDefault(impl::ValuePtr value) const -> bool = 0;
};

template <typename Type>
using ValueToMethod = Type (el::conf::Value::*)() const;
template <typename Type, ValueToMethod<Type> tMethod>
struct DoesToMethodReturnDefaultT : DoesToMethodReturnDefault {
    [[nodiscard]] auto isDefault(impl::ValuePtr value) const -> bool override {
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

    impl::ValuePtr value;

    void requireDefaults(ValueType valueType) {
        REQUIRE(value != nullptr);
        REQUIRE(value->hasParent() == false);
        REQUIRE(value->parent() == nullptr);
        REQUIRE(value->type() == valueType);
        REQUIRE(value->hasLocation() == false);
        REQUIRE(value->location().isUndefined());
        REQUIRE(value->size() == 0);
        REQUIRE(value->value(0U) == nullptr);
        // REQUIRE(value->value(el::text::String{"test"_el}) == nullptr);
        REQUIRE(value->begin() == value->end());
        for (auto const &[type, op] : cDoesReturnDefault) {
            if (type != valueType) {
                runWithContext(
                    SOURCE_LOCATION(),
                    [&]() { REQUIRE(op->isDefault(value) == true); },
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
        value = impl::Value::createInteger(70ll);
        WITH_CONTEXT(requireDefaults(ValueType::Integer));
        REQUIRE(value->asInteger() == 70ll);
        REQUIRE(value->toTextRepresentation() == "70"_el);
        value = impl::Value::createInteger(0x1234'5678'abcd'ef01ll); // make sure 64bit are actually stored.
        REQUIRE(value->asInteger() == 0x1234'5678'abcd'ef01ll);
        REQUIRE(value->toTextRepresentation() == "1311768467750121217"_el)
        value = impl::Value::createBoolean(true);
        WITH_CONTEXT(requireDefaults(ValueType::Boolean));
        REQUIRE(value->asBoolean() == true);
        REQUIRE(value->toTextRepresentation() == "true"_el)
        value = impl::Value::createBoolean(false);
        REQUIRE(value->asBoolean() == false);
        REQUIRE(value->toTextRepresentation() == "false"_el)
        value = impl::Value::createFloat(29.18e+20);
        WITH_CONTEXT(requireDefaults(ValueType::Float));
        REQUIRE(std::abs(value->asFloat() - 29.18e+20) < 1e-10);
        value = impl::Value::createText("→ Text ←"_el);
        WITH_CONTEXT(requireDefaults(ValueType::Text));
        REQUIRE(value->asText() == "→ Text ←"_el);
        REQUIRE(value->toTextRepresentation() == "→ Text ←"_el);
        value = impl::Value::createDate(makeDate(2024, 8, 21));
        WITH_CONTEXT(requireDefaults(ValueType::Date));
        REQUIRE(value->asDate() == makeDate(2024, 8, 21));
        REQUIRE(value->toTextRepresentation() == "2024-08-21"_el);
        value = impl::Value::createTime(makeTime(23, 19, 27));
        WITH_CONTEXT(requireDefaults(ValueType::Time));
        REQUIRE(value->asTime() == makeTime(23, 19, 27));
        REQUIRE(value->asTimeWithZone().timeZone().isLocalTime());
        REQUIRE(value->toTextRepresentation() == "23:19:27"_el);
        const auto copiedTime = value->deepCopy();
        REQUIRE(copiedTime != value);
        REQUIRE(copiedTime->asTime() == makeTime(23, 19, 27));
        value = impl::Value::createTimeWithZone(makeTimeWithZone(23, 19, 27));
        WITH_CONTEXT(requireDefaults(ValueType::Time));
        REQUIRE(value->asTime() == makeTime(23, 19, 27));
        REQUIRE(value->asTimeWithZone() == makeTimeWithZone(23, 19, 27));
        REQUIRE(value->toTextRepresentation() == "23:19:27Z"_el);
        value = impl::Value::createDateTime(makeDateTime(2024, 8, 21, 23, 19, 27, 0, 0));
        WITH_CONTEXT(requireDefaults(ValueType::DateTime));
        REQUIRE(value->asDateTime() == makeDateTime(2024, 8, 21, 23, 19, 27, 0, 0));
        REQUIRE(value->toTextRepresentation() == "2024-08-21 23:19:27Z"_el);
        value = impl::Value::createBytes(bytesFromHex("0102ff00"_el));
        WITH_CONTEXT(requireDefaults(ValueType::Bytes));
        REQUIRE(value->asBytes() == bytesFromHex("0102ff00"_el));
        REQUIRE(value->toTextRepresentation() == "0102ff00"_el);
        value = impl::Value::createCalendarDelta(el::time::CalendarDelta{el::time::Hours{18}});
        WITH_CONTEXT(requireDefaults(ValueType::TimeDelta));
        REQUIRE(value->asCalendarDelta() == el::time::CalendarDelta{el::time::Hours{18}});
        REQUIRE(value->toTextRepresentation() == "18h"_el);
        const auto compiledRegEx = el::re::RegEx::compile("^\\d+$"_el);
        value = impl::Value::createRegEx(compiledRegEx);
        WITH_CONTEXT(requireDefaults(ValueType::RegEx));
        REQUIRE(value->asRegEx() != nullptr);
        REQUIRE(value->asRegEx()->pattern() == "^\\d+$"_el);
        REQUIRE(value->toTextRepresentation() == "^\\d+$"_el);
        const auto copiedRegExValue = value->deepCopy();
        REQUIRE(copiedRegExValue != value);
        REQUIRE(copiedRegExValue->asRegEx() == compiledRegEx);
        value = impl::Value::createValueList({});
        WITH_CONTEXT(requireDefaults(ValueType::ValueList));
        REQUIRE(value->asValueList().empty() == true);
        REQUIRE(value->toTextRepresentation().isEmpty());
        value = impl::Value::createSectionList();
        WITH_CONTEXT(requireDefaults(ValueType::SectionList));
        REQUIRE(value->toTextRepresentation().isEmpty());
        value = impl::Value::createIntermediateSection();
        WITH_CONTEXT(requireDefaults(ValueType::IntermediateSection));
        REQUIRE(value->toTextRepresentation().isEmpty());
        value = impl::Value::createSectionWithNames();
        WITH_CONTEXT(requireDefaults(ValueType::SectionWithNames));
        REQUIRE(value->toTextRepresentation().isEmpty());
        value = impl::Value::createSectionWithTexts();
        WITH_CONTEXT(requireDefaults(ValueType::SectionWithTexts));
        REQUIRE(value->toTextRepresentation().isEmpty());
    }

    void testLocation() {
        value = impl::Value::createInteger(1);
        REQUIRE(value->hasLocation() == false);
        auto sourceIdentifier = SourceIdentifier::createForFile("main.elcl"_el);
        value->setLocation(
            Location(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{9U}, el::unit::ColumnIndex{4U}}));
        REQUIRE(value->hasLocation() == true);
        REQUIRE(
            value->location() ==
            Location(sourceIdentifier, el::unit::CodeLocation{el::unit::LineIndex{9U}, el::unit::ColumnIndex{4U}}));
        auto sourceIdentifier2 = SourceIdentifier::createForFile("another.elcl"_el);
        value->setLocation(
            Location(sourceIdentifier2, el::unit::CodeLocation{el::unit::LineIndex{6U}, el::unit::ColumnIndex{8U}}));
        REQUIRE(value->hasLocation() == true);
        REQUIRE(
            value->location() ==
            Location(sourceIdentifier2, el::unit::CodeLocation{el::unit::LineIndex{6U}, el::unit::ColumnIndex{8U}}));
        value->setLocation({});
        REQUIRE(value->hasLocation() == false);
        REQUIRE(value->location() == Location());
    }
};
