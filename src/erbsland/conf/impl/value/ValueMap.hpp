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
    /// Store values in definition order.
    using List = std::vector<ValuePtr>;
    /// Index values by name.
    using Map = std::unordered_map<Name, ValuePtr>;

public:
    /// Create a new value map, from a list of unnamed values.
    /// - Automatically builds the indexes and names the elements.
    /// @param valueList The value list to use to build the map.
    explicit ValueMap(List &&valueList);

    // defaults
    ValueMap() = default;

public:
    /// Test whether this map contains no values.
    [[nodiscard]] auto empty() const noexcept -> bool;
    /// Get the number of values.
    [[nodiscard]] auto size() const noexcept -> std::size_t;
    /// Test whether a value exists at `namePath`.
    [[nodiscard]] auto hasValue(const NamePathLike &namePath) const noexcept -> bool;
    /// Get the value at `namePath`, if any.
    [[nodiscard]] auto value(const NamePathLike &namePath) const -> ValuePtr;
    /// Get the value at `namePath` or throw using `value` as context.
    [[nodiscard]] auto valueOrThrow(const NamePathLike &namePath, const conf::Value &value) const -> ValuePtr;
    /// Get an iterator at the first value.
    [[nodiscard]] auto begin() const noexcept -> ValueIterator;
    /// Get the end iterator.
    [[nodiscard]] auto end() const noexcept -> ValueIterator;

public:
    /// Allow or reject text indexes in name paths.
    void setTextIndexesAllowed(const bool allow) { _textIndexesAllowed = allow; }
    /// Add one value to this map.
    void addValue(const ValuePtr &value);
    /// Remove values declared as defaults.
    void removeDefaultValues();
    /// Access the values in definition order.
    [[nodiscard]] auto valueList() const noexcept -> const List & { return _valueList; }
    /// Access the name-to-value index.
    [[nodiscard]] auto valueMap() const noexcept -> const Map & { return _valueMap; }
    /// Set the parent of all contained values.
    void setParent(const conf::ValuePtr &parent);

public:
    /// Test whether a nested path exists.
    [[nodiscard]] auto hasValueImpl(const NamePath &namePath) const noexcept -> bool;
    /// Test whether a name exists.
    [[nodiscard]] auto hasValueImpl(const Name &name) const noexcept -> bool;
    /// Test whether an index exists.
    [[nodiscard]] auto hasValueImpl(std::size_t index) const noexcept -> bool;
    /// Get the value at a nested path.
    [[nodiscard]] auto valueImpl(const NamePath &namePath) const -> ValuePtr;
    /// Get the value with a name.
    [[nodiscard]] auto valueImpl(const Name &name) const -> ValuePtr;
    /// Get the value at an index.
    [[nodiscard]] auto valueImpl(std::size_t index) const -> ValuePtr;

private:
    bool _textIndexesAllowed{false}; ///< Whether text indexes are allowed.
    List _valueList;                 ///< The list with the values in order of their definition.
    Map _valueMap;                   ///< A map with the names of the values.
};

}
