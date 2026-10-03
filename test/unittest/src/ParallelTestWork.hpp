// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <optional>
#include <thread>
#include <type_traits>
#include <vector>

/// Run independent test computations with bounded concurrency and ordered main-thread validation.
/// Workers must not use UnitTest assertions, contexts, output, or mutable suite state.
namespace parallel_test_work {

/// One computation's value or exception, consumed after all workers have joined.
/// @tparam T The computation's non-void result type.
template <typename T>
class Result final {
public:
    /// Evaluate one computation and capture its result or failure.
    /// @tparam tCompute The computation callable type.
    /// @param compute The shared computation.
    /// @param index The input index.
    template <typename tCompute>
    void evaluate(const tCompute &compute, const std::size_t index) {
        try {
            _value.emplace(std::invoke(compute, index));
        } catch (...) {
            _failure = std::current_exception();
        }
    }

public: // accessors
    /// Access the completed result in the calling test's assertion context.
    /// @return The computation's value.
    /// @throws std::exception Rethrows any exception captured from the computation.
    [[nodiscard]] auto value() const -> const T & {
        if (_failure) {
            std::rethrow_exception(_failure);
        }
        return _value.value();
    }

private:
    std::optional<T> _value;     ///< The successfully computed value.
    std::exception_ptr _failure; ///< An exception to rethrow in the main test context.
};

/// Compute indexed cases and join all workers before returning results in input order.
/// The callable is shared and must be safe to call concurrently; each case owns its mutable data.
/// @tparam tCompute The callable type, returning a non-void value for one index.
/// @param count The number of independent cases.
/// @param compute A computation that does not call the UnitTest framework.
/// @param maximumWorkers The concurrency cap, clamped to at least one and available hardware threads.
/// @return Ordered results containing values or captured exceptions.
/// @throws std::exception If allocating results or starting worker threads fails.
template <typename tCompute>
[[nodiscard]] auto run(std::size_t count, const tCompute &compute, std::size_t maximumWorkers = 4U)
    -> std::vector<Result<std::invoke_result_t<tCompute, std::size_t>>> {
    using Value = std::invoke_result_t<tCompute, std::size_t>;
    auto results = std::vector<Result<Value>>(count);
    auto next = std::atomic<std::size_t>{};
    const auto workerCount = std::min(
        {count,
            std::max(maximumWorkers, std::size_t{1U}),
            std::max(static_cast<std::size_t>(std::thread::hardware_concurrency()), std::size_t{1U})});
    auto workers = std::vector<std::jthread>{};
    workers.reserve(workerCount);
    for (auto worker = std::size_t{}; worker < workerCount; ++worker) {
        workers.emplace_back([&]() -> void {
            while (true) {
                const auto index = next.fetch_add(1U, std::memory_order_relaxed);
                if (index >= count) {
                    return;
                }
                results[index].evaluate(compute, index);
            }
        });
    }
    workers.clear();
    return results;
}

}
