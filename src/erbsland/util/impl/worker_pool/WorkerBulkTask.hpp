// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WorkerInvokeTraits.hpp"
#include "WorkerResult.hpp"
#include "WorkerTask.hpp"

#include "../../List.hpp"

#include <concepts>
#include <memory>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

namespace erbsland::util::impl {

/// An owned input snapshot scheduled one item at a time without per-item futures.
/// @tparam tValue The copyable input value type.
/// @tparam tFunction The concurrently const-invocable work type.
/// @tparam tMap Whether to collect mapped values or execute void work.
/// @tested{WorkerPoolBulkTest WorkerPoolEventTest}
template <typename tValue, typename tFunction, bool tMap>
class WorkerBulkTask final : public WorkerTask {
private:
    using ItemResult =
        decltype(invokeWorker(std::declval<const tFunction &>(), StopToken{}, std::declval<const tValue &>()));
    using StoredResult = std::conditional_t<tMap, ItemResult, bool>;
    static_assert(std::copy_constructible<tValue>, "Bulk inputs must be copy constructible.");
    static_assert(
        !tMap ||
            (!std::is_void_v<ItemResult> && !std::is_reference_v<ItemResult> && std::copy_constructible<ItemResult>),
        "Mapped results must be copyable values.");
    static_assert(tMap || std::is_void_v<ItemResult>, "forEach work must return void.");

public:
    using Result = std::conditional_t<tMap, List<ItemResult>, void>; ///< Aggregate result type.

public:
    /// Snapshot a finite readable input range before accepting the operation.
    /// @tparam tRange The input range type.
    /// @param values Input values copied into owned storage.
    /// @param function The work callable.
    template <std::ranges::input_range tRange>
    WorkerBulkTask(tRange &&values, tFunction function) : _function{std::move(function)} {
        for (auto &&value : values) {
            _values.emplace_back(value);
        }
        if constexpr (tMap) {
            _results.resize(_values.size());
        }
    }

public: // implement WorkerTask
    auto itemCount() const noexcept -> std::size_t override { return _values.size(); }
    void execute(std::size_t index, StopToken token) override {
        if constexpr (tMap) {
            _results[index].emplace(
                invokeWorker(std::as_const(*_function), std::move(token), std::as_const(_values)[index]));
        } else {
            invokeWorker(std::as_const(*_function), std::move(token), std::as_const(_values)[index]);
        }
    }
    void releaseWork() override {
        _function.reset();
        _values.clear();
        if constexpr (tMap) {
            if (!error) {
                auto values = std::vector<ItemResult>{};
                values.reserve(_results.size());
                for (auto &value : _results) {
                    values.emplace_back(std::move(*value));
                }
                result->value.emplace(std::move(values));
            }
            _results.clear();
        }
    }
    auto publish(std::exception_ptr error) -> event::EventCallback override { return result->publish(error); }

public:
    std::shared_ptr<WorkerResult<Result>> result{std::make_shared<WorkerResult<Result>>()}; ///< Aggregate recipient.

private:
    std::optional<tFunction> _function;                ///< One const-invoked callable shared by the items.
    std::vector<tValue> _values;                       ///< Owned immutable input snapshot.
    std::vector<std::optional<StoredResult>> _results; ///< Independent output slots in input order.
};

}
