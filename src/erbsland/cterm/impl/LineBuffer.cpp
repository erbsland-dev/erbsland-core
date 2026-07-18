// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LineBuffer.hpp"

#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"

#include <cassert>

namespace erbsland::cterm::impl {

void LineBuffer::setCachingEnabled(const bool enabled) noexcept {
    if (_cachingEnabled != enabled) {
        _cachingEnabled = enabled;
        if (enabled) {
            _buffer.reserve(unit::ByteLength::fromSizeT(cEnabledSize));
        } else {
            handleEmit(true);
        }
    }
}

void LineBuffer::write(const text::String &text) noexcept {
    _buffer.append(text);
    if (text.containsOneOf(newLineCharacters())) {
        _hasNewLine = true;
    }
}

void LineBuffer::write(const Block character) noexcept {
    _buffer.append(character.toString());
    if (character == U'\n') {
        _hasNewLine = true;
    }
}

void LineBuffer::handleEmit(const bool forceEmit) noexcept {
    if (!_cachingEnabled || forceEmit) {
        emitFullLineBuffer();
    }
    if (_buffer.length() >= unit::ByteLength::fromSizeT(cMaxSize)) { // force emit if the buffer gets too large.
        emitFullLineBuffer();
        return;
    }
    if (_emitLock.isLocked() || !_hasNewLine) {
        return;
    }
    const auto nlPos = _buffer.findLastOf(newLineCharacters());
    if (nlPos.isNoIndex()) { // coverage: should never happen.
        emitFullLineBuffer();
        return;
    }
    const auto emittedEnd = nlPos + unit::ByteLength::one();
    _backend->emitText(_buffer.slice(text::StringSide::Front, emittedEnd));
    _buffer.remove(unit::ByteRange{unit::ByteIndex::zero(), emittedEnd});
}

void LineBuffer::shutdown() noexcept {
    handleEmit(true);
    _buffer.clear();
    _buffer.shrinkToFit();
}

auto LineBuffer::newLineCharacters() -> const text::CharSet & {
    static const auto cNewLineCharacters = text::CharSet{U'\n'};
    return cNewLineCharacters;
}

void LineBuffer::emitFullLineBuffer() noexcept {
    _backend->emitText(_buffer);
    _buffer.clear();
    _hasNewLine = false;
}

auto LineBuffer::EmitLock::isLocked() const noexcept -> bool {
    return _emitLockCount > 0;
}

void LineBuffer::EmitLock::lock() noexcept {
    assert(_emitLockCount < 100'000); // detect misuse
    _emitLockCount += 1;
}

void LineBuffer::EmitLock::unlock() noexcept {
    assert(_emitLockCount > 0); // detect misuse
    _emitLockCount -= 1;
}

LineBuffer::EmitLockGuard::EmitLockGuard(LineBuffer &lineBuffer) : _lineBuffer{lineBuffer} {
    _lineBuffer._emitLock.lock();
}

LineBuffer::EmitLockGuard::~EmitLockGuard() {
    _lineBuffer._emitLock.unlock();
}

}
