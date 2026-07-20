// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Value.hpp"

#include <memory>

namespace erbsland::conf::impl {

class Container;
using ContainerPtr = std::shared_ptr<Container>;
class Value;
using ValuePtr = std::shared_ptr<Value>;

/// The interface for container classes.
class Container {
public:
    virtual ~Container() = default;

public:
    /// Set the parent of this value.
    /// @param parent The parent value.
    virtual void setParent(const conf::ValuePtr &parent) = 0;

    /// Add a child value to this node.
    /// @param childValue The child value to add.
    virtual void addValue(const ValuePtr &childValue) = 0;
};

}
