// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ScriptedByteInputStream.hpp"

#include <algorithm>
#include <thread>

namespace demo {

ScriptedByteInputStream::ScriptedByteInputStream(
    std::vector<uint8_t> bytes,
    const std::size_t maximumChunk,
    const std::size_t timeoutCount,
    const bool ready,
    const bool blocked) :
    _bytes{std::move(bytes)},
    _maximumChunk{maximumChunk},
    _timeoutCount{timeoutCount},
    _ready{ready},
    _blocked{blocked} {
}

auto ScriptedByteInputStream::isReady() const noexcept -> bool {
    return _ready && _state == el::StreamState::Open;
}

auto ScriptedByteInputStream::waitForReady() -> el::StreamWaitStatus {
    if (_state != el::StreamState::Open) {
        return el::StreamWaitStatus::Timeout;
    }
    if (!_ready) {
        _ready = true;
        return el::StreamWaitStatus::Timeout;
    }
    return el::StreamWaitStatus::Ready;
}

auto ScriptedByteInputStream::close() -> el::StreamCloseStatus {
    _state = el::StreamState::Closed;
    return el::StreamCloseStatus::Closed;
}

void ScriptedByteInputStream::abort() noexcept {
    _state = el::StreamState::Closed;
}

auto ScriptedByteInputStream::readFromSource(
    const std::span<el::Byte> destination, [[maybe_unused]] ReadDeadline deadline)
    -> el::StreamReadResult<el::ByteLength> {
    ++_readCount;
    if (_failure) {
        _state = el::StreamState::Failed;
        throwError("Failed to read the scripted stream."_el, "The simulated sensor source failed."_el);
    }
    if (_state != el::StreamState::Open) {
        throwError("Failed to read the scripted stream."_el, "The stream is not open."_el);
    }
    while (_blocked && !_released) {
        std::this_thread::yield();
    }
    if (_timeoutCount > 0U) {
        --_timeoutCount;
        return {el::StreamReadStatus::Timeout, el::ByteLength::zero()};
    }
    if (_position == _bytes.size()) {
        return {el::StreamReadStatus::Finished, el::ByteLength::zero()};
    }
    const auto count = std::min({_maximumChunk, destination.size(), _bytes.size() - _position});
    for (auto index = std::size_t{0U}; index < count; ++index) {
        destination[index] = el::Byte{_bytes[_position + index]};
    }
    _position += count;
    return {el::StreamReadStatus::Data, el::ByteLength::fromSizeT(count)};
}

}
