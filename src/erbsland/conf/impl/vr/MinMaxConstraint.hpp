// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"

#include <cstdint>

namespace erbsland::conf::impl {

using namespace text::literals;

class MinMaxConstraint : public Constraint {
public:
    enum MinOrMax : uint8_t { Min, Max };

public:
    explicit MinMaxConstraint(const MinOrMax minOrMax) {
        if (minOrMax == Min) {
            setType(vr::ConstraintType::Minimum);
        } else {
            setType(vr::ConstraintType::Maximum);
        }
    }

    template <typename T>
    [[nodiscard]] auto compare(const T &a, const T &b) const -> bool {
        if (type() == vr::ConstraintType::Minimum) {
            return a < b;
        }
        return a > b;
    }

    auto comparisonText() const -> const text::String & {
        static const text::String lessThan = "less than"_el;
        static const text::String atLeast = "at least"_el;
        static const text::String atMost = "at most"_el;
        static const text::String greaterThan = "greater than"_el;
        if (type() == vr::ConstraintType::Minimum) {
            return isNegated() ? lessThan : atLeast;
        }
        return isNegated() ? greaterThan : atMost;
    }
};

template <typename T>
class TypedMinMaxConstraint : public MinMaxConstraint {
public:
    explicit TypedMinMaxConstraint(const MinOrMax minOrMax, const T value) :
        MinMaxConstraint{minOrMax}, _value{value} {}

    [[nodiscard]] auto value() const -> const T & { return _value; }

protected:
    [[nodiscard]] auto isNotValid(const T validatedValue) const -> bool {
        if (isNegated()) {
            return !compare(validatedValue, _value);
        }
        return compare(validatedValue, _value);
    }

protected:
    T _value;
};

class MinMaxIntegerConstraint final : public TypedMinMaxConstraint<Integer> {
public:
    explicit MinMaxIntegerConstraint(const MinOrMax minOrMax, Integer value) : TypedMinMaxConstraint{minOrMax, value} {}

protected:
    void validateInteger(const ValidationContext &context, Integer value) const override;
    void validateText(const ValidationContext &context, const text::String &value) const override;
    void validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const override;
    void validateValueList(const ValidationContext &context) const override;
    void validateSectionList(const ValidationContext &context) const override;
    void validateSectionWithNames(const ValidationContext &context) const override;
    void validateSectionWithTexts(const ValidationContext &context) const override;
};

class MinMaxFloatConstraint final : public TypedMinMaxConstraint<Float> {
public:
    explicit MinMaxFloatConstraint(const MinOrMax minOrMax, Float value) : TypedMinMaxConstraint{minOrMax, value} {}

protected:
    void validateFloat(const ValidationContext &context, Float value) const override;
};

class MinMaxMatrixConstraint final : public TypedMinMaxConstraint<Integer> {
public:
    explicit MinMaxMatrixConstraint(const MinOrMax minOrMax, Integer first, Integer second) :
        TypedMinMaxConstraint{minOrMax, first}, _second{second} {}

    [[nodiscard]] auto secondValue() const -> Integer { return _second; }

protected:
    [[nodiscard]] auto isSecondNotValid(Integer validatedValue) const -> bool;
    void validateValueList(const ValidationContext &context) const override;

private:
    Integer _second;
};

class MinMaxDateConstraint final : public TypedMinMaxConstraint<time::Date> {
public:
    explicit MinMaxDateConstraint(const MinOrMax minOrMax, const time::Date &date) :
        TypedMinMaxConstraint{minOrMax, date} {}

protected:
    void validateDate(const ValidationContext &context, const time::Date &value) const override;
    void validateDateTime(const ValidationContext &context, const time::DateTime &value) const override;

private:
    time::Date _date;
};

class MinMaxDateTimeConstraint final : public TypedMinMaxConstraint<time::DateTime> {
public:
    explicit MinMaxDateTimeConstraint(const MinOrMax minOrMax, const time::DateTime &dateTime) :
        TypedMinMaxConstraint{minOrMax, dateTime} {}

protected:
    void validateDate(const ValidationContext &context, const time::Date &value) const override;
    void validateDateTime(const ValidationContext &context, const time::DateTime &value) const override;

private:
    time::DateTime _dateTime;
};

auto handleMinimumConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;
auto handleMaximumConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
