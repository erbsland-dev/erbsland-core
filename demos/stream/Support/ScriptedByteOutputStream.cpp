// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ScriptedByteOutputStream.hpp"

namespace demo {

ScriptedByteOutputStream::ScriptedByteOutputStream(
    const std::size_t writeTimeoutCount, const std::size_t flushTimeoutCount, const std::size_t closeTimeoutCount) :
    _writeTimeoutCount{writeTimeoutCount},
    _flushTimeoutCount{flushTimeoutCount},
    _closeTimeoutCount{closeTimeoutCount} {
}

auto ScriptedByteOutputStream::isReady() const noexcept -> bool {
    return _writeTimeoutCount == 0U && _state == el::StreamState::Open;
}

auto ScriptedByteOutputStream::waitForReady() -> el::StreamWaitStatus {
    if (_state != el::StreamState::Open) {
        return el::StreamWaitStatus::Timeout;
    }
    if (_writeTimeoutCount > 0U) {
        --_writeTimeoutCount;
        return el::StreamWaitStatus::Timeout;
    }
    return el::StreamWaitStatus::Ready;
}

auto ScriptedByteOutputStream::flush() -> el::StreamWriteStatus {
    if (_failure) {
        _state = el::StreamState::Failed;
        throwError("Failed to flush the scripted stream."_el, "The simulated recorder failed."_el);
    }
    if (_flushTimeoutCount > 0U) {
        --_flushTimeoutCount;
        return el::StreamWriteStatus::Timeout;
    }
    return el::StreamWriteStatus::Success;
}

auto ScriptedByteOutputStream::close() -> el::StreamCloseStatus {
    if (_failure) {
        _state = el::StreamState::Failed;
        throwError("Failed to close the scripted stream."_el, "The simulated recorder failed."_el);
    }
    _state = el::StreamState::Closing;
    if (_closeTimeoutCount > 0U) {
        --_closeTimeoutCount;
        return el::StreamCloseStatus::Timeout;
    }
    _state = el::StreamState::Closed;
    return el::StreamCloseStatus::Closed;
}

void ScriptedByteOutputStream::abort() noexcept {
    _bytes.clear();
    _state = el::StreamState::Closed;
}

auto ScriptedByteOutputStream::write(const std::span<const el::Byte> bytes) -> el::StreamWriteStatus {
    if (_failure) {
        _state = el::StreamState::Failed;
        throwError("Failed to write the scripted stream."_el, "The simulated recorder failed."_el);
    }
    if (_state != el::StreamState::Open) {
        throwError("Failed to write the scripted stream."_el, "The stream is not open."_el);
    }
    if (_writeTimeoutCount > 0U) {
        --_writeTimeoutCount;
        return el::StreamWriteStatus::Timeout;
    }
    for (const auto byte : bytes) {
        _bytes.push_back(byte.toUInt8());
    }
    _position += el::ByteLength::fromSizeT(bytes.size());
    return el::StreamWriteStatus::Success;
}

auto ScriptedByteOutputStream::position() const -> el::ByteIndex {
    if (!_positioning) {
        return el::ByteOutputStream::position();
    }
    return _position;
}

auto ScriptedByteOutputStream::setPosition(const el::ByteIndex position) -> el::StreamPositionStatus {
    if (!_positioning) {
        return el::ByteOutputStream::setPosition(position);
    }
    if (_positionTimeoutCount > 0U) {
        --_positionTimeoutCount;
        return el::StreamPositionStatus::Timeout;
    }
    _position = position;
    return el::StreamPositionStatus::Success;
}

auto ScriptedByteOutputStream::movePosition(const el::StreamPositionOrigin origin, const el::ByteOffset offset)
    -> el::StreamPositionStatus {
    if (!_positioning) {
        return el::ByteOutputStream::movePosition(origin, offset);
    }
    auto base = el::ByteIndex{};
    if (origin == el::StreamPositionOrigin::Current) {
        base = _position;
    } else if (origin == el::StreamPositionOrigin::End) {
        base = el::ByteIndex::fromSizeT(_bytes.size());
    }
    return setPosition(base + offset);
}

}
