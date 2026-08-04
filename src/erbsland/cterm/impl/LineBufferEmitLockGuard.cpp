// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LineBufferEmitLockGuard.hpp"

#include "LineBuffer.hpp"

namespace erbsland::cterm::impl {

LineBufferEmitLockGuard::LineBufferEmitLockGuard(LineBuffer &lineBuffer) : _lineBuffer{lineBuffer} {
    _lineBuffer._emitLock.lock();
}

LineBufferEmitLockGuard::~LineBufferEmitLockGuard() {
    _lineBuffer._emitLock.unlock();
}

}
