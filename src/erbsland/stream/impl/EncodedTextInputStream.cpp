// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EncodedTextInputStream.hpp"

#include "../StreamError.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/impl/UnsafeDecodeBufferAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringBuilder.hpp"

#include <exception>
#include <utility>

namespace erbsland::stream::impl {

using namespace text;
using namespace text::literals;
using namespace unit;

namespace {

constexpr auto cDecodeBufferSize = ByteLength{4U * 1024U};

}

EncodedTextInputStream::EncodedTextInputStream(
    ByteInputStreamPtr byteInputStream,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) :
    _byteInputStream{std::move(byteInputStream)}, _decodeBuffer{cDecodeBufferSize, encoding, bomMode, errorMode} {
    if (!_byteInputStream) {
        throw StreamError{StreamErrorContext{
            "Failed to create the text input stream."_el, "The required byte input stream was not provided."_el}};
    }
    if (_byteInputStream->supportsPositioning()) {
        _positionBase = _byteInputStream->position();
    }
}

auto EncodedTextInputStream::createErrorContext() const noexcept -> StreamErrorContext {
    return _byteInputStream->createErrorContext();
}

auto EncodedTextInputStream::encoding() const noexcept -> StringEncoding {
    const auto lock = std::scoped_lock{_mutex};
    return _decodeBuffer.encoding();
}

auto EncodedTextInputStream::effectiveEncoding() const noexcept -> StringEncoding {
    const auto lock = std::scoped_lock{_mutex};
    return _decodeBuffer.effectiveEncoding();
}

auto EncodedTextInputStream::supportsPositioning() const noexcept -> bool {
    return _byteInputStream->supportsPositioning();
}

auto EncodedTextInputStream::position() const -> unit::ByteIndex {
    if (!supportsPositioning()) {
        return StreamPositioning::position();
    }
    const auto lock = std::scoped_lock{_mutex};
    return positionLocked();
}

auto EncodedTextInputStream::setPosition(const unit::ByteIndex position) -> StreamPositionStatus {
    if (!supportsPositioning()) {
        return StreamPositioning::setPosition(position);
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamPositionStatus::Timeout;
    }
    if (!position.isZero() && !canContinueAtNonzeroPosition()) {
        throwError(
            "Failed to set the text stream position."_el,
            "The text stream byte order must be resolved before positioning away from byte zero."_el);
    }
    const auto effectiveEncoding = _decodeBuffer.effectiveEncoding();
    const auto result = _byteInputStream->setPosition(position);
    if (result.isSuccess()) {
        resetAfterPositioning(position, effectiveEncoding);
    }
    return result;
}

auto EncodedTextInputStream::movePosition(const StreamPositionOrigin origin, const unit::ByteOffset offset)
    -> StreamPositionStatus {
    if (!supportsPositioning()) {
        return StreamPositioning::movePosition(origin, offset);
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamPositionStatus::Timeout;
    }
    auto target = unit::ByteIndex{};
    if (origin == StreamPositionOrigin::Start) {
        target = unit::ByteIndex{}.movedOrThrow(offset);
    } else if (origin == StreamPositionOrigin::Current) {
        target = positionLocked().movedOrThrow(offset);
    } else if (!canContinueAtNonzeroPosition()) {
        throwError(
            "Failed to move the text stream position."_el,
            "The text stream byte order must be resolved before positioning relative to the end."_el);
    }
    if (!target.isZero() && origin != StreamPositionOrigin::End && !canContinueAtNonzeroPosition()) {
        throwError(
            "Failed to move the text stream position."_el,
            "The text stream byte order must be resolved before positioning away from byte zero."_el);
    }

    const auto effectiveEncoding = _decodeBuffer.effectiveEncoding();
    const auto result = origin == StreamPositionOrigin::End ? _byteInputStream->movePosition(origin, offset)
                                                            : _byteInputStream->setPosition(target);
    if (result.isSuccess()) {
        target = _byteInputStream->position();
        resetAfterPositioning(target, effectiveEncoding);
    }
    return result;
}

auto EncodedTextInputStream::inputSettings() const noexcept -> const InputStreamSettings & {
    return _byteInputStream->inputSettings();
}

auto EncodedTextInputStream::state() const noexcept -> StreamState {
    return _byteInputStream->state();
}

auto EncodedTextInputStream::isReady() const noexcept -> bool {
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return false;
    }
    return !_replayText.isEmpty() || sourceIsReadyLocked();
}

auto EncodedTextInputStream::waitForReady() -> StreamWaitStatus {
    if (isReady()) {
        return StreamWaitStatus::Ready;
    }
    return _byteInputStream->waitForReady();
}

auto EncodedTextInputStream::deadlineFromNow() const -> ReadDeadline {
    return time::TimePoint::inFuture(inputSettings().timeout());
}

auto EncodedTextInputStream::close() -> StreamCloseStatus {
    const auto result = _byteInputStream->close();
    if (result.isClosed()) {
        const auto lock = std::unique_lock{_mutex, std::try_to_lock};
        if (lock.owns_lock()) {
            _pendingText = {};
            _replayText = {};
            _aggregateKind = AggregateReadKind::None;
            _aggregateTarget = {};
        }
    }
    return result;
}

void EncodedTextInputStream::abort() noexcept {
    {
        const auto lock = std::unique_lock{_mutex, std::try_to_lock};
        if (lock.owns_lock()) {
            _pendingText = {};
            _replayText = {};
            _aggregateKind = AggregateReadKind::None;
            _aggregateTarget = {};
        }
    }
    if (_byteInputStream) {
        _byteInputStream->abort();
    }
}

auto EncodedTextInputStream::readChar() -> StreamReadResult<Char> {
    const auto result = read(CpLength::one());
    if (result != StreamReadStatus::Data) {
        return {result.status(), Char{}};
    }
    return {StreamReadStatus::Data, result.data().charAt(StringSide::Front)};
}

auto EncodedTextInputStream::read(const CpLength maximum) -> StreamReadResult<String> {
    if (maximum.isInfinite()) {
        throw err::ParameterError{"The maximum text read length must be finite.", "maximum"};
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, String{}};
    }
    cancelAggregateRead();
    auto replay = takeReplay(maximum);
    if (!replay.isEmpty()) {
        return {StreamReadStatus::Data, std::move(replay)};
    }
    return readDecodedText(maximum, deadlineFromNow());
}

auto EncodedTextInputStream::readLine(const CpLength maximum) -> StreamReadResult<String> {
    if (maximum.isInfinite()) {
        throw err::ParameterError{"The maximum line length must be finite.", "maximum"};
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, String{}};
    }
    selectAggregateRead(AggregateReadKind::Line, maximum);
    if (maximum.isZero()) {
        return {StreamReadStatus::Data, takePending()};
    }
    const auto deadline = deadlineFromNow();
    while (!pendingLineIsComplete()) {
        if (!_pendingText.isEmpty() && !sourceIsReadyLocked()) {
            return {StreamReadStatus::Timeout, String{}};
        }
        const auto result = readLineChunk(maximum - _pendingText.characterLength(), deadline);
        if (result == StreamReadStatus::Timeout) {
            return {StreamReadStatus::Timeout, String{}};
        }
        if (result == StreamReadStatus::Finished) {
            if (_pendingText.isEmpty()) {
                _aggregateKind = AggregateReadKind::None;
                _aggregateTarget = {};
                return {StreamReadStatus::Finished, String{}};
            }
            return {StreamReadStatus::Data, takePending()};
        }
        _pendingText.append(result.data());
    }
    return {StreamReadStatus::Data, takePending()};
}

auto EncodedTextInputStream::readAll(const CpLength maximum) -> StreamReadResult<String> {
    if (maximum.isInfinite()) {
        throw err::ParameterError{"The maximum aggregate text length must be finite.", "maximum"};
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, String{}};
    }
    selectAggregateRead(AggregateReadKind::All, maximum);
    if (maximum.isZero()) {
        return {StreamReadStatus::Data, takePending()};
    }
    const auto deadline = deadlineFromNow();
    while (_pendingText.characterLength() < maximum) {
        if (!_pendingText.isEmpty() && !sourceIsReadyLocked()) {
            return {StreamReadStatus::Timeout, String{}};
        }
        auto replay = takeReplay(maximum - _pendingText.characterLength());
        auto result = replay.isEmpty() ? readDecodedText(maximum - _pendingText.characterLength(), deadline)
                                       : StreamReadResult<String>{StreamReadStatus::Data, std::move(replay)};
        if (result == StreamReadStatus::Timeout) {
            return {StreamReadStatus::Timeout, String{}};
        }
        if (result == StreamReadStatus::Finished) {
            if (_pendingText.isEmpty()) {
                _aggregateKind = AggregateReadKind::None;
                _aggregateTarget = {};
                return {StreamReadStatus::Finished, String{}};
            }
            return {StreamReadStatus::Data, takePending()};
        }
        _pendingText.append(result.data());
    }
    return {StreamReadStatus::Data, takePending()};
}

auto EncodedTextInputStream::readLineChunk(const CpLength maximum, const ReadDeadline deadline)
    -> StreamReadResult<String> {
    auto replay = takeReplayLine(maximum);
    if (!replay.isEmpty()) {
        return {StreamReadStatus::Data, std::move(replay)};
    }
    auto text = _decodeBuffer.takeStringLine(maximum);
    if (!text.isEmpty()) {
        return {StreamReadStatus::Data, std::move(text)};
    }
    if (_byteInputFinished) {
        return {StreamReadStatus::Finished, String{}};
    }
    const auto status = fillDecodeBuffer(deadline);
    text = _decodeBuffer.takeStringLine(maximum);
    if (!text.isEmpty()) {
        return {StreamReadStatus::Data, std::move(text)};
    }
    return {_byteInputFinished ? StreamReadStatus::Finished : status, String{}};
}

auto EncodedTextInputStream::takeReplay(const CpLength maximum) -> String {
    if (_replayText.isEmpty() || maximum.isZero()) {
        return {};
    }
    const auto count = std::min(maximum, _replayText.characterLength());
    auto [result, remaining] = _replayText.splitAt(unit::CpIndex{count.toRawValue()});
    _replayText = std::move(remaining);
    return result;
}

auto EncodedTextInputStream::takeReplayLine(const CpLength maximum) -> String {
    auto candidate = takeReplay(maximum);
    if (candidate.isEmpty()) {
        return {};
    }
    const auto newline = String::fromCharacter(U'\n');
    const auto newlineIndex = candidate.find(newline);
    if (newlineIndex.isNoIndex()) {
        return candidate;
    }
    auto [line, suffix] = candidate.splitAt(newlineIndex + newline.length());
    if (!suffix.isEmpty()) {
        auto builder = StringBuilder{};
        builder.append(suffix);
        builder.append(_replayText);
        _replayText = builder.takeString();
    }
    return line;
}

void EncodedTextInputStream::selectAggregateRead(const AggregateReadKind kind, const CpLength target) {
    if (_aggregateKind == kind && _aggregateTarget == target) {
        return;
    }
    cancelAggregateRead();
    _aggregateKind = kind;
    _aggregateTarget = target;
}

void EncodedTextInputStream::cancelAggregateRead() {
    if (_aggregateKind == AggregateReadKind::None) {
        return;
    }
    if (!_pendingText.isEmpty()) {
        auto builder = StringBuilder{};
        builder.append(_pendingText);
        builder.append(_replayText);
        _replayText = builder.takeString();
    }
    _pendingText = {};
    _aggregateKind = AggregateReadKind::None;
    _aggregateTarget = {};
}

auto EncodedTextInputStream::takePending() -> String {
    auto result = std::move(_pendingText);
    _pendingText = {};
    _aggregateKind = AggregateReadKind::None;
    _aggregateTarget = {};
    return result;
}

auto EncodedTextInputStream::pendingLineIsComplete() const -> bool {
    const auto newline = String::fromCharacter(U'\n');
    return _pendingText.characterLength() >= _aggregateTarget || _pendingText.endsWith(newline);
}

auto EncodedTextInputStream::sourceIsReadyLocked() const noexcept -> bool {
    auto &buffer = const_cast<text::StringDecodeBuffer &>(_decodeBuffer);
    return buffer.decodableCharacters(CpLength::one()) > CpLength::zero() || _byteInputFinished ||
        _byteInputStream->isReady();
}

auto EncodedTextInputStream::positionLocked() const -> unit::ByteIndex {
    auto &buffer = const_cast<text::StringDecodeBuffer &>(_decodeBuffer);
    const auto access = text::impl::UnsafeDecodeBufferAccess{buffer};
    return _positionBase.advancedOrThrow(access.consumedByteLength());
}

auto EncodedTextInputStream::canContinueAtNonzeroPosition() const noexcept -> bool {
    const auto encoding = _decodeBuffer.encoding();
    if (encoding != StringEncoding::Utf16 && encoding != StringEncoding::Utf32) {
        return true;
    }
    auto &buffer = const_cast<text::StringDecodeBuffer &>(_decodeBuffer);
    return text::impl::UnsafeDecodeBufferAccess{buffer}.isBomResolved();
}

void EncodedTextInputStream::resetAfterPositioning(
    const unit::ByteIndex position, const StringEncoding effectiveEncoding) {
    _byteInputFinished = false;
    _pendingText = {};
    _replayText = {};
    _aggregateKind = AggregateReadKind::None;
    _aggregateTarget = {};
    _positionBase = position;
    if (position.isZero()) {
        _decodeBuffer.reset();
    } else {
        text::impl::UnsafeDecodeBufferAccess{_decodeBuffer}.resetForContinuation(effectiveEncoding);
    }
}

auto EncodedTextInputStream::fillDecodeBuffer(const ReadDeadline deadline) -> StreamReadStatus {
    auto access = text::impl::UnsafeDecodeBufferAccess{_decodeBuffer};
    const auto destination = access.writableSpan();
    if (destination.empty()) {
        throwError(
            "Failed to decode text from the input stream."_el,
            "The text decode buffer is full but does not contain a complete character."_el);
    }
    const auto result = _byteInputStream->readUntil(destination, deadline);
    if (result == StreamReadStatus::Finished) {
        _byteInputFinished = true;
        _decodeBuffer.finish();
        return StreamReadStatus::Finished;
    }
    if (result == StreamReadStatus::Data) {
        access.commitWritten(result.data());
    }
    if (result == StreamReadStatus::Data) {
        return StreamReadStatus::Data;
    }
    return result == StreamReadStatus::Finished ? StreamReadStatus::Finished : StreamReadStatus::Timeout;
}

auto EncodedTextInputStream::readDecodedText(const CpLength maximum, const ReadDeadline deadline)
    -> StreamReadResult<String> {
    if (maximum.isZero()) {
        return {
            _byteInputFinished && _decodeBuffer.isEmpty() ? StreamReadStatus::Finished : StreamReadStatus::Data,
            String{}};
    }

    auto text = _decodeBuffer.takeString(maximum);
    if (!text.isEmpty()) {
        return {StreamReadStatus::Data, std::move(text)};
    }
    if (_byteInputFinished) {
        return {StreamReadStatus::Finished, String{}};
    }
    const auto status = fillDecodeBuffer(deadline);
    text = _decodeBuffer.takeString(maximum);
    if (!text.isEmpty()) {
        return {StreamReadStatus::Data, std::move(text)};
    }
    return {_byteInputFinished ? StreamReadStatus::Finished : status, String{}};
}

}
