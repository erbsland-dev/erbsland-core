// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WorkerInvokeTraits.hpp"
#include "WorkerResult.hpp"
#include "WorkerTask.hpp"

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace erbsland::util::impl {

/// One callable with independently retained result storage.
/// @tparam tFunction The movable callable type.
/// @tested{WorkerPoolTest WorkerPoolEventTest}
template <typename tFunction>
class WorkerSingleTask final : public WorkerTask {
public:
    using Result = decltype(invokeWorker(std::declval<tFunction &>(), StopToken{})); ///< Callable result.
    static_assert(!std::is_reference_v<Result>, "Worker results must be values or void.");

public:
    /// Retain the callable until its execution finishes.
    /// @param function The work callable.
    explicit WorkerSingleTask(tFunction function) : _function{std::move(function)} {}

public: // implement WorkerTask
    auto itemCount() const noexcept -> std::size_t override { return 1; }
    void execute(std::size_t, StopToken token) override {
        if constexpr (std::is_void_v<Result>) {
            invokeWorker(*_function, std::move(token));
        } else {
            result->value.emplace(invokeWorker(*_function, std::move(token)));
        }
    }
    void releaseWork() override { _function.reset(); }
    auto publish(std::exception_ptr error) -> event::EventCallback override { return result->publish(error); }

public:
    std::shared_ptr<WorkerResult<Result>> result{std::make_shared<WorkerResult<Result>>()}; ///< Result recipient.

private:
    std::optional<tFunction> _function; ///< Work captures, destroyed before result readiness.
};

}
