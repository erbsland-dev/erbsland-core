#include "EventContinuation.hpp"

#include "CallbackEventData.hpp"
#include "EventLoop.hpp"
#include "SchedulerBackend.hpp"

#include "../EventQueueFullError.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::event::impl {

using namespace text::literals;

EventContinuation::EventContinuation(std::weak_ptr<EventLoop> loop) : _loop{std::move(loop)} {
}

EventContinuation::~EventContinuation() {
    if (const auto loop = _loop.lock(); loop != nullptr) {
        const auto lock = std::scoped_lock{loop->_mutex};
        --loop->_continuationCount;
    }
}

auto EventContinuation::create(const EventTargetPtr &events) -> EventContinuationPtr {
    const auto loop = std::dynamic_pointer_cast<EventLoop>(events);
    if (loop == nullptr) {
        throw EventQueueFullError{"This event target does not support reserved continuations."_el};
    }
    const auto lock = std::scoped_lock{loop->_mutex};
    if (loop->_quitRequested || loop->_continuationCount >= 1024U) {
        throw EventQueueFullError{"The event loop cannot admit another reserved continuation."_el};
    }
    auto result = EventContinuationPtr{new EventContinuation{loop}};
    ++loop->_continuationCount;
    return result;
}

void EventContinuation::invoke(EventCallback callback) {
    queue(std::move(callback), State::Idle);
}

void EventContinuation::queue(EventCallback callback, State expected) {
    if (!_state.compare_exchange_strong(expected, State::Queued)) {
        if (expected == State::Cancelled) {
            return;
        }
        throw err::LogicFailure{"A reserved event continuation already has a queued callback."_el};
    }
    const auto loop = _loop.lock();
    if (loop == nullptr) {
        _state.store(State::Cancelled);
        return;
    }
    auto event = Event{
        id::InvocationEvent,
        std::make_unique<CallbackEventData>([self = shared_from_this(), callback = std::move(callback)]() -> void {
            auto expected = State::Queued;
            if (self->_state.compare_exchange_strong(expected, State::Idle)) {
                callback();
            }
        })};
    {
        const auto lock = std::scoped_lock{loop->_mutex};
        if (loop->_quitRequested) {
            _state.store(State::Cancelled);
            return;
        }
        loop->_continuations.push_back(std::move(event));
    }
    loop->_driver->wake();
}

void EventContinuation::invokeAfter(time::TimeDelta delay, EventCallback callback) {
    if (!delay.isPositive()) {
        invoke(std::move(callback));
        return;
    }
    auto expected = State::Idle;
    if (!_state.compare_exchange_strong(expected, State::Scheduled)) {
        if (expected == State::Cancelled) {
            return;
        }
        throw err::LogicFailure{"A reserved event continuation already has a scheduled callback."_el};
    }
    if (const auto loop = _loop.lock(); loop != nullptr) {
        const auto scheduler = dynamic_cast<SchedulerBackend *>(&loop->get<EventScheduler>());
        if (scheduler == nullptr) {
            _state.store(State::Cancelled);
            throw EventQueueFullError{"This scheduler does not support reserved deadlines."_el};
        }
        scheduler->invokeAfterReserved(delay, shared_from_this(), std::move(callback));
    } else {
        _state.store(State::Cancelled);
    }
}

void EventContinuation::cancel() noexcept {
    const auto previous = _state.exchange(State::Cancelled);
    if (previous == State::Scheduled) {
        if (const auto loop = _loop.lock(); loop != nullptr) {
            if (const auto scheduler = dynamic_cast<SchedulerBackend *>(&loop->get<EventScheduler>());
                scheduler != nullptr) {
                scheduler->cancelReserved(this);
            }
        }
    }
}

}
