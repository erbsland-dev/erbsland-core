// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithChildren.hpp"

namespace erbsland::conf::impl {

/// Implementation of value for value lists.
class ValueList final : public ValueWithChildren {
public:
    /// Create a list value from child values.
    /// @param valueList The child values to move into the list.
    explicit ValueList(std::vector<ValuePtr> &&valueList) noexcept :
        ValueWithChildren(ValueMap{std::move(valueList)}) {}
    [[nodiscard]] auto type() const noexcept -> ValueType override { return ValueType::ValueList; }
    [[nodiscard]] auto asValueList() const noexcept -> conf::ValueList override {
        return {_children.begin(), _children.end()};
    }
    [[nodiscard]] auto asValueListOrThrow() const -> conf::ValueList override {
        return {_children.begin(), _children.end()};
    }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override {
        std::vector<ValuePtr> children;
        for (const auto &child : _children.valueList()) {
            children.push_back(child->deepCopy());
        }
        return std::make_shared<ValueList>(std::move(children));
    }
    /// Initialize the child values.
    void initializeChildren() { _children.setParent(shared_from_this()); }
};

}
