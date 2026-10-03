// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SubprocessState.hpp"

#include "SubprocessErrorContext.hpp"

#include "../../../core/Definitions.hpp"
#include "../../../err/LogicFailure.hpp"
#include "../../../err/ParameterFailure.hpp"
#include "../../PlatformError.hpp"

#include <algorithm>
#include <chrono>

namespace erbsland::system::impl {

using namespace text::literals;

SubprocessState::SubprocessState(
    path::Path executable, text::StringList arguments, SubprocessOptions options, BackendFactory factory) :
    _executable{std::move(executable)},
    _arguments{std::move(arguments)},
    _options{std::move(options)},
    _factory{std::move(factory)} {
    for (auto *output : {&_output, &_errorOutput}) {
        output->pending = std::make_unique<mem::RingBuffer>(
            std::min(_options.captureLimit(), unit::ByteCount{4096}), _options.captureLimit());
    }
}

SubprocessState::~SubprocessState() {
    shutdown();
}

void SubprocessState::start() {
    beginLaunch();
    try {
        launchNative();
    } catch (const PlatformError &error) {
        recordFailure(std::current_exception(), error.context());
        cleanupNative();
        complete();
        throw;
    } catch (...) {
        // Fatal launch failures keep their original type and never become source error/final events.
        closeObservation();
        recordFailure(std::current_exception(), {});
        cleanupNative();
        complete();
        throw;
    }
    wake();
}

void SubprocessState::monitor() noexcept {
    try {
        while (true) {
            const auto result = pollNative();
            if (result.has_value()) {
                recordStatus(*result);
                break;
            }
            // Never hold the native lock while delaying: terminate() and source destruction must not starve.
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }
    } catch (const PlatformError &error) {
        recordFailure(std::current_exception(), error.context());
    }
    if (hasFailure()) {
        cleanupNative();
    }
    complete();
}

void SubprocessState::cleanupNative() noexcept {
    const auto nativeLock = std::scoped_lock{_nativeMutex};
    if (_backend == nullptr) {
        return;
    }
    try {
        if (_backend->isRunning()) {
            _backend->kill();
        }
        _backend->wait();
    } catch (const PlatformError &) { // Preserve the original workflow failure; source destruction retries termination.
    }
}

void SubprocessState::shutdown() noexcept {
    closeObservation();
    try {
        if (isRunning()) {
            terminate(false);
            if (!wait(time::TimeDelta::milliseconds(500)).has_value()) {
                terminate(true);
            }
        }
    } catch (const PlatformError &) {
        try {
            terminate(true);
        } catch (const PlatformError &) { // Destruction cannot report cleanup failures.
        }
    }
    if (_monitor.joinable()) {
        _monitor.join();
    }
}

void SubprocessState::readError(PlatformErrorContextConstPtr error) {
    recordFailure(std::make_exception_ptr(PlatformError{"Failed to read subprocess output."_el, error}), error);
    wake();
}

auto SubprocessState::wait(const std::optional<time::TimeDelta> timeout) -> std::optional<SubprocessExitStatus> {
    auto lock = std::unique_lock{_mutex};
    if (_phase == Phase::Prepared) {
        throw err::LogicFailure{"Cannot wait for a subprocess before start()."_el};
    }
    if (timeout.has_value()) {
        if (timeout->isNegative()) {
            throw err::ParameterFailure{"Wait timeout must not be negative."_el, "timeout"_el};
        }
        if (!_completedCondition.wait_for(
                lock, timeout->toStdNanoseconds(), [this]() -> bool { return _phase == Phase::Completed; })) {
            return {};
        }
    } else {
        _completedCondition.wait(lock, [this]() -> bool { return _phase == Phase::Completed; });
    }
    if (_failure != nullptr) {
        std::rethrow_exception(_failure);
    }
    return _status;
}

void SubprocessState::terminate(const bool force) {
    const auto nativeLock = std::scoped_lock{_nativeMutex};
    if (_backend == nullptr) {
        return;
    }
    if (force) {
        _backend->kill();
    } else {
        _backend->terminate();
    }
}

auto SubprocessState::status() const -> std::optional<SubprocessExitStatus> {
    const auto lock = std::scoped_lock{_mutex};
    return _status;
}

auto SubprocessState::processId() const noexcept -> ProcessId {
    const auto lock = std::scoped_lock{_mutex};
    return _processId;
}

auto SubprocessState::isRunning() const -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return _processId.isValid() && _phase != Phase::Completed;
}

void SubprocessState::beginLaunch() {
    const auto lock = std::scoped_lock{_mutex};
    if (_phase != Phase::Prepared) {
        throw err::LogicFailure{"A subprocess can only be started once."_el};
    }
    _phase = Phase::Starting;
}

void SubprocessState::launchNative() {
    const auto nativeLock = std::scoped_lock{_nativeMutex};
    _backend = _factory(
        _executable,
        _arguments,
        _options,
        SubprocessOutputSink{
            [this](bool error, std::string_view bytes) -> void { receive(error, bytes); },
            [this](PlatformErrorContextConstPtr error) -> void { readError(std::move(error)); }});
    recordLaunch(_backend->processId());
    _monitor = std::thread{[this]() -> void { monitor(); }};
}

void SubprocessState::recordLaunch(ProcessId id) {
    const auto lock = std::scoped_lock{_mutex};
    _processId = id;
    _phase = Phase::Running;
    _startedPending = _observation == Observation::Active;
}

auto SubprocessState::pollNative() -> std::optional<SubprocessExitStatus> {
    const auto nativeLock = std::scoped_lock{_nativeMutex};
    if (hasFailure()) {
        _backend->kill();
    }
    return _backend->wait(time::TimeDelta::zero());
}

void SubprocessState::recordStatus(SubprocessExitStatus status) {
    const auto lock = std::scoped_lock{_mutex};
    _status = status;
    auto abnormal = status.wasSignaled();
#ifdef ERBSLAND_OS_WINDOWS
    abnormal = status.hasExited() && (static_cast<std::uint32_t>(*status.exitCode()) & 0x80000000U) != 0U;
#endif
    if (abnormal && _error == nullptr) {
        _error = std::make_shared<SubprocessErrorContext>(status);
    }
}

void SubprocessState::recordFailure(std::exception_ptr failure, PlatformErrorContextConstPtr error) {
    const auto lock = std::scoped_lock{_mutex};
    if (_failure == nullptr) {
        _failure = std::move(failure);
        _error = std::move(error);
    }
}

auto SubprocessState::hasFailure() const -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return _failure != nullptr;
}

void SubprocessState::markCompleted() {
    const auto lock = std::scoped_lock{_mutex};
    flushOutputBomPrefix(_output);
    flushOutputBomPrefix(_errorOutput);
    _phase = Phase::Completed;
    _terminalPending = _observation == Observation::Active;
}

void SubprocessState::complete() {
    markCompleted();
    _completedCondition.notify_all();
    wake();
}

}
