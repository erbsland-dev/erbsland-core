// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LineBuffer.hpp"

#include "../../unit/ByteCount.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteRange.hpp"

#include <cassert>

namespace erbsland::cterm::impl {

using namespace text;
using namespace unit;

void LineBuffer::setCachingEnabled(const bool enabled) noexcept {
    if (_cachingEnabled != enabled) {
        _cachingEnabled = enabled;
        if (enabled) {
            _buffer.reserve(ByteCount::fromSizeT(cEnabledSize));
        } else {
            handleEmit(true);
        }
    }
}

void LineBuffer::write(const String &text) noexcept {
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
    if (_buffer.length() >= ByteCount::fromSizeT(cMaxSize)) { // force emit if the buffer gets too large.
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
    const auto emittedEnd = nlPos + ByteCount::one();
    _backend->emitText(_buffer.slice(StringSide::Front, emittedEnd));
    _buffer.remove(ByteRange{ByteIndex::zero(), emittedEnd});
}

void LineBuffer::shutdown() noexcept {
    handleEmit(true);
    _buffer.clear();
    _buffer.shrinkToFit();
}

auto LineBuffer::newLineCharacters() -> const CharSet & {
    static const auto cNewLineCharacters = CharSet{U'\n'};
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

}
