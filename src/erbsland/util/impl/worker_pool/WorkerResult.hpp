// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../event/EventCallback.hpp"

#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <utility>

namespace erbsland::util::impl {

/// Single-consumer result storage independent of a callable's lifetime.
/// @tparam T The non-reference result type.
/// @tested{WorkerPoolTest WorkerPoolBulkTest WorkerPoolEventTest}
template <typename T>
class WorkerResult final : public std::enable_shared_from_this<WorkerResult<T>> {
public:
    std::optional<T> value; ///< Value produced by the operation.

public:                     // delivery
    /// Select procedural delivery and obtain its future.
    [[nodiscard]] auto future() -> std::future<T> { return _promise.emplace().get_future(); }
    /// Select event delivery.
    /// @tparam tFunction The completion callable type.
    /// @param callback The successful completion callback.
    template <typename tFunction>
    void setCompleted(tFunction callback) {
        _completed = [callback = std::make_shared<tFunction>(std::move(callback))](
                         T result) -> void { std::invoke(*callback, std::move(result)); };
    }
    /// Publish an outcome after work captures have been released.
    /// @param error The committed error, or null for success.
    /// @return A successful event invocation, or empty for future/error delivery.
    auto publish(std::exception_ptr error) -> event::EventCallback {
        if (_promise) {
            if (error) {
                _promise->set_exception(error);
            } else {
                _promise->set_value(std::move(*value));
            }
            value.reset();
            return {};
        }
        if (error) {
            return {};
        }
        return [self = this->shared_from_this()]() -> void { self->_completed(std::move(*self->value)); };
    }

private:
    std::optional<std::promise<T>> _promise; ///< Present only for procedural delivery.
    std::function<void(T)> _completed;       ///< Successful event recipient.
};

/// Single-consumer completion storage for a void operation.
/// @tested{WorkerPoolTest WorkerPoolBulkTest WorkerPoolEventTest}
template <>
class WorkerResult<void> final {
public: // delivery
    /// Select procedural delivery and obtain its future.
    [[nodiscard]] auto future() -> std::future<void>;
    /// Select event delivery.
    /// @tparam tFunction The completion callable type.
    /// @param callback The successful completion callback.
    template <typename tFunction>
    void setCompleted(tFunction callback) {
        _completed = [callback = std::make_shared<tFunction>(std::move(callback))]() -> void {
            std::invoke(*callback);
        };
    }
    /// Publish an outcome after work captures have been released.
    /// @param error The committed error, or null for success.
    /// @return A successful event invocation, or empty for future/error delivery.
    auto publish(std::exception_ptr error) -> event::EventCallback;

private:
    std::optional<std::promise<void>> _promise; ///< Present only for procedural delivery.
    event::EventCallback _completed;            ///< Successful event recipient.
};

}
