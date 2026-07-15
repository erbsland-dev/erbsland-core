// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoTaskAwaiterBase.hpp"
#include "CoTaskState.hpp"

#include "../../err/LogicError.hpp"

#include <exception>
#include <memory>
#include <mutex>
#include <utility>

namespace erbsland::util::impl {

/// Awaiter that consumes a completed coroutine task result.
/// @tparam tValue The task result type.
/// @tested{CoTaskTest}
template <typename tValue>
class CoTaskAwaiter final : public CoTaskAwaiterBase<CoTaskState<tValue>> {
    using Base = CoTaskAwaiterBase<CoTaskState<tValue>>;

public:
    /// Create an awaiter for shared task state.
    /// @param state The state to await and consume.
    explicit CoTaskAwaiter(std::shared_ptr<CoTaskState<tValue>> state) : Base{std::move(state)} {}

public:
    /// Consume the completed task result.
    /// @return The task result.
    /// @throws err::LogicError If the result was already consumed.
    /// @throws CoTaskCancelled If a parent task was cancelled.
    /// @throws Any exception produced by the task coroutine.
    auto await_resume() -> tValue {
        Base::beforeResume();
        const auto lock = std::scoped_lock{Base::_state->mutex};
        if (Base::_state->exception) {
            std::rethrow_exception(Base::_state->exception);
        }
        if (Base::_state->consumed) {
            throw err::LogicError{"The CoTask result was already consumed."};
        }
        Base::_state->consumed = true;
        return std::move(*Base::_state->value);
    }
};

/// Awaiter that observes completion of a coroutine task without a result value.
/// @tested{CoTaskTest}
template <>
class CoTaskAwaiter<void> final : public CoTaskAwaiterBase<CoTaskState<void>> {
    using Base = CoTaskAwaiterBase<CoTaskState<void>>;

public:
    /// Create an awaiter for shared task state.
    /// @param state The state to await.
    explicit CoTaskAwaiter(std::shared_ptr<CoTaskState<void>> state) : Base{std::move(state)} {}

public:
    /// Observe task completion and propagate failure.
    /// @throws CoTaskCancelled If a parent task was cancelled.
    /// @throws Any exception produced by the task coroutine.
    void await_resume() {
        Base::beforeResume();
        const auto lock = std::scoped_lock{Base::_state->mutex};
        if (Base::_state->exception) {
            std::rethrow_exception(Base::_state->exception);
        }
    }
};

}
