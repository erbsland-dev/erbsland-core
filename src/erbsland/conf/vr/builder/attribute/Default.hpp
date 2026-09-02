// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../re/RegEx_fwd.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../../time/CalendarDelta.hpp"
#include "../../../../time/Date.hpp"
#include "../../../../time/DateTime.hpp"
#include "../../../../time/Time.hpp"
#include "../../../../time/TimeWithZone.hpp"
#include "../../../Float.hpp"
#include "../../../Integer.hpp"
#include "../../../Value_fwd.hpp"

#include <type_traits>
#include <vector>

namespace erbsland::conf::vr::builder {

/// Assigns a default value to a rule.
class Default : public Attribute {
public:
    /// Creates a default-value attribute from a configuration value.
    /// @param value The default configuration value.
    explicit Default(ValuePtr value);
    /// Creates an integer default-value attribute.
    explicit Default(Integer value);
    /// Creates a Boolean default-value attribute.
    explicit Default(bool value);
    /// Creates a floating-point default-value attribute.
    explicit Default(Float value);
    /// Creates a text default-value attribute.
    explicit Default(text::String value);
    /// Creates a date default-value attribute.
    explicit Default(const time::Date &value);
    /// Creates a time default-value attribute.
    explicit Default(const time::Time &value);
    /// Creates a zoned-time default-value attribute.
    explicit Default(const time::TimeWithZone &value);
    /// Creates a date-time default-value attribute.
    explicit Default(const time::DateTime &value);
    /// Creates a byte-block default-value attribute.
    explicit Default(const mem::ByteBlock &value);
    /// Creates a time-delta default-value attribute.
    explicit Default(const time::CalendarDelta &value);
    /// Creates a regular-expression default-value attribute.
    explicit Default(const re::RegExPtr &value);
    /// Creates an integer default-value attribute from a native integer.
    template <typename TValue>
        requires(std::is_integral_v<TValue> && !std::is_same_v<TValue, bool> && !std::is_same_v<TValue, Integer>)
    explicit Default(const TValue value) : Default(static_cast<Integer>(value)) {}
    /// Creates a floating-point default-value attribute from a native float.
    template <typename TValue>
        requires(std::is_floating_point_v<TValue> && !std::is_same_v<TValue, Float>)
    explicit Default(const TValue value) : Default(static_cast<Float>(value)) {}
    /// Creates a default-value attribute from integer values.
    /// @param values The default integer values.
    explicit Default(const std::vector<Integer> &values);
    /// Creates a default-value attribute from Boolean values.
    /// @param values The default Boolean values.
    explicit Default(const std::vector<bool> &values);
    /// Creates a default-value attribute from floating-point values.
    /// @param values The default floating-point values.
    explicit Default(const std::vector<Float> &values);
    /// Creates a default-value attribute from text values.
    /// @param values The default text values.
    explicit Default(const text::StringList &values);
    /// Creates a default-value attribute from byte-block values.
    /// @param values The default byte-block values.
    explicit Default(const std::vector<mem::ByteBlock> &values);
    /// Creates a default-value attribute from integer matrices.
    /// @param values The default integer matrices.
    explicit Default(const std::vector<std::vector<Integer>> &values);
    /// Creates a default-value attribute from floating-point matrices.
    /// @param values The default floating-point matrices.
    explicit Default(const std::vector<std::vector<Float>> &values);

    void apply(RuleDefinition &rule) const override;

public: // access
    /// Get the represented configuration value.
    [[nodiscard]] auto value() const noexcept -> const ValuePtr & { return _value; }

private:
    /// Convert a scalar range to a configuration value list.
    /// @tparam Range The range type that supplies scalar values.
    /// @param values The scalar values to convert.
    /// @return A value-list containing the converted scalar values.
    /// @tested{VrBuilderApiTest}
    template <typename Range>
    static auto createScalarListValue(const Range &values) -> ValuePtr;

    /// Convert a scalar matrix to a nested configuration value list.
    /// @tparam T The scalar matrix element type.
    /// @param values The scalar matrix to convert.
    /// @return A value-list containing one value-list for each matrix row.
    /// @tested{VrBuilderApiTest}
    template <typename T>
    static auto createScalarMatrixValue(const std::vector<std::vector<T>> &values) -> ValuePtr;

private:
    ValuePtr _value; ///< The represented default value.
};

}
