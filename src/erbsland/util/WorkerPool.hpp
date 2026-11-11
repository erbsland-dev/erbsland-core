// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StopToken.hpp"
#include "WorkerPool_fwd.hpp"
#include "WorkerPoolEventEditor.hpp"
#include "WorkerPoolOptions.hpp"

#include "impl/UtilitiesBackend_fwd.hpp"
#include "impl/worker_pool/WorkerPoolState_fwd.hpp"
#include "impl/worker_pool/WorkerTask_fwd.hpp"

#include "../event/EventSource.hpp"

#include <future>
#include <memory>
#include <ranges>

namespace erbsland::util {

/// An independently owned worker pool with procedural results and optional owner-loop completion.
/// @seedoc{/reference/util/worker_pool}
/// @tested{WorkerPoolTest WorkerPoolBulkTest WorkerPoolEventTest WorkerPoolFatalTest}
class WorkerPool final : public event::EventSource {
    friend class impl::UtilitiesBackend;

public:
    /// Drain accepted work and join workers, suppressing callbacks that have not begun.
    ~WorkerPool() override;

public: // factories
    /// Create an open pool; no event loop is required.
    /// @param options Concurrency and operation limits, copied into the pool.
    /// @return A shared pool owner, which must not be finally released on one of its own workers.
    [[nodiscard]] static auto create(const WorkerPoolOptions &options = {}) -> WorkerPoolPtr;

public: // accessors
    /// Get the effective fixed worker count.
    [[nodiscard]] auto workerCount() const noexcept -> unsigned int;
    /// Test whether new procedural operations can be admitted, independently of capacity.
    [[nodiscard]] auto isAccepting() const noexcept -> bool;
    /// Get the shared cooperative stop token.
    [[nodiscard]] auto stopToken() const noexcept -> StopToken;

public: // lifecycle
    /// Wait for work and its captures without closing admission or dispatching callbacks.
    /// Must not be called by one of this pool's workers.
    void wait();
    /// Close admission and drain accepted operations without waiting.
    void quit() noexcept;
    /// Close admission, cancel unfinished operations, and signal running callables without waiting.
    void stop() noexcept;
    /// Join workers without initiating shutdown or dispatching callbacks.
    /// Must not be called by one of this pool's workers.
    void join();

public: // work
    /// Submit one callable, optionally accepting a StopToken, and return its standard future.
    /// @tparam tFunction The movable work type, returning a non-reference value or void.
    /// @param work The callable, retained until execution or cancellation.
    /// @return A future carrying the value, RuntimeError, or WorkerCancelledError.
    /// @throws WorkerPoolFullError If the configured admission limit is reached.
    /// @throws WorkerPoolClosedError If admission has closed.
    template <typename tFunction>
    [[nodiscard]] auto submit(tFunction work);
    /// Submit one callable and deliver its outcome on the current owner loop.
    /// @tparam tFunction The movable work type.
    /// @tparam tCompleted The completion callable type.
    /// @param work The callable, optionally accepting a StopToken.
    /// @param completed Successful recipient of the result by value, or no argument for void.
    /// @param failed Optional operational-error/cancellation recipient overriding pool handlers.
    /// @throws WorkerPoolFullError If the configured admission limit is reached.
    /// @throws WorkerPoolClosedError If admission or owner-loop delivery has closed.
    /// @throws EventQueueFullError If initial owner binding cannot reserve delivery capacity.
    template <typename tFunction, typename tCompleted>
    void submit(tFunction work, tCompleted completed, WorkerPoolEventEditor::ErrorFn failed = {});
    /// Transform a finite range in parallel, preserving input order in one aggregate future.
    /// @tparam tRange The finite readable range type with copyable values.
    /// @tparam tFunction The const-invocable work type producing copyable, non-reference values.
    /// @param values Values copied into owned storage before this call returns.
    /// @param work Work accepting a const item reference and optionally a StopToken.
    /// @return A future carrying List of mapped results, or the first failure/cancellation.
    /// @throws WorkerPoolFullError If the configured admission limit is reached.
    /// @throws WorkerPoolClosedError If admission has closed.
    template <std::ranges::input_range tRange, typename tFunction>
    [[nodiscard]] auto map(tRange &&values, tFunction work);
    /// Transform a finite range and deliver one ordered result list on the owner loop.
    /// @tparam tRange The finite readable range type with copyable values.
    /// @tparam tFunction The const-invocable work type producing copyable values.
    /// @tparam tCompleted The successful completion callable type.
    /// @param values Values copied into owned storage before this call returns.
    /// @param work Work accepting a const item reference and optionally a StopToken.
    /// @param completed Recipient of the ordered result list by value.
    /// @param failed Optional operation-specific error/cancellation recipient.
    /// @throws WorkerPoolFullError If the configured admission limit is reached.
    /// @throws WorkerPoolClosedError If admission or event delivery has closed.
    /// @throws EventQueueFullError If initial owner binding cannot reserve delivery capacity.
    template <std::ranges::input_range tRange, typename tFunction, typename tCompleted>
    void map(tRange &&values, tFunction work, tCompleted completed, WorkerPoolEventEditor::ErrorFn failed = {});
    /// Process a finite range in parallel and return one void future.
    /// @tparam tRange The finite readable range type with copyable values.
    /// @tparam tFunction The concurrently const-invocable void work type.
    /// @param values Values copied into owned storage; the caller's range is not modified.
    /// @param work Work accepting a const item reference and optionally a StopToken.
    /// @return A future completed after all running items release their captures.
    /// @throws WorkerPoolFullError If the configured admission limit is reached.
    /// @throws WorkerPoolClosedError If admission has closed.
    template <std::ranges::input_range tRange, typename tFunction>
    [[nodiscard]] auto forEach(tRange &&values, tFunction work);
    /// Process a finite range and deliver one completion on the owner loop.
    /// @tparam tRange The finite readable range type with copyable values.
    /// @tparam tFunction The concurrently const-invocable void work type.
    /// @tparam tCompleted The successful completion callable type.
    /// @param values Values copied into owned storage; the caller's range is not modified.
    /// @param work Work accepting a const item reference and optionally a StopToken.
    /// @param completed No-argument recipient of successful completion.
    /// @param failed Optional operation-specific error/cancellation recipient.
    /// @throws WorkerPoolFullError If the configured admission limit is reached.
    /// @throws WorkerPoolClosedError If admission or event delivery has closed.
    /// @throws EventQueueFullError If initial owner binding cannot reserve delivery capacity.
    template <std::ranges::input_range tRange, typename tFunction, typename tCompleted>
    void forEach(tRange &&values, tFunction work, tCompleted completed, WorkerPoolEventEditor::ErrorFn failed = {});

public: // implement event::EventSource
    [[nodiscard]] auto events() -> WorkerPoolEventEditor & override;

private:
    /// Create and start the independently synchronized worker state.
    /// @param options Concurrency and admission settings.
    explicit WorkerPool(const WorkerPoolOptions &options);
    /// Bind or verify the current owner loop before accepting callback work.
    void prepareEvents();
    /// Bind initial event delivery to the selected owner loop.
    /// @param owner The owner loop whose context has been verified by the caller.
    void bindEvents(const event::EventTargetPtr &owner);
    /// Admit a prepared operation to the scheduler.
    /// @param task The configured operation and result recipient.
    void enqueue(impl::WorkerTaskPtr task);

private:
    impl::WorkerPoolStatePtr _state;                ///< Independently synchronized computation and delivery.
    std::unique_ptr<WorkerPoolEventEditor> _editor; ///< Stable, lazily bound editor.
};

}

#include "WorkerPool_work.tpp"
