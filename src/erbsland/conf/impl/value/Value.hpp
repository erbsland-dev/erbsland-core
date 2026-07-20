// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Container.hpp"

#include "../lexer/Content.hpp"

#include <utility>
#include <vector>

namespace erbsland::conf::impl {

class Value;
using ValuePtr = std::shared_ptr<Value>;
using ConstValuePtr = std::shared_ptr<const Value>;
class Rule;
using RulePtr = std::shared_ptr<Rule>;

/// Internal implementation of the public `conf::Value` interface.
/// The parser creates instances of this class and its derived types while building the value tree for a
/// configuration document.
class Value : public conf::Value, public Container {
public:
    // defaults
    Value() = default;
    ~Value() override = default;

    // disable copy and assign.
    Value(const Value &) = delete;
    auto operator=(const Value &) -> Value & = delete;
    Value(Value &&) = delete;
    auto operator=(Value &&) -> Value & = delete;

public:
    [[nodiscard]] auto name() const noexcept -> Name override;
    [[nodiscard]] auto namePath() const noexcept -> NamePath override;
    [[nodiscard]] auto hasParent() const noexcept -> bool override;
    [[nodiscard]] auto parent() const noexcept -> conf::ValuePtr override;
    [[nodiscard]] auto size() const noexcept -> std::size_t override;
    [[nodiscard]] auto hasValue(const NamePathLike &) const noexcept -> bool override;
    [[nodiscard]] auto value(const NamePathLike &) const noexcept -> conf::ValuePtr override;
    [[nodiscard]] auto valueOrThrow(const NamePathLike &namePath) const -> conf::ValuePtr override;
    [[nodiscard]] auto begin() const noexcept -> ValueIterator override;
    [[nodiscard]] auto end() const noexcept -> ValueIterator override;
    [[nodiscard]] auto hasLocation() const noexcept -> bool override;
    [[nodiscard]] auto location() const noexcept -> Location override;
    void setLocation(const Location &newLocation) noexcept override;
    [[nodiscard]] auto wasValidated() const noexcept -> bool override;
    [[nodiscard]] auto validationRule() const noexcept -> vr::RulePtr override;
    [[nodiscard]] auto isDefaultValue() const noexcept -> bool override;

    // empty defaults
    [[nodiscard]] auto asInteger() const noexcept -> int64_t override;
    [[nodiscard]] auto asBoolean() const noexcept -> bool override;
    [[nodiscard]] auto asFloat() const noexcept -> double override;
    [[nodiscard]] auto asText() const noexcept -> text::String override;
    [[nodiscard]] auto asDate() const noexcept -> time::Date override;
    [[nodiscard]] auto asTime() const noexcept -> time::Time override;
    [[nodiscard]] auto asTimeWithZone() const noexcept -> time::TimeWithZone override;
    [[nodiscard]] auto asDateTime() const noexcept -> time::DateTime override;
    [[nodiscard]] auto asBytes() const noexcept -> mem::ByteBlock override;
    [[nodiscard]] auto asCalendarDelta() const noexcept -> time::CalendarDelta override;
    [[nodiscard]] auto asRegEx() const noexcept -> re::RegExPtr override;
    [[nodiscard]] auto asValueList() const noexcept -> conf::ValueList override;
    [[nodiscard]] auto asIntegerOrThrow() const -> int64_t override;
    [[nodiscard]] auto asBooleanOrThrow() const -> bool override;
    [[nodiscard]] auto asFloatOrThrow() const -> double override;
    [[nodiscard]] auto asTextOrThrow() const -> text::String override;
    [[nodiscard]] auto asDateOrThrow() const -> time::Date override;
    [[nodiscard]] auto asTimeOrThrow() const -> time::Time override;
    [[nodiscard]] auto asTimeWithZoneOrThrow() const -> time::TimeWithZone override;
    [[nodiscard]] auto asDateTimeOrThrow() const -> time::DateTime override;
    [[nodiscard]] auto asBytesOrThrow() const -> mem::ByteBlock override;
    [[nodiscard]] auto asCalendarDeltaOrThrow() const -> time::CalendarDelta override;
    [[nodiscard]] auto asRegExOrThrow() const -> re::RegExPtr override;
    [[nodiscard]] auto asValueListOrThrow() const -> ValueList override;
    [[nodiscard]] auto toTextRepresentation() const noexcept -> text::String override;

public: // modification
    /// Set the name for this value.
    template <typename Fwd>
    void setName(Fwd &&name) noexcept {
        _name = std::forward<Fwd>(name);
    }

    /// Set the validation rule for this value.
    void setValidationRule(RulePtr rule) noexcept { _rule = std::move(rule); }

    /// Mark this value as default value.
    void markAsDefaultValue() noexcept { _isDefaultValue = true; }

    /// Transform a value type into another.
    /// @param targetType The target type for the transformation.
    virtual void transform([[maybe_unused]] ValueType targetType) { throw err::LogicError("Conversion not possible."); }

    /// Create a deep copy of this value.
    /// This creates a deep copy of this value, without the original parent and without a name.
    /// It is primarily used to store scalar values (and value lists) from validation rules documents.
    /// The resulting value (value structure) equals the one generated using the `create...` factory methods.
    /// Implementations that do not support deep-copy must throw an internal error.
    /// @return A deep copy of the scalar value or value list.
    [[nodiscard]] virtual auto deepCopy() const -> ValuePtr = 0;

public: // factory methods
    /// @{
    /// Factory method to create a value.
    [[nodiscard]] static auto createInteger(Integer value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createBoolean(bool value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createFloat(Float value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createText(text::String value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createDate(const time::Date &value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createTime(const time::Time &value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createTimeWithZone(const time::TimeWithZone &value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createDateTime(const time::DateTime &value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createBytes(const mem::ByteBlock &value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createBytes(mem::ByteBlock &&value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createCalendarDelta(const time::CalendarDelta &value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createRegEx(const re::RegExPtr &value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createRegEx(re::RegExPtr &&value) noexcept -> ValuePtr;
    [[nodiscard]] static auto createValueList(std::vector<ValuePtr> &&valueList) noexcept -> ValuePtr;
    [[nodiscard]] static auto createSectionList() noexcept -> ValuePtr;
    [[nodiscard]] static auto createIntermediateSection() noexcept -> ValuePtr;
    [[nodiscard]] static auto createSectionWithNames() noexcept -> ValuePtr;
    [[nodiscard]] static auto createSectionWithTexts() noexcept -> ValuePtr;
    /// @}

public: // implement `Container`
    void setParent(const conf::ValuePtr &parent) override;
    void addValue(const ValuePtr &childValue) override;

public: // helper methods.
    /// Fast access to all child-values.
    [[nodiscard]] virtual auto childrenImpl() const noexcept -> const std::vector<ValuePtr> &;
    /// Fast name-based access for child-values.
    [[nodiscard]] virtual auto valueImpl([[maybe_unused]] const Name &name) const noexcept -> ValuePtr;
    /// Remove default values from direct children.
    virtual void removeDefaultValues() {}

    [[noreturn]] static void throwAsTypeMismatch(const conf::Value &thisValue, ValueType expectedType);

    template <typename MessageFwd>
    [[noreturn]] static void throwErrorWithPath(
        const ConfErrorCategory errorCategory,
        MessageFwd &&message,
        const conf::Value &thisValue,
        const NamePathLike &namePath) {

        auto path = thisValue.namePath();
        path.append(toNamePath(namePath));
        throw ConfError(errorCategory, std::forward<MessageFwd>(message), std::move(path));
    }

    [[noreturn]] static void throwValueNotFound(const conf::Value &thisValue, const NamePathLike &namePath);

    [[noreturn]] static void throwTypeMismatch(
        const conf::Value &thisValue, ValueType expectedType, ValueType actualType, const NamePathLike &namePath);

    template <ValueType::Enum tValueType>
    [[nodiscard]] static auto getterOrThrow(const conf::Value &thisValue, const NamePathLike &namePath)
        -> conf::ValuePtr {

        const auto valuePtr = thisValue.value(namePath);
        if (valuePtr == nullptr) {
            throwValueNotFound(thisValue, namePath);
        }
        if (valuePtr->type() != tValueType) {
            throwTypeMismatch(thisValue, tValueType, valuePtr->type(), namePath);
        }
        return valuePtr;
    }

    template <typename ReturnType, ValueType::Enum tValueType>
    [[nodiscard]] static auto valueGetterOrThrow(const conf::Value &thisValue, const NamePathLike &namePath)
        -> ReturnType {

        const auto valuePtr = getterOrThrow<tValueType>(thisValue, namePath);
        return valuePtr->template asType<ReturnType>();
    }

    template <ValueType::Enum tValueType>
    [[nodiscard]] static auto sectionGetter(const conf::Value &thisValue, const NamePathLike &namePath) noexcept
        -> conf::ValuePtr {

        auto valuePtr = thisValue.value(namePath);
        if (valuePtr == nullptr) {
            return nullptr;
        }
        if (valuePtr->type() != tValueType) {
            return nullptr;
        }
        return valuePtr;
    }

    template <typename T>
    [[nodiscard]] static auto valueGetter(
        const conf::Value &thisValue, const NamePathLike &namePath, const T &defaultValue) noexcept -> T {

        const auto valuePtr = thisValue.value(namePath);
        if (valuePtr == nullptr) {
            return defaultValue;
        }
        if (valuePtr->type() != ValueType::from<T>()) {
            return defaultValue;
        }
        return valuePtr->asType<T>();
    }

    template <typename T, typename U>
    [[nodiscard]] static auto valueGetterWithDefaultToConvert(
        const conf::Value &thisValue, const NamePathLike &namePath, const U &defaultValue) noexcept -> T {

        const auto valuePtr = thisValue.value(namePath);
        if (valuePtr == nullptr) {
            return T{defaultValue};
        }
        if (valuePtr->type() != ValueType::from<T>()) {
            return T{defaultValue};
        }
        return valuePtr->asType<T>();
    }

protected:
    Name _name;                         ///< The name of the value.
    std::weak_ptr<conf::Value> _parent; ///< The parent value.
    Location _location;                 ///< The location of this value.
    RulePtr _rule;                      ///< The validation rule that was used when this value was validated.
    bool _isDefaultValue{false};        ///< Flag if this is a default value.
};

}
