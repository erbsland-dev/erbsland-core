// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/Result.hpp"

namespace erbsland::path {

/// The result of a path walk call.
/// @tested{PathWalkerTest PosixPathWalkerTest}
class PathWalkResult final : public util::Result {
public:
    using Result::Result;

public:
    /// Successfully completed the walk.
    static const PathWalkResult Success;
    /// The user early stopped the walk (successfully).
    static const PathWalkResult Stopped;
    /// The walk was stopped because of a failure.
    static const PathWalkResult Failure;
};

inline constexpr PathWalkResult PathWalkResult::Success = Value::success<0>();
inline constexpr PathWalkResult PathWalkResult::Stopped = Value::success<1>();
inline constexpr PathWalkResult PathWalkResult::Failure = Value::failure<0>();

}
