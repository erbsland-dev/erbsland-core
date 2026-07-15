// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoTaskStateBase.hpp"

#include <coroutine>
#include <exception>
#include <mutex>
#include <optional>

namespace erbsland::util::impl {

/// Shared completion state for a coroutine task result.
/// @tparam tValue The stored result type.
/// @tested{CoTaskTest}
template <typename tValue>
class CoTaskState final : public CoTaskStateBase {
public:
    mutable std::mutex mutex;             ///< Protects completion state and the continuation.
    bool complete{false};                 ///< Indicates that the coroutine reached final suspension.
    bool consumed{false};                 ///< Indicates that the move-only result was consumed.
    std::optional<tValue> value;          ///< The result returned by the coroutine.
    std::exception_ptr exception;         ///< The exception that escaped the coroutine.
    std::coroutine_handle<> continuation; ///< The single coroutine awaiting this task.
};

/// Shared completion state for a coroutine task without a result value.
/// @tested{CoTaskTest}
template <>
class CoTaskState<void> final : public CoTaskStateBase {
public:
    mutable std::mutex mutex;             ///< Protects completion state and the continuation.
    bool complete{false};                 ///< Indicates that the coroutine reached final suspension.
    std::exception_ptr exception;         ///< The exception that escaped the coroutine.
    std::coroutine_handle<> continuation; ///< The single coroutine awaiting this task.
};

}
