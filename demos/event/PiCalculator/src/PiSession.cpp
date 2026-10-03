// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PiSession.hpp"

#include <erbsland/BindEvent.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/Events.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/String.hpp>

#include <utility>

namespace demo {

using namespace el::text::literals;

PiSession::PiSession(
    el::EventsPtr events,
    std::shared_ptr<DigitHistory> history,
    OnUpdated onUpdated,
    OnFinished onFinished,
    PiWorker::Advance advance) :
    _events{std::move(events)},
    _history{std::move(history)},
    _onUpdated{std::move(onUpdated)},
    _onFinished{std::move(onFinished)},
    _worker{std::move(advance)} {
    if (!_events || !_history || !_onFinished) {
        throw el::ParameterFailure{"A pi session needs a target, history, and completion callback."_el, "session"_el};
    }
}

void PiSession::start(const std::optional<el::CpCount> limit) {
    if (_started) {
        throw el::LogicFailure{"A pi calculation session starts once."_el};
    }
    // Required results retain the session and always return to its owner loop.
    _worker.start(
        limit,
        el::bindEvent(_events, shared_from_this(), &PiSession::onDigits),
        el::bindEvent(_events, shared_from_this(), &PiSession::onFinished));
    _started = true;
}

void PiSession::onDigits(el::String digits) {
    _history->append(std::move(digits));
    if (_onUpdated) {
        _onUpdated();
    }
    // Main accepts the result before asking for more. Optional display delivery never releases a worker slot.
    if (!_stopping) {
        _worker.requestNextBatch();
    }
}

void PiSession::requestStop() {
    if (_finished || _stopping) {
        return;
    }
    _stopping = true;
    _worker.requestStop();
    if (_onUpdated) {
        _onUpdated();
    }
}

void PiSession::onFinished(std::exception_ptr error) {
    _worker.join();
    _finished = true;
    if (_onUpdated) {
        _onUpdated();
    }
    // Retire target-retaining callbacks once the operation is terminal, including on exceptional shutdown.
    _onUpdated = {};
    _events.reset();
    if (auto completion = std::move(_onFinished)) {
        completion(std::move(error));
    }
}

void PiSession::shutdown() {
    // Exception cleanup can run after dispatch has ended; joining must not depend on a pending main-loop callback.
    _stopping = true;
    _worker.requestStop();
    _worker.join();
    // A stopped loop may still own callbacks retaining this session. Break the return ownership path.
    _onUpdated = {};
    _onFinished = {};
    _events.reset();
}

}
