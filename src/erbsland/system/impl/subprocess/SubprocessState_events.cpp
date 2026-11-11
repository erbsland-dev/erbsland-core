// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SubprocessState.hpp"

#include "../../../err/RuntimeError.hpp"
#include "../../../event/EventBackendTarget.hpp"
#include "../../../event/EventTarget.hpp"

namespace erbsland::system::impl {

void SubprocessState::bind(event::EventBackendTargetWeakPtr target) {
    const auto lock = std::scoped_lock{_mutex};
    _target = std::move(target);
    _observation = Observation::Active;
}

auto SubprocessState::isClosed() const -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return _observation == Observation::Closed;
}

void SubprocessState::closeObservation() {
    const auto lock = std::scoped_lock{_mutex};
    _observation = Observation::Closed;
}

void SubprocessState::wake() noexcept {
    if (const auto target = claimWake()) {
        target->wakeFromBackend();
    }
}

auto SubprocessState::claimWake() -> event::EventBackendTargetPtr {
    const auto lock = std::scoped_lock{_mutex};
    if (_observation == Observation::Closed || _wakePending) {
        return {};
    }
    auto target = _target.lock();
    _wakePending = target != nullptr;
    return target;
}

void SubprocessState::queueDispatch(const event::EventTargetPtr &owner, std::weak_ptr<SubprocessState> self) {
    if (!claimDispatch()) {
        return;
    }
    try {
        owner->invoke([self]() -> void {
            if (const auto state = self.lock()) {
                state->dispatch();
            }
        });
    } catch (...) {
        releaseDispatch();
        throw;
    }
}

auto SubprocessState::claimDispatch() -> bool {
    const auto lock = std::scoped_lock{_mutex};
    _wakePending = false;
    if (_observation == Observation::Closed || _deliveryQueued ||
        (!_processId.isValid() && _phase != Phase::Completed)) {
        return false;
    }
    if (!_startedPending && _overflow != Overflow::Pending && !_terminalPending && !outputReady(_output) &&
        !outputReady(_errorOutput)) {
        return false;
    }
    _deliveryQueued = true;
    return true;
}

void SubprocessState::releaseDispatch() {
    const auto lock = std::scoped_lock{_mutex};
    _deliveryQueued = false;
}

void SubprocessState::dispatch() {
    releaseDispatch();
    if (const auto callback = takeStarted()) {
        callback();
    }
    if (const auto callback = takeOverflow()) {
        callback();
    }
    // Bound work per dispatch to preserve fairness for continuously producing children.
    for (auto count = 0; count < 16; ++count) {
        deliverOutput(false);
        deliverOutput(true);
    }
    sendCompletion(takeCompletion());
}

auto SubprocessState::takeStarted() -> system::SubprocessEventEditor::EventFn {
    const auto lock = std::scoped_lock{_mutex};
    if (_observation == Observation::Closed || !_startedPending) {
        return {};
    }
    _startedPending = false;
    return _onStarted;
}

auto SubprocessState::takeOverflow() -> system::SubprocessEventEditor::EventFn {
    const auto lock = std::scoped_lock{_mutex};
    if (_observation == Observation::Closed || _overflow != Overflow::Pending) {
        return {};
    }
    _overflow = Overflow::Delivered;
    return _onOutputTruncated;
}

auto SubprocessState::takeCompletion() -> Completion {
    const auto lock = std::scoped_lock{_mutex};
    if (_observation == Observation::Closed || !_terminalPending || !_output.pending->isEmpty() ||
        !_errorOutput.pending->isEmpty()) {
        return {};
    }
    _terminalPending = false;
    return {_onExit, _onError, _onFinal, _error, _status};
}

void SubprocessState::sendCompletion(const Completion &completion) {
    try {
        if (completion.error != nullptr) {
            if (completion.failed) {
                completion.failed(*completion.error);
            }
        } else if (completion.exited && completion.status.has_value() && completion.status->hasExited()) {
            completion.exited(unit::ExitCode{*completion.status->exitCode()});
        }
    } catch (const err::RuntimeError &) {
        if (completion.final && !isClosed()) {
            completion.final();
        }
        throw;
    }
    if (completion.final && !isClosed()) {
        completion.final();
    }
}

void SubprocessState::setStarted(system::SubprocessEventEditor::EventFn callback) {
    const auto lock = std::scoped_lock{_mutex};
    _onStarted = std::move(callback);
}

void SubprocessState::setOutputTruncated(system::SubprocessEventEditor::EventFn callback) {
    const auto lock = std::scoped_lock{_mutex};
    _onOutputTruncated = std::move(callback);
}

void SubprocessState::setExit(system::SubprocessEventEditor::ExitFn callback) {
    const auto lock = std::scoped_lock{_mutex};
    _onExit = std::move(callback);
}

void SubprocessState::setError(system::SubprocessEventEditor::ErrorFn callback) {
    const auto lock = std::scoped_lock{_mutex};
    _onError = std::move(callback);
}

void SubprocessState::setFinal(system::SubprocessEventEditor::EventFn callback) {
    const auto lock = std::scoped_lock{_mutex};
    _onFinal = std::move(callback);
}

}
