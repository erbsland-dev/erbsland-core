// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixSignalDispatcher.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <utility>

namespace erbsland::cterm::impl {

std::array<struct sigaction, PosixSignalDispatcher::cSignals.size()> PosixSignalDispatcher::_previousSignalActions{};
volatile sig_atomic_t PosixSignalDispatcher::_signalWriteFd{-1};

PosixSignalDispatcher::PosixSignalDispatcher(SignalCallback onSignal) : _onSignal{std::move(onSignal)} {
    openSignalPipe();
    startWatcher();
    registerHandlers();
}

PosixSignalDispatcher::~PosixSignalDispatcher() {
    unregisterHandlers();
    sendShutdownSignal();
    waitForShutdown();
    closeSignalPipe();
}

void PosixSignalDispatcher::openSignalPipe() {
    if (pipe(_signalPipe.data()) != 0) {
        _signalPipe = {-1, -1};
        _signalWriteFd = -1;
        return;
    }
    const auto flags = fcntl(_signalPipe[1], F_GETFL, 0);
    if (flags < 0 || fcntl(_signalPipe[1], F_SETFL, flags | O_NONBLOCK) != 0) {
        closeSignalPipe();
        _signalWriteFd = -1;
        return;
    }
    // make the FD available for the signal handler.
    _signalWriteFd = _signalPipe[1];
}

void PosixSignalDispatcher::startWatcher() {
    if (_signalPipe[0] >= 0) {
        _watcher = std::thread([this]() -> void { runWatcher(); });
    }
}

void PosixSignalDispatcher::sendShutdownSignal() noexcept {
    int shutdown = cShutdownToken;
    ::write(_signalPipe[1], &shutdown, sizeof(shutdown));
}

void PosixSignalDispatcher::waitForShutdown() noexcept {
    if (_watcher.joinable()) {
        _watcher.join();
    }
}

void PosixSignalDispatcher::closeSignalPipe() noexcept {
    _signalWriteFd = -1; // no longer accept signals.
    if (_signalPipe[1] >= 0) {
        ::close(_signalPipe[1]);
        _signalPipe[1] = -1;
    }
    if (_signalPipe[0] >= 0) {
        ::close(_signalPipe[0]);
        _signalPipe[0] = -1;
    }
}

void PosixSignalDispatcher::registerHandlers() {
    struct sigaction action{};
    action.sa_handler = &PosixSignalDispatcher::onSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    for (std::size_t index = 0; index < cSignals.size(); ++index) {
        sigaction(cSignals[index], &action, &_previousSignalActions[index]);
    }
}

void PosixSignalDispatcher::unregisterHandlers() {
    for (std::size_t index = 0; index < cSignals.size(); ++index) {
        sigaction(cSignals[index], &_previousSignalActions[index], nullptr);
    }
}

void PosixSignalDispatcher::runWatcher() {
    while (_signalPipe[0] >= 0) {
        int signalNumber = 0;
        const auto bytesRead = ::read(_signalPipe[0], &signalNumber, sizeof(signalNumber));
        if (bytesRead == 0) {
            break;
        }
        if (bytesRead < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (bytesRead != static_cast<ssize_t>(sizeof(signalNumber))) {
            continue;
        }
        if (signalNumber != cShutdownToken) {
            _onSignal(signalNumber);
        }
        break;
    }
}

void PosixSignalDispatcher::onSignal(const int signalNumber) noexcept {
    const int fd = static_cast<int>(_signalWriteFd);
    if (fd < 0) {
        return;
    }
    const int forwardedSignal = signalNumber;
    // If the pipe is already full, we may lose the signal. That's fine.
    [[maybe_unused]] const auto rc = ::write(fd, &forwardedSignal, sizeof(forwardedSignal));
}

}
