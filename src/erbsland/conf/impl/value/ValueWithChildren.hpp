// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Value.hpp"
#include "ValueMap.hpp"

#include "../utilities/InternalError.hpp"

namespace erbsland::conf::impl {

/// A generic base class for value containers.
class ValueWithChildren : public Value {
public:
    /// Create a value with children using an existing list.
    /// @param children The existing list of values.
    explicit ValueWithChildren(ValueMap &&children) : _children(std::move(children)) {}

    // defaults
    ValueWithChildren() = default;

public: // implement `Value`
    [[nodiscard]] auto size() const noexcept -> std::size_t override { return _children.size(); }
    [[nodiscard]] auto hasValue(const NamePathLike &namePath) const noexcept -> bool override {
        return _children.hasValue(namePath);
    }
    [[nodiscard]] auto value(const NamePathLike &namePath) const noexcept -> conf::ValuePtr override {
        return _children.value(namePath);
    }
    [[nodiscard]] auto valueOrThrow(const NamePathLike &namePath) const -> conf::ValuePtr override {
        return _children.valueOrThrow(namePath, *this);
    }
    [[nodiscard]] auto begin() const noexcept -> ValueIterator override { return _children.begin(); }
    [[nodiscard]] auto end() const noexcept -> ValueIterator override { return _children.end(); }
    void addValue(const ValuePtr &childValue) override { _children.addValue(childValue); }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override {
        throwInternalError("Deep copy is not supported for this value type."_el);
    }

public: // implement impl::Value
    [[nodiscard]] auto childrenImpl() const noexcept -> const std::vector<ValuePtr> & override {
        return _children.valueList();
    }
    [[nodiscard]] auto valueImpl(const Name &name) const noexcept -> ValuePtr override {
        return _children.valueImpl(name);
    }
    void removeDefaultValues() override { _children.removeDefaultValues(); }

protected:
    ValueMap _children; ///< A map of child values.
};

}
