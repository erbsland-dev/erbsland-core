// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/Result.hpp"

namespace erbsland::text {

/// The result of a text node walk call.
/// @tested{TextDocumentTest}
class TextWalkResult final : public util::Result {
public:
    using Result::Result;

public:
    /// Successfully completed the walk.
    static const TextWalkResult Success;
    /// The user early stopped the walk successfully.
    static const TextWalkResult Stopped;
    /// The walk was stopped because of a failure.
    static const TextWalkResult Failure;
};

inline constexpr TextWalkResult TextWalkResult::Success = Value::success<0>();
inline constexpr TextWalkResult TextWalkResult::Stopped = Value::success<1>();
inline constexpr TextWalkResult TextWalkResult::Failure = Value::failure<0>();

}
