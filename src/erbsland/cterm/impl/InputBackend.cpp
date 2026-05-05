// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InputBackend.hpp"

namespace erbsland::cterm::impl {

auto InputBackend::mode() const noexcept -> Mode {
    return _backend->inputMode();
}

void InputBackend::setMode(const Mode mode) {
    _backend->setInputMode(mode);
}

auto InputBackend::readKeyImpl(const std::chrono::milliseconds timeout) const -> Key {
    return _backend->readKey(timeout);
}

auto InputBackend::waitForKeyImpl() const -> Key {
    return _backend->waitForKey();
}

auto InputBackend::readLine() -> text::String {
    return _backend->readLine();
}

void InputBackend::setBackend(BackendPtr backend) noexcept {
    _backend = std::move(backend);
}

}
