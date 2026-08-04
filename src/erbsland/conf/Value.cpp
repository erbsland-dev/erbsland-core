// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Value.hpp"

#include "ConfError.hpp"

#include "impl/lexer/TokenType.hpp"
#include "impl/utilities/InternalError.hpp"
#include "impl/utilities/TestTextHelper.hpp"
#include "impl/value/Value.hpp"
#include "impl/value/ValueTreeHelper.hpp"
#include "vr/Rule.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::conf {

using namespace text::literals;

auto Value::toTestText(TestFormat format) const noexcept -> text::String {
    return impl::toTestText(*this, format);
}

auto Value::toTestValueTree(const TestFormat format) const noexcept -> text::String {
    auto thisValue = shared_from_this();
    auto lines = impl::ValueTreeHelper(thisValue, format).createLines();
    unit::ByteLength resultSize;
    for (const auto &line : lines) {
        resultSize += line.length();
        resultSize += unit::ByteLength::one();
    }
    text::StringEditor result;
    result.reserve(resultSize);
    for (const auto &line : lines) {
        result.append(line);
        result.append("\n"_el);
    }
    return result;
}

auto Value::isSecret() const noexcept -> bool {
    if (!wasValidated()) {
        return false;
    }
    return validationRule()->isSecret();
}

auto Value::toValueList() const noexcept -> ConstValueList {
    if (type() == ValueType::ValueList) {
        const auto valueList = asValueList();
        return ConstValueList{valueList.begin(), valueList.end()};
    }
    if (type().isScalar()) {
        return {shared_from_this()};
    }
    return {};
}

auto Value::toValueList() noexcept -> ValueList {
    if (type() == ValueType::ValueList) {
        return asValueList();
    }
    if (type().isScalar()) {
        return {shared_from_this()};
    }
    return {};
}

template <typename tValueMatrix, typename tValue>
auto Value::toValueMatrixImpl(tValue &value) noexcept -> tValueMatrix {
    if (value.type().isScalar()) {
        tValueMatrix matrix{unit::ItemCount::one(), unit::ItemCount::one()};
        matrix.setValue(unit::ItemIndex::zero(), unit::ItemIndex::zero(), value.shared_from_this());
        return matrix;
    }
    if (value.type() != ValueType::ValueList) {
        return {};
    }
    const auto valueList = value.asValueList();
    if (valueList.empty()) {
        return {};
    }
    const std::size_t maxColumns =
        std::ranges::max(valueList | std::views::transform([](const auto &listEntry) -> std::size_t {
            ERBSLAND_CORE_CONF_REQUIRE_SAFETY(listEntry != nullptr, "List entry cannot be null"_el);
            return (listEntry->type() == ValueType::ValueList) ? listEntry->size() : std::size_t{1};
        }));
    if (maxColumns == 0) {
        return {};
    }
    tValueMatrix matrix{unit::ItemCount::fromSizeT(valueList.size()), unit::ItemCount::fromSizeT(maxColumns)};
    for (std::size_t row = 0; row < valueList.size(); ++row) {
        const auto rowIndex = unit::ItemIndex::fromSizeT(row);
        const auto &listEntry = valueList[row];
        ERBSLAND_CORE_CONF_REQUIRE_SAFETY(listEntry != nullptr, "Matrix entry cannot be null"_el);
        if (listEntry->type() != ValueType::ValueList) {
            matrix.setValue(rowIndex, unit::ItemIndex::zero(), listEntry);
            continue;
        }
        const auto rowList = listEntry->asValueList();
        for (std::size_t column = 0; column < rowList.size(); ++column) {
            ERBSLAND_CORE_CONF_REQUIRE_SAFETY(rowList[column] != nullptr, "Matrix entry cannot be null"_el);
            matrix.setValue(
                rowIndex, unit::ItemIndex::fromSizeT(column), std::const_pointer_cast<Value>(rowList[column]));
        }
    }
    return matrix;
}

auto Value::toValueMatrix() const noexcept -> ConstValueMatrix {
    return toValueMatrixImpl<ConstValueMatrix, const Value>(*this);
}

auto Value::toValueMatrix() noexcept -> ValueMatrix {
    return toValueMatrixImpl<ValueMatrix, Value>(*this);
}

auto Value::empty() const noexcept -> bool {
    return size() == 0;
}

auto Value::firstValue() const noexcept -> ValuePtr {
    if (empty()) {
        return {};
    }
    return value(0U);
}

auto Value::lastValue() const noexcept -> ValuePtr {
    if (empty()) {
        return {};
    }
    return value(size() - 1U);
}

auto Value::getInteger(const NamePathLike &namePath, const Integer defaultValue) const noexcept -> Integer {
    return impl::Value::valueGetter<Integer>(*this, namePath, defaultValue);
}

auto Value::getBoolean(const NamePathLike &namePath, bool defaultValue) const noexcept -> bool {
    return impl::Value::valueGetter<bool>(*this, namePath, defaultValue);
}

auto Value::getFloat(const NamePathLike &namePath, Float defaultValue) const noexcept -> Float {
    return impl::Value::valueGetter<Float>(*this, namePath, defaultValue);
}

auto Value::getText(const NamePathLike &namePath, text::String defaultValue) const noexcept -> text::String {
    return impl::Value::valueGetter<text::String>(*this, namePath, std::move(defaultValue));
}

auto Value::getDate(const NamePathLike &namePath, const time::Date &defaultValue) const noexcept -> time::Date {
    return impl::Value::valueGetter<time::Date>(*this, namePath, defaultValue);
}

auto Value::getTime(const NamePathLike &namePath, const time::Time &defaultValue) const noexcept -> time::Time {
    return impl::Value::valueGetter<time::Time>(*this, namePath, defaultValue);
}

auto Value::getTimeWithZone(const NamePathLike &namePath, const time::TimeWithZone &defaultValue) const noexcept
    -> time::TimeWithZone {
    return impl::Value::valueGetter<time::TimeWithZone>(*this, namePath, defaultValue);
}

auto Value::getDateTime(const NamePathLike &namePath, const time::DateTime &defaultValue) const noexcept
    -> time::DateTime {
    return impl::Value::valueGetter<time::DateTime>(*this, namePath, defaultValue);
}

auto Value::getBytes(const NamePathLike &namePath, const mem::ByteBlock &defaultValue) const noexcept
    -> mem::ByteBlock {
    return impl::Value::valueGetter<mem::ByteBlock>(*this, namePath, defaultValue);
}

auto Value::getCalendarDelta(const NamePathLike &namePath, const time::CalendarDelta &defaultValue) const noexcept
    -> time::CalendarDelta {
    return impl::Value::valueGetter<time::CalendarDelta>(*this, namePath, defaultValue);
}

auto Value::getRegEx(const NamePathLike &namePath, const re::RegExPtr &defaultValue) const noexcept -> re::RegExPtr {
    return impl::Value::valueGetter<re::RegExPtr>(*this, namePath, defaultValue);
}

auto Value::getValueList(const NamePathLike &namePath) const noexcept -> ValueList {
    const auto valueAtPath = value(namePath);
    if (valueAtPath == nullptr) {
        return {};
    }
    return valueAtPath->asValueList();
}

auto Value::getIntegerOrThrow(const NamePathLike &namePath) const -> Integer {
    return impl::Value::valueGetterOrThrow<Integer, ValueType::Integer>(*this, namePath);
}

auto Value::getBooleanOrThrow(const NamePathLike &namePath) const -> bool {
    return impl::Value::valueGetterOrThrow<bool, ValueType::Boolean>(*this, namePath);
}

auto Value::getFloatOrThrow(const NamePathLike &namePath) const -> Float {
    return impl::Value::valueGetterOrThrow<Float, ValueType::Float>(*this, namePath);
}

auto Value::getTextOrThrow(const NamePathLike &namePath) const -> text::String {
    return impl::Value::valueGetterOrThrow<text::String, ValueType::Text>(*this, namePath);
}

auto Value::getDateOrThrow(const NamePathLike &namePath) const -> time::Date {
    return impl::Value::valueGetterOrThrow<time::Date, ValueType::Date>(*this, namePath);
}

auto Value::getTimeOrThrow(const NamePathLike &namePath) const -> time::Time {
    return impl::Value::valueGetterOrThrow<time::Time, ValueType::Time>(*this, namePath);
}

auto Value::getTimeWithZoneOrThrow(const NamePathLike &namePath) const -> time::TimeWithZone {
    return impl::Value::valueGetterOrThrow<time::TimeWithZone, ValueType::Time>(*this, namePath);
}

auto Value::getDateTimeOrThrow(const NamePathLike &namePath) const -> time::DateTime {
    return impl::Value::valueGetterOrThrow<time::DateTime, ValueType::DateTime>(*this, namePath);
}

auto Value::getBytesOrThrow(const NamePathLike &namePath) const -> mem::ByteBlock {
    return impl::Value::valueGetterOrThrow<mem::ByteBlock, ValueType::Bytes>(*this, namePath);
}

auto Value::getRegExOrThrow(const NamePathLike &namePath) const -> re::RegExPtr {
    return impl::Value::valueGetterOrThrow<re::RegExPtr, ValueType::RegEx>(*this, namePath);
}

auto Value::getCalendarDeltaOrThrow(const NamePathLike &namePath) const -> time::CalendarDelta {
    return impl::Value::valueGetterOrThrow<time::CalendarDelta, ValueType::TimeDelta>(*this, namePath);
}

auto Value::getValueListOrThrow(const NamePathLike &namePath) const -> ValueList {
    return impl::Value::valueGetterOrThrow<ValueList, ValueType::ValueList>(*this, namePath);
}

auto Value::getSectionWithNames(const NamePathLike &namePath) const noexcept -> ValuePtr {
    return impl::Value::sectionGetter<ValueType::SectionWithNames>(*this, namePath);
}

auto Value::getSectionList(const NamePathLike &namePath) const noexcept -> ValuePtr {
    return impl::Value::sectionGetter<ValueType::SectionList>(*this, namePath);
}

auto Value::getSectionWithNamesOrThrow(const NamePathLike &namePath) const -> ValuePtr {
    return impl::Value::getterOrThrow<ValueType::SectionWithNames>(*this, namePath);
}

auto Value::getSectionListOrThrow(const NamePathLike &namePath) const -> ValuePtr {
    return impl::Value::getterOrThrow<ValueType::SectionList>(*this, namePath);
}

auto Value::getSectionWithTexts(const NamePathLike &namePath) const noexcept -> ValuePtr {
    return impl::Value::sectionGetter<ValueType::SectionWithTexts>(*this, namePath);
}

auto Value::getSectionWithTextsOrThrow(const NamePathLike &namePath) const -> ValuePtr {
    return impl::Value::getterOrThrow<ValueType::SectionWithTexts>(*this, namePath);
}

}
