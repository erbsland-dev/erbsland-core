// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoTaskStateBase.hpp"

#include <coroutine>
#include <exception>
#include <memory>
#include <optional>
#include <utility>

namespace erbsland::util::impl {

/// Shared state for a callable running on the coroutine worker service.
/// @tparam tResult The callable result type.
/// @tparam tFunction The callable type.
/// @tested{CoTaskTest}
template <typename tResult, typename tFunction>
class CoWorkState final {
public:
    /// Create state for one callable.
    /// @param function The callable to retain until a worker executes it.
    explicit CoWorkState(tFunction function) : function{std::move(function)} {}

public:
    std::optional<tFunction> function;           ///< The callable retained until worker execution starts.
    std::optional<tResult> result;               ///< The result returned by the callable.
    std::exception_ptr exception;                ///< The exception thrown by the callable.
    std::coroutine_handle<> continuation;        ///< The coroutine awaiting worker completion.
    std::weak_ptr<CoTaskStateBase> cancellation; ///< Cancellation state of the owning coroutine task.
};

/// Shared state for a void callable running on the coroutine worker service.
/// @tparam tFunction The callable type.
/// @tested{CoTaskTest}
template <typename tFunction>
class CoWorkState<void, tFunction> final {
public:
    /// Create state for one callable.
    /// @param function The callable to retain until a worker executes it.
    explicit CoWorkState(tFunction function) : function{std::move(function)} {}

public:
    std::optional<tFunction> function;           ///< The callable retained until worker execution starts.
    std::exception_ptr exception;                ///< The exception thrown by the callable.
    std::coroutine_handle<> continuation;        ///< The coroutine awaiting worker completion.
    std::weak_ptr<CoTaskStateBase> cancellation; ///< Cancellation state of the owning coroutine task.
};

}
