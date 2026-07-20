// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Document.hpp"

#include "Section.hpp"
#include "Value.hpp"

#include "../vr/Rule.hpp"

#include <stack>

namespace erbsland::conf::impl {

auto Document::name() const noexcept -> Name {
    return {};
}

auto Document::namePath() const noexcept -> NamePath {
    return {};
}

auto Document::hasParent() const noexcept -> bool {
    return false;
}

auto Document::parent() const noexcept -> conf::ValuePtr {
    return {};
}

auto Document::type() const noexcept -> ValueType {
    return ValueType::Document;
}

auto Document::asInteger() const noexcept -> Integer {
    return {};
}

auto Document::asBoolean() const noexcept -> bool {
    return false;
}

auto Document::asFloat() const noexcept -> Float {
    return {};
}

auto Document::asText() const noexcept -> text::String {
    return {};
}

auto Document::asDate() const noexcept -> time::Date {
    return {};
}

auto Document::asTime() const noexcept -> time::Time {
    return {};
}

auto Document::asTimeWithZone() const noexcept -> time::TimeWithZone {
    return {};
}

auto Document::asDateTime() const noexcept -> time::DateTime {
    return {};
}

auto Document::asBytes() const noexcept -> mem::ByteBlock {
    return {};
}

auto Document::asCalendarDelta() const noexcept -> time::CalendarDelta {
    return {};
}

auto Document::asRegEx() const noexcept -> re::RegExPtr {
    return {};
}

auto Document::asValueList() const noexcept -> ValueList {
    return {};
}

auto Document::asIntegerOrThrow() const -> Integer {
    impl::Value::throwAsTypeMismatch(*this, ValueType::Integer);
}

auto Document::asBooleanOrThrow() const -> bool {
    impl::Value::throwAsTypeMismatch(*this, ValueType::Boolean);
}

auto Document::asFloatOrThrow() const -> Float {
    impl::Value::throwAsTypeMismatch(*this, ValueType::Float);
}

auto Document::asTextOrThrow() const -> text::String {
    impl::Value::throwAsTypeMismatch(*this, ValueType::Text);
}

auto Document::asDateOrThrow() const -> time::Date {
    impl::Value::throwAsTypeMismatch(*this, ValueType::Date);
}

auto Document::asTimeOrThrow() const -> time::Time {
    impl::Value::throwAsTypeMismatch(*this, ValueType::Time);
}

auto Document::asTimeWithZoneOrThrow() const -> time::TimeWithZone {
    impl::Value::throwAsTypeMismatch(*this, ValueType::Time);
}

auto Document::asDateTimeOrThrow() const -> time::DateTime {
    impl::Value::throwAsTypeMismatch(*this, ValueType::DateTime);
}

auto Document::asBytesOrThrow() const -> mem::ByteBlock {
    impl::Value::throwAsTypeMismatch(*this, ValueType::Bytes);
}

auto Document::asCalendarDeltaOrThrow() const -> time::CalendarDelta {
    impl::Value::throwAsTypeMismatch(*this, ValueType::TimeDelta);
}

auto Document::asRegExOrThrow() const -> re::RegExPtr {
    impl::Value::throwAsTypeMismatch(*this, ValueType::RegEx);
}

auto Document::asValueListOrThrow() const -> ValueList {
    impl::Value::throwAsTypeMismatch(*this, ValueType::ValueList);
}

auto Document::toTextRepresentation() const noexcept -> text::String {
    return {};
}

void Document::setValidationRule(RulePtr rule) noexcept {
    _rule = std::move(rule);
}

void Document::removeDefaultValues() {
    _children.removeDefaultValues();
}

auto Document::hasLocation() const noexcept -> bool {
    return !_location.isUndefined();
}

auto Document::location() const noexcept -> Location {
    return _location;
}

void Document::setLocation(const Location &newLocation) noexcept {
    _location = newLocation;
}

auto Document::wasValidated() const noexcept -> bool {
    return _rule != nullptr;
}

auto Document::validationRule() const noexcept -> vr::RulePtr {
    return _rule;
}

auto Document::isDefaultValue() const noexcept -> bool {
    return false;
}

auto Document::size() const noexcept -> std::size_t {
    return _children.size();
}

auto Document::hasValue(const NamePathLike &namePath) const noexcept -> bool {
    return _children.hasValue(namePath);
}

auto Document::value(const NamePathLike &namePath) const noexcept -> conf::ValuePtr {
    return _children.value(namePath);
}

auto Document::valueOrThrow(const NamePathLike &namePath) const -> conf::ValuePtr {
    return _children.valueOrThrow(namePath, *this);
}

auto Document::begin() const noexcept -> ValueIterator {
    return _children.begin();
}

auto Document::end() const noexcept -> ValueIterator {
    return _children.end();
}

auto Document::toFlatValueMap() const noexcept -> FlatValueMap {
    std::stack<conf::ConstValuePtr> stack;
    auto thisDocument = shared_from_this();
    stack.push(thisDocument);
    FlatValueMap result;
    while (!stack.empty()) {
        auto value = stack.top();
        stack.pop();
        if (!value->empty()) {
            for (const auto &childValue : *value) {
                stack.push(childValue);
            }
        }
        if (value != thisDocument) { // do not add the document root.
            result.emplace(value->namePath(), value);
        }
    }
    return result;
}

void Document::setParent(const conf::ValuePtr &) {
    throw err::LogicError("The document must not have a parent.");
}

void Document::addValue(const ValuePtr &childValue) {
    _children.addValue(childValue);
}

}
