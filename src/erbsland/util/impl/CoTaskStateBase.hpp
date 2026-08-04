// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>

namespace erbsland::util::impl {

/// Shared cancellation state for coroutine tasks.
/// @tested{CoTaskTest}
class CoTaskStateBase {
public:
    // defaults
    virtual ~CoTaskStateBase() = default;

public:
    /// Test if cancellation was requested.
    /// @return `true` if the owning task was cancelled or destroyed before completion.
    [[nodiscard]] auto isCancelled() const noexcept -> bool { return cancelled.load(); }

public:
    /// The atomic cancellation request flag.
    std::atomic<bool> cancelled{false};
};

}
