// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../text/StringList.hpp"
#include "../../../impl/value/Value.hpp"

#include <vector>

namespace erbsland::conf::vr::builder {

/// Assigns a default value to a rule.
class Default : public Attribute {
public:
    /// Creates a default-value attribute from a native value.
    /// @tparam tValue The native value type.
    /// @param value The default value.
    template <typename tValue>
    explicit Default(tValue value) : _value(impl::Value::createFromValue(value)) {}

    /// Creates a default-value attribute from a configuration value.
    /// @param value The default configuration value.
    explicit Default(impl::ValuePtr value);
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

    void operator()(impl::Rule &rule) override;

private:
    /// Convert a scalar range to a configuration value list.
    /// @tparam Range The range type that supplies scalar values.
    /// @param values The scalar values to convert.
    /// @return A value-list containing the converted scalar values.
    /// @tested{VrBuilderApiTest}
    template <typename Range>
    auto createScalarListValue(const Range &values) -> impl::ValuePtr;

    /// Convert a scalar matrix to a nested configuration value list.
    /// @tparam T The scalar matrix element type.
    /// @param values The scalar matrix to convert.
    /// @return A value-list containing one value-list for each matrix row.
    /// @tested{VrBuilderApiTest}
    template <typename T>
    auto createScalarMatrixValue(const std::vector<std::vector<T>> &values) -> impl::ValuePtr;

public:
    impl::ValuePtr _value;
};

}
