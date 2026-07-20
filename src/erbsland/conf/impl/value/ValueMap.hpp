// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../NamePath.hpp"
#include "../../ValueIterator.hpp"

#include <unordered_map>
#include <vector>

namespace erbsland::conf::impl {

/// A map of values.
class ValueMap {
public:
    using List = std::vector<ValuePtr>;
    using Map = std::unordered_map<Name, ValuePtr>;

public:
    /// Create a new value map, from a list of unnamed values.
    /// - Automatically builds the indexes and names the elements.
    /// @param valueList The value list to use to build the map.
    explicit ValueMap(List &&valueList);

    // defaults
    ValueMap() = default;

public:
    [[nodiscard]] auto empty() const noexcept -> bool;
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    [[nodiscard]] auto hasValue(const NamePathLike &namePath) const noexcept -> bool;
    [[nodiscard]] auto value(const NamePathLike &namePath) const -> ValuePtr;
    [[nodiscard]] auto valueOrThrow(const NamePathLike &namePath, const conf::Value &value) const -> ValuePtr;
    [[nodiscard]] auto begin() const noexcept -> ValueIterator;
    [[nodiscard]] auto end() const noexcept -> ValueIterator;

public:
    void setTextIndexesAllowed(const bool allow) { _textIndexesAllowed = allow; }
    void addValue(const ValuePtr &value);
    void removeDefaultValues();
    [[nodiscard]] auto valueList() const noexcept -> const List & { return _valueList; }
    [[nodiscard]] auto valueMap() const noexcept -> const Map & { return _valueMap; }
    void setParent(const conf::ValuePtr &parent);

public:
    [[nodiscard]] auto hasValueImpl(const NamePath &namePath) const noexcept -> bool;
    [[nodiscard]] auto hasValueImpl(const Name &name) const noexcept -> bool;
    [[nodiscard]] auto hasValueImpl(std::size_t index) const noexcept -> bool;
    [[nodiscard]] auto valueImpl(const NamePath &namePath) const -> ValuePtr;
    [[nodiscard]] auto valueImpl(const Name &name) const -> ValuePtr;
    [[nodiscard]] auto valueImpl(std::size_t index) const -> ValuePtr;

private:
    bool _textIndexesAllowed{false}; ///< Whether text indexes are allowed.
    List _valueList;                 ///< The list with the values in order of their definition.
    Map _valueMap;                   ///< A map with the names of the values.
};

}
