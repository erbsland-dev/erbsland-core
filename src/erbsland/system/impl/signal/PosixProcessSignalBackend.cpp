// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixProcessSignalBackend.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <utility>

namespace erbsland::system::impl {

std::array<struct sigaction, PosixProcessSignalBackend::cSignals.size()> PosixProcessSignalBackend::_previousActions{};
volatile sig_atomic_t PosixProcessSignalBackend::_writeFd{-1};

auto ProcessSignalBackend::create(SignalFn signalFn) -> std::unique_ptr<ProcessSignalBackend> {
    return std::make_unique<PosixProcessSignalBackend>(std::move(signalFn));
}

PosixProcessSignalBackend::PosixProcessSignalBackend(SignalFn signalFn) : _signalFn{std::move(signalFn)} {
    openPipe();
    if (_pipe[0] >= 0) {
        _watcher = std::thread{[this]() noexcept -> void { runWatcher(); }};
        registerHandlers();
    }
}

PosixProcessSignalBackend::~PosixProcessSignalBackend() {
    unregisterHandlers();
    if (_pipe[1] >= 0) {
        const auto token = cShutdownToken;
        [[maybe_unused]] const auto result = ::write(_pipe[1], &token, sizeof(token));
    }
    if (_watcher.joinable()) {
        _watcher.join();
    }
    _writeFd = -1;
    for (auto &fd : _pipe) {
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
    }
}

void PosixProcessSignalBackend::openPipe() {
    if (::pipe(_pipe.data()) != 0) {
        _pipe = {-1, -1};
        return;
    }
    const auto flags = ::fcntl(_pipe[1], F_GETFL, 0);
    if (flags < 0 || ::fcntl(_pipe[1], F_SETFL, flags | O_NONBLOCK) != 0) {
        ::close(_pipe[0]);
        ::close(_pipe[1]);
        _pipe = {-1, -1};
        return;
    }
    _writeFd = _pipe[1];
}

void PosixProcessSignalBackend::registerHandlers() {
    struct sigaction action{};
    action.sa_handler = &PosixProcessSignalBackend::onSignal;
    sigemptyset(&action.sa_mask);
    for (std::size_t index = 0; index < cSignals.size(); ++index) {
        if (::sigaction(cSignals[index], &action, &_previousActions[index]) != 0) {
            while (index > 0U) {
                --index;
                ::sigaction(cSignals[index], &_previousActions[index], nullptr);
            }
            _writeFd = -1;
            return;
        }
    }
    _handlersRegistered = true;
}

void PosixProcessSignalBackend::unregisterHandlers() noexcept {
    _writeFd = -1;
    if (!_handlersRegistered) {
        return;
    }
    for (std::size_t index = 0; index < cSignals.size(); ++index) {
        ::sigaction(cSignals[index], &_previousActions[index], nullptr);
    }
    _handlersRegistered = false;
}

void PosixProcessSignalBackend::runWatcher() noexcept {
    while (true) {
        auto signalNumber = 0;
        const auto count = ::read(_pipe[0], &signalNumber, sizeof(signalNumber));
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count != static_cast<ssize_t>(sizeof(signalNumber)) || signalNumber == cShutdownToken) {
            return;
        }
        _signalFn(normalizedSignal(signalNumber));
    }
}

void PosixProcessSignalBackend::onSignal(const int signalNumber) noexcept {
    const auto fd = static_cast<int>(_writeFd);
    if (fd >= 0) {
        [[maybe_unused]] const auto result = ::write(fd, &signalNumber, sizeof(signalNumber));
    }
}

auto PosixProcessSignalBackend::normalizedSignal(const int signalNumber) noexcept -> ProcessSignal {
    switch (signalNumber) {
    case SIGINT:
        return ProcessSignal::Interrupt;
    case SIGTERM:
        return ProcessSignal::Terminate;
    case SIGHUP:
        return ProcessSignal::Hangup;
    default:
        return ProcessSignal::Quit;
    }
}

auto PosixProcessSignalBackend::nativeSignal(const ProcessSignal signal) noexcept -> int {
    switch (signal) {
    case ProcessSignal::Interrupt:
        return SIGINT;
    case ProcessSignal::Terminate:
        return SIGTERM;
    case ProcessSignal::Hangup:
        return SIGHUP;
    default:
        return SIGQUIT;
    }
}

void PosixProcessSignalBackend::terminateWithDefault(const ProcessSignal signal) noexcept {
    const auto signalNumber = nativeSignal(signal);
    auto *previousAction = static_cast<const struct sigaction *>(nullptr);
    for (std::size_t index = 0; index < cSignals.size(); ++index) {
        if (cSignals[index] == signalNumber) {
            previousAction = &_previousActions[index];
            break;
        }
    }
    if (previousAction == nullptr) {
        std::_Exit(128 + signalNumber);
    }
    ::sigaction(signalNumber, previousAction, nullptr);
    ::kill(::getpid(), signalNumber);
    if (previousAction->sa_handler == SIG_DFL) {
        std::_Exit(128 + signalNumber);
    }
}

}
