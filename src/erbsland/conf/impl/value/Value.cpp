// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Value.hpp"

#include "BooleanValue.hpp"
#include "BytesValue.hpp"
#include "CalendarDeltaValue.hpp"
#include "DateTimeValue.hpp"
#include "DateValue.hpp"
#include "Document.hpp"
#include "FloatValue.hpp"
#include "IntegerValue.hpp"
#include "IntermediateSection.hpp"
#include "RegExValue.hpp"
#include "SectionList.hpp"
#include "SectionWithNames.hpp"
#include "SectionWithTexts.hpp"
#include "TextValue.hpp"
#include "TimeValue.hpp"
#include "TimeWithZoneValue.hpp"
#include "ValueList.hpp"

#include "../vr/Rule.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace erbsland::conf::impl {

using namespace text::literals;

auto Value::name() const noexcept -> Name {
    return _name;
}

auto Value::namePath() const noexcept -> NamePath {
    if (!hasParent()) { // A standalone value with no parent.
        return _name;
    }
    conf::ConstValueList tempValueList;
    tempValueList.reserve(10);
    tempValueList.emplace_back(shared_from_this());
    for (auto value = parent(); value != nullptr && !value->isRoot(); value = value->parent()) {
        tempValueList.emplace_back(value);
    }
    NameList nameList;
    nameList.reserve(tempValueList.size() + 1);
    for (const auto &value : std::views::reverse(tempValueList)) {
        nameList.emplace_back(value->name());
    }
    return NamePath{std::move(nameList)};
}

auto Value::hasParent() const noexcept -> bool {
    return !_parent.expired();
}

auto Value::parent() const noexcept -> conf::ValuePtr {
    return _parent.lock();
}

auto Value::size() const noexcept -> std::size_t {
    return 0;
}

auto Value::hasValue(const NamePathLike &) const noexcept -> bool {
    return false;
}

auto Value::value(const NamePathLike &) const noexcept -> conf::ValuePtr {
    return {};
}

auto Value::valueOrThrow(const NamePathLike &namePath) const -> conf::ValuePtr {
    throwValueNotFound(*this, namePath);
}

auto Value::begin() const noexcept -> ValueIterator {
    return {};
}

auto Value::end() const noexcept -> ValueIterator {
    return {};
}

auto Value::asInteger() const noexcept -> int64_t {
    return 0LL;
}

auto Value::asBoolean() const noexcept -> bool {
    return false;
}

auto Value::asFloat() const noexcept -> double {
    return 0.0;
}

auto Value::asText() const noexcept -> text::String {
    return {};
}

auto Value::asDate() const noexcept -> time::Date {
    return {};
}

auto Value::asTime() const noexcept -> time::Time {
    return {};
}

auto Value::asTimeWithZone() const noexcept -> time::TimeWithZone {
    return {};
}

auto Value::asDateTime() const noexcept -> time::DateTime {
    return {};
}

auto Value::asBytes() const noexcept -> mem::ByteBlock {
    return {};
}

auto Value::asCalendarDelta() const noexcept -> time::CalendarDelta {
    return {};
}

auto Value::asRegEx() const noexcept -> re::RegExPtr {
    return {};
}

auto Value::asValueList() const noexcept -> conf::ValueList {
    return {};
}

auto Value::asIntegerOrThrow() const -> int64_t {
    throwAsTypeMismatch(*this, ValueType::Integer);
}

auto Value::asBooleanOrThrow() const -> bool {
    throwAsTypeMismatch(*this, ValueType::Boolean);
}

auto Value::asFloatOrThrow() const -> double {
    throwAsTypeMismatch(*this, ValueType::Float);
}

auto Value::asTextOrThrow() const -> text::String {
    throwAsTypeMismatch(*this, ValueType::Text);
}

auto Value::asDateOrThrow() const -> time::Date {
    throwAsTypeMismatch(*this, ValueType::Date);
}

auto Value::asTimeOrThrow() const -> time::Time {
    throwAsTypeMismatch(*this, ValueType::Time);
}

auto Value::asTimeWithZoneOrThrow() const -> time::TimeWithZone {
    throwAsTypeMismatch(*this, ValueType::Time);
}

auto Value::asDateTimeOrThrow() const -> time::DateTime {
    throwAsTypeMismatch(*this, ValueType::DateTime);
}

auto Value::asBytesOrThrow() const -> mem::ByteBlock {
    throwAsTypeMismatch(*this, ValueType::Bytes);
}

auto Value::asCalendarDeltaOrThrow() const -> time::CalendarDelta {
    throwAsTypeMismatch(*this, ValueType::TimeDelta);
}

auto Value::asRegExOrThrow() const -> re::RegExPtr {
    throwAsTypeMismatch(*this, ValueType::RegEx);
}

auto Value::asValueListOrThrow() const -> conf::ValueList {
    throwAsTypeMismatch(*this, ValueType::ValueList);
}

auto Value::toTextRepresentation() const noexcept -> text::String {
    return {};
}

auto Value::hasLocation() const noexcept -> bool {
    return !_location.isUndefined();
}

auto Value::location() const noexcept -> Location {
    return _location;
}

void Value::setLocation(const Location &newLocation) noexcept {
    _location = newLocation;
}

auto Value::wasValidated() const noexcept -> bool {
    return _rule != nullptr;
}

auto Value::validationRule() const noexcept -> vr::RulePtr {
    return _rule;
}

auto Value::isDefaultValue() const noexcept -> bool {
    return _isDefaultValue;
}

void Value::setParent(const conf::ValuePtr &parent) {
    _parent = parent;
}

void Value::addValue(const ValuePtr &) {
    throw err::LogicError("Child values are not supported for this type."_el);
}

auto Value::childrenImpl() const noexcept -> const std::vector<ValuePtr> & {
    static std::vector<ValuePtr> empty;
    return empty;
}

auto Value::valueImpl([[maybe_unused]] const Name &name) const noexcept -> ValuePtr {
    return nullptr;
}

void Value::throwAsTypeMismatch(const conf::Value &thisValue, ValueType expectedType) {
    auto error = ConfError(
        ConfErrorCategory::TypeMismatch,
        text::StringFormat{"A value has not the required type. Expected '{}' but got '{}'."_el}.build(
            expectedType.toText(), thisValue.type().toText()),
        thisValue.location(),
        thisValue.namePath());
    if (thisValue.isSecret()) {
        throw error.withoutCodeSnippet();
    }
    throw error;
}

void Value::throwValueNotFound(const conf::Value &thisValue, const NamePathLike &namePath) {

    throwErrorWithPath(ConfErrorCategory::ValueNotFound, "A value was not found."_el, thisValue, namePath);
}

auto Value::createInteger(Integer value) noexcept -> ValuePtr {
    return std::make_shared<IntegerValue>(value);
}

auto Value::createBoolean(bool value) noexcept -> ValuePtr {
    return std::make_shared<BooleanValue>(value);
}

auto Value::createFloat(Float value) noexcept -> ValuePtr {
    return std::make_shared<FloatValue>(value);
}

auto Value::createText(text::String value) noexcept -> ValuePtr {
    return std::make_shared<TextValue>(std::move(value));
}

auto Value::createDate(const time::Date &value) noexcept -> ValuePtr {
    return std::make_shared<DateValue>(value);
}

auto Value::createTime(const time::Time &value) noexcept -> ValuePtr {
    return std::make_shared<TimeValue>(value);
}

auto Value::createTimeWithZone(const time::TimeWithZone &value) noexcept -> ValuePtr {
    return std::make_shared<TimeWithZoneValue>(value);
}

auto Value::createDateTime(const time::DateTime &value) noexcept -> ValuePtr {
    return std::make_shared<DateTimeValue>(value);
}

auto Value::createBytes(const mem::ByteBlock &value) noexcept -> ValuePtr {
    return std::make_shared<BytesValue>(value);
}

auto Value::createCalendarDelta(const time::CalendarDelta &value) noexcept -> ValuePtr {
    return std::make_shared<CalendarDeltaValue>(value);
}

auto Value::createRegEx(const re::RegExPtr &value) -> ValuePtr {
    if (value == nullptr) {
        throw err::ParameterError("The regular expression must not be a nullptr"_el, "value"_el);
    }
    return std::make_shared<RegExValue>(value);
}

auto Value::createValueList(std::vector<ValuePtr> &&valueList) noexcept -> ValuePtr {
    auto result = std::make_shared<ValueList>(std::move(valueList));
    result->initializeChildren();
    return result;
}

auto Value::createSectionList() noexcept -> ValuePtr {
    return std::make_shared<SectionList>();
}

auto Value::createIntermediateSection() noexcept -> ValuePtr {
    return std::make_shared<IntermediateSection>();
}

auto Value::createSectionWithNames() noexcept -> ValuePtr {
    return std::make_shared<SectionWithNames>();
}

auto Value::createSectionWithTexts() noexcept -> ValuePtr {
    return std::make_shared<SectionWithTexts>();
}

auto Value::createFromValue(Integer value) noexcept -> ValuePtr {
    return createInteger(value);
}

auto Value::createFromValue(bool value) noexcept -> ValuePtr {
    return createBoolean(value);
}

auto Value::createFromValue(Float value) noexcept -> ValuePtr {
    return createFloat(value);
}

auto Value::createFromValue(text::String value) noexcept -> ValuePtr {
    return createText(std::move(value));
}

auto Value::createFromValue(const time::Date &value) noexcept -> ValuePtr {
    return createDate(value);
}

auto Value::createFromValue(const time::Time &value) noexcept -> ValuePtr {
    return createTime(value);
}

auto Value::createFromValue(const time::TimeWithZone &value) noexcept -> ValuePtr {
    return createTimeWithZone(value);
}

auto Value::createFromValue(const time::DateTime &value) noexcept -> ValuePtr {
    return createDateTime(value);
}

auto Value::createFromValue(const mem::ByteBlock &value) noexcept -> ValuePtr {
    return createBytes(value);
}

auto Value::createFromValue(const time::CalendarDelta &value) noexcept -> ValuePtr {
    return createCalendarDelta(value);
}

auto Value::createFromValue(const re::RegExPtr &value) -> ValuePtr {
    return createRegEx(value);
}

}
