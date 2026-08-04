// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Value.hpp"

#include <functional>

namespace erbsland::conf::impl {

/// Walks a configuration value tree in declaration order without recursion.
/// Provides a non-recursive depth-first traversal (preorder) over a configuration value tree.
/// Children are visited in declaration order.
class ValueTreeWalker final {
public:
    /// A predicate to decide whether a node (and its subtree) should be visited.
    /// If the filter returns false for a node, the entire subtree rooted at that
    /// node will be skipped (neither the node nor its children will be visited).
    using Filter = std::function<bool(const conf::ValuePtr &)>;

    /// A callback invoked for every visited node (in preorder).
    using Visit = std::function<void(const conf::ValuePtr &)>;

public:
    // defaults
    ValueTreeWalker() = default;

    /// Set the root node to traverse. Accepts any Value (Document derives from Value).
    void setRoot(const conf::ValuePtr &root) { _root = root; }

    /// Set an optional filter. If empty, all nodes are visited.
    void setFilter(const Filter &filter) { _filter = filter; }

    /// Traverse from the configured root and invoke the provided visit callback.
    /// If no root is set or it is null, nothing happens.
    void walk(const Visit &visit) const;

private:
    conf::ValuePtr _root;
    Filter _filter;
};

}
