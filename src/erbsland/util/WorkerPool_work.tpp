// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/worker_pool/WorkerBulkTask.hpp"
#include "impl/worker_pool/WorkerSingleTask.hpp"

#include <utility>

namespace erbsland::util {

template <typename tFunction>
auto WorkerPool::submit(tFunction work) {
    auto task = std::make_shared<impl::WorkerSingleTask<tFunction>>(std::move(work));
    auto future = task->result->future();
    enqueue(std::move(task));
    return future;
}

template <typename tFunction, typename tCompleted>
void WorkerPool::submit(tFunction work, tCompleted completed, WorkerPoolEventEditor::ErrorFn failed) {
    prepareEvents();
    auto task = std::make_shared<impl::WorkerSingleTask<tFunction>>(std::move(work));
    task->result->setCompleted(std::move(completed));
    task->eventDelivery = true;
    task->failed = std::move(failed);
    enqueue(std::move(task));
}

template <std::ranges::input_range tRange, typename tFunction>
auto WorkerPool::map(tRange &&values, tFunction work) {
    using Task = impl::WorkerBulkTask<std::ranges::range_value_t<tRange>, tFunction, true>;
    auto task = std::make_shared<Task>(std::forward<tRange>(values), std::move(work));
    auto future = task->result->future();
    enqueue(std::move(task));
    return future;
}

template <std::ranges::input_range tRange, typename tFunction, typename tCompleted>
void WorkerPool::map(tRange &&values, tFunction work, tCompleted completed, WorkerPoolEventEditor::ErrorFn failed) {
    prepareEvents();
    using Task = impl::WorkerBulkTask<std::ranges::range_value_t<tRange>, tFunction, true>;
    auto task = std::make_shared<Task>(std::forward<tRange>(values), std::move(work));
    task->result->setCompleted(std::move(completed));
    task->eventDelivery = true;
    task->failed = std::move(failed);
    enqueue(std::move(task));
}

template <std::ranges::input_range tRange, typename tFunction>
auto WorkerPool::forEach(tRange &&values, tFunction work) {
    using Task = impl::WorkerBulkTask<std::ranges::range_value_t<tRange>, tFunction, false>;
    auto task = std::make_shared<Task>(std::forward<tRange>(values), std::move(work));
    auto future = task->result->future();
    enqueue(std::move(task));
    return future;
}

template <std::ranges::input_range tRange, typename tFunction, typename tCompleted>
void WorkerPool::forEach(tRange &&values, tFunction work, tCompleted completed, WorkerPoolEventEditor::ErrorFn failed) {
    prepareEvents();
    using Task = impl::WorkerBulkTask<std::ranges::range_value_t<tRange>, tFunction, false>;
    auto task = std::make_shared<Task>(std::forward<tRange>(values), std::move(work));
    task->result->setCompleted(std::move(completed));
    task->eventDelivery = true;
    task->failed = std::move(failed);
    enqueue(std::move(task));
}

}
