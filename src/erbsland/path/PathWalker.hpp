// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Path_fwd.hpp"
#include "PathWalker_fwd.hpp"
#include "PathWalkFn.hpp"
#include "PathWalkOptions.hpp"
#include "PathWalkResult.hpp"

#include "impl/PathWalker_fwd.hpp"

namespace erbsland::path {

/// Walk a path tree in a deterministic order.
/// @tested{PathWalkerTest}
class PathWalker final {
public:
    /// Create an empty/invalid path walker.
    PathWalker();
    /// Create a path walker for the given path.
    explicit PathWalker(const Path &path);
    /// dtor
    ~PathWalker();

    // defaults
    PathWalker(const PathWalker &) = delete;
    PathWalker(PathWalker &&) noexcept;

    // defaults/deletions
    auto operator=(const PathWalker &) -> PathWalker & = delete;
    /// Move path-walker state into this instance.
    auto operator=(PathWalker &&) noexcept -> PathWalker &;

public: // attributes
    /// Test if the path is empty.
    [[nodiscard]] auto isEmpty() const -> bool;
    /// Access the underlying path.
    [[nodiscard]] auto path() const -> const Path &;

public:
    /// Walk all paths starting with this base path.
    /// The base path is included. `RootToLeaf` reports a directory before its children, while `LeafToRoot` reports it
    /// after its children. `Skip` prevents descent in root-to-leaf order; in leaf-to-root order descent has already
    /// happened and `Skip` is equivalent to `Continue`.
    /// @param walkFn The walk function.
    /// @param options Options for the walk operation.
    /// @return The walk operation result.
    auto walk(const PathWalkFn &walkFn, PathWalkOptions options = {}) const -> PathWalkResult;
    /// Walk all paths starting with this base path with additional path info for each path.
    /// This function may be faster, as file attributes can be pre-fetched.
    /// @param walkFn The walk function.
    /// @param options Options for the walk operation.
    /// @return The walk operation result.
    auto walk(const PathInfoWalkFn &walkFn, PathWalkOptions options = {}) const -> PathWalkResult;
    /// Walk all paths starting with this base path.
    /// The walk function returning the status `Error` is reported as an `Error` result, not throwing an exception.
    /// @param walkFn The walk function. Can throw an exception.
    /// @param options Options for the walk operation.
    /// @return The walk operation result.
    /// @throws PathError if an error occurs during the walk operation and `IgnoreErrors` is not set.
    auto walkOrThrow(const PathWalkFn &walkFn, PathWalkOptions options = {}) const -> PathWalkResult;
    /// Walk all paths starting with this base path with additional path info for each path.
    /// This function may be faster, as file attributes can be pre-fetched.
    /// The walk function returning the status `Error` is reported as an `Error` result, not throwing an exception.
    /// @param walkFn The walk function. Can throw an exception.
    /// @param options Options for the walk operation.
    /// @return The walk operation result.
    /// @throws PathError if an error occurs during the walk operation and `IgnoreErrors` is not set.
    auto walkOrThrow(const PathInfoWalkFn &walkFn, PathWalkOptions options = {}) const -> PathWalkResult;

private:
    impl::PathWalkerPtr _impl;
};

}
