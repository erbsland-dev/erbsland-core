// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CoTaskCancelled.hpp"
#include "CoWorkerService.hpp"
#include "CoWorkState.hpp"

#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace erbsland::util::impl {

/// Awaiter that executes one bounded callable on the coroutine worker service.
/// @tparam tResult The callable result type.
/// @tparam tFunction The callable type.
/// @tested{CoTaskTest}
template <typename tResult, typename tFunction>
class CoWorkAwaiter final {
public:
    /// Create an awaiter for one callable.
    /// @param function The callable to execute asynchronously.
    explicit CoWorkAwaiter(tFunction function) :
        _state{std::make_shared<CoWorkState<tResult, tFunction>>(std::move(function))} {}

public:
    /// Indicate that work must always be submitted before the coroutine continues.
    /// @return Always `false`.
    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
    /// Submit the callable and suspend its continuation.
    /// @tparam tPromise The awaiting coroutine promise type.
    /// @param continuation The coroutine to resume after the callable completes.
    template <typename tPromise>
    void await_suspend(const std::coroutine_handle<tPromise> continuation) {
        _state->continuation = continuation;
        if constexpr (requires(tPromise &promise) { promise.cancellationState(); }) {
            _state->cancellation = continuation.promise().cancellationState();
        }
        auto state = _state;
        CoWorkerService::submit([state = std::move(state)]() mutable -> void {
            try {
                const auto cancellation = state->cancellation.lock();
                if (!cancellation || !cancellation->isCancelled()) {
                    auto function = std::move(*state->function);
                    state->function.reset();
                    if constexpr (std::is_void_v<tResult>) {
                        std::invoke(std::move(function));
                    } else {
                        state->result.emplace(std::invoke(std::move(function)));
                    }
                }
            } catch (...) {
                state->exception = std::current_exception();
            }
            state->continuation.resume();
        });
    }
    /// Return the callable result after resumption.
    /// @return The callable result, or nothing when `tResult` is `void`.
    /// @throws CoTaskCancelled If the owning task was cancelled.
    /// @throws Any exception thrown by the callable.
    auto await_resume() -> tResult {
        const auto cancellation = _state->cancellation.lock();
        if (cancellation && cancellation->isCancelled()) {
            throw CoTaskCancelled{};
        }
        if (_state->exception) {
            std::rethrow_exception(_state->exception);
        }
        if constexpr (!std::is_void_v<tResult>) {
            return std::move(*_state->result);
        }
    }

private:
    std::shared_ptr<CoWorkState<tResult, tFunction>> _state;
};

}
