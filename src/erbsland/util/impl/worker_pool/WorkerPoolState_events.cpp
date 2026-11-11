// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WorkerPoolState.hpp"

#include "WorkerPoolWake.hpp"

#include "../../../event/EventLoop.hpp"
#include "../../../event/impl/EventContinuation.hpp"

#include <utility>

namespace erbsland::util::impl {

void WorkerPoolState::bind(const event::EventTargetPtr &owner, event::EventSourceWeakPtr source) {
    auto continuation = event::impl::EventContinuation::create(owner);
    const auto lock = std::scoped_lock{_mutex};
    _owner = std::dynamic_pointer_cast<event::EventLoop>(owner);
    _source = std::move(source);
    _continuation = std::move(continuation);
    _eventsEnabled = true;
    // Binding does not replay past idle or final transitions.
    _idleNotified = _working == 0;
    _finalNotified = _closed && _liveWorkers == 0;
}

void WorkerPoolState::kick() {
    auto continuation = event::impl::EventContinuationPtr{};
    {
        const auto lock = std::scoped_lock{_mutex};
        if (!_eventsEnabled || _eventsDiscarded || _wakePending) {
            return;
        }
        const auto idle = _pending == 0 && _hadWork && !_idleNotified;
        const auto final = _pending == 0 && _closed && _liveWorkers == 0 && !_finalNotified;
        if (_deliveries.empty() && !idle && !final) {
            return;
        }
        _wakePending = true;
        continuation = _continuation;
    }
    auto wake = std::make_shared<WorkerPoolWake>(weak_from_this());
    continuation->invoke([wake = std::move(wake)]() -> void { wake->invoke(); });
}

void WorkerPoolState::dispatch() {
    const auto source = _source.lock();
    const auto owner = _owner.lock();
    if (!source || !owner || owner->isQuitRequested()) {
        discardEvents();
        return;
    }
    auto delivery = WorkerDelivery{};
    auto callback = event::EventCallback{};
    auto onError = std::function<void(std::exception_ptr)>{};
    auto onCancelled = event::EventCallback{};
    {
        const auto lock = std::scoped_lock{_mutex};
        _wakePending = false;
        if (_eventsDiscarded) {
            return;
        }
        if (!_deliveries.empty()) {
            delivery = std::move(_deliveries.front());
            _deliveries.pop_front();
            --_pending;
            onError = _onError;
            onCancelled = _onCancelled;
        } else if (_pending == 0 && _hadWork && !_idleNotified) {
            _idleNotified = true;
            callback = _onIdle;
        } else if (_pending == 0 && _closed && _liveWorkers == 0 && !_finalNotified) {
            _finalNotified = true;
            callback = _onFinal;
        }
    }
    // Rearm before calling user code so a throwing handler cannot strand later outcomes.
    kick();
    if (delivery.error) {
        if (delivery.failed) {
            delivery.failed(delivery.error);
        } else if (delivery.cancelled) {
            if (onCancelled) {
                onCancelled();
            }
        } else if (onError) {
            onError(delivery.error);
        } else {
            std::rethrow_exception(delivery.error);
        }
    } else if (delivery.completed) {
        delivery.completed();
    } else if (callback) {
        callback();
    }
}

void WorkerPoolState::discardEvents() noexcept {
    auto discarded = std::deque<WorkerDelivery>{};
    auto onError = std::function<void(std::exception_ptr)>{};
    auto onCancelled = event::EventCallback{};
    auto onIdle = event::EventCallback{};
    auto onFinal = event::EventCallback{};
    {
        const auto lock = std::scoped_lock{_mutex};
        _eventsDiscarded = true;
        _wakePending = false;
        _pending -= _deliveries.size();
        discarded.swap(_deliveries);
        onError.swap(_onError);
        onCancelled.swap(_onCancelled);
        onIdle.swap(_onIdle);
        onFinal.swap(_onFinal);
    }
}

void WorkerPoolState::setError(std::function<void(std::exception_ptr)> callback) {
    const auto lock = std::scoped_lock{_mutex};
    _onError.swap(callback);
}

void WorkerPoolState::setCancelled(event::EventCallback callback) {
    const auto lock = std::scoped_lock{_mutex};
    _onCancelled.swap(callback);
}

void WorkerPoolState::setIdle(event::EventCallback callback) {
    const auto lock = std::scoped_lock{_mutex};
    _onIdle.swap(callback);
}

void WorkerPoolState::setFinal(event::EventCallback callback) {
    const auto lock = std::scoped_lock{_mutex};
    _onFinal.swap(callback);
}

}
