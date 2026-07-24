// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EncodedTextInputStream.hpp"

#include "BufferedByteInputStream.hpp"
#include "RetainedTextBuffer.hpp"
#include "StreamBufferSizes.hpp"

#include "../StreamError.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/impl/UnsafeDecodeBufferAccess.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::stream::impl {

using namespace text;
using namespace text::literals;
using namespace unit;

EncodedTextInputStream::EncodedTextInputStream(
    ByteInputStreamPtr byteInputStream,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingMode mode) :
    _byteInputStream{std::move(byteInputStream)}, _encoding{encoding}, _bomMode{bomMode}, _mode{mode} {
    if (!_byteInputStream) {
        throw StreamError{StreamErrorContext{
            "Failed to create the text input stream."_el, "The required byte input stream was not provided."_el}};
    }
    _retainedText = std::make_unique<RetainedTextBuffer>(_byteInputStream->inputSettings().isSensitive());
    resetDecoder();
    if (_byteInputStream->supportsPositioning()) {
        _positionBase = _byteInputStream->position();
    }
}

EncodedTextInputStream::~EncodedTextInputStream() {
    abort();
}

auto EncodedTextInputStream::inputSettings() const noexcept -> const InputStreamSettings & {
    return _byteInputStream->inputSettings();
}

auto EncodedTextInputStream::state() const noexcept -> StreamState {
    return _byteInputStream->state();
}

auto EncodedTextInputStream::isReady() const noexcept -> bool {
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    return lock.owns_lock() &&
        (!_retainedText->isEmpty() || decoderHasCharacter() || _byteInputFinished || _byteInputStream->isReady());
}

auto EncodedTextInputStream::waitForReady() -> StreamWaitStatus {
    return isReady() ? StreamWaitStatus::Ready : _byteInputStream->waitForReady();
}

auto EncodedTextInputStream::close() -> StreamCloseStatus {
    const auto result = _byteInputStream->close();
    if (result.isClosed()) {
        discardBufferedInput();
    }
    return result;
}

void EncodedTextInputStream::abort() noexcept {
    discardBufferedInput();
    if (_byteInputStream) {
        _byteInputStream->abort();
    }
}

auto EncodedTextInputStream::createErrorContext() const noexcept -> StreamErrorContext {
    return _byteInputStream->createErrorContext();
}

auto EncodedTextInputStream::encoding() const noexcept -> StringEncoding {
    return _encoding;
}

auto EncodedTextInputStream::effectiveEncoding() const noexcept -> StringEncoding {
    const auto lock = std::scoped_lock{_mutex};
    return _decoder->effectiveEncoding();
}

auto EncodedTextInputStream::readChar() -> StreamReadResult<Char> {
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, {}};
    }
    if (!_retainedText->isEmpty()) {
        return {StreamReadStatus::Data, _retainedText->takeChar()};
    }
    const auto takeDecoded = [this]() -> std::optional<Char> { return _decoder->readChar(); };
    if (const auto character = takeDecoded()) {
        return {StreamReadStatus::Data, *character};
    }
    if (_byteInputFinished) {
        return {StreamReadStatus::Finished, {}};
    }
    const auto deadline = deadlineFromNow();
    while (!_byteInputFinished) {
        const auto status = fillDecodeBuffer(deadline);
        if (const auto character = takeDecoded()) {
            return {StreamReadStatus::Data, *character};
        }
        if (status == StreamReadStatus::Timeout) {
            return {StreamReadStatus::Timeout, {}};
        }
    }
    return {StreamReadStatus::Finished, {}};
}

auto EncodedTextInputStream::read(const CpLength maximum) -> StreamReadResult<String> {
    if (maximum.isInfinite()) {
        throw err::ParameterError{"The maximum text read length must be finite.", "maximum"};
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, {}};
    }
    return takeResult(prepareChunk(maximum, deadlineFromNow()), maximum, false);
}

auto EncodedTextInputStream::readLine(const CpLength maximum) -> StreamReadResult<String> {
    if (maximum.isInfinite()) {
        throw err::ParameterError{"The maximum line length must be finite.", "maximum"};
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, {}};
    }
    return takeResult(prepareLine(maximum, deadlineFromNow()), maximum, true);
}

auto EncodedTextInputStream::readAll(const CpLength maximum) -> StreamReadResult<String> {
    if (maximum.isInfinite()) {
        throw err::ParameterError{"The maximum aggregate text length must be finite.", "maximum"};
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return {StreamReadStatus::Timeout, {}};
    }
    return takeResult(prepareAll(maximum, deadlineFromNow()), maximum, false);
}

auto EncodedTextInputStream::supportsPositioning() const noexcept -> bool {
    return _byteInputStream->supportsPositioning();
}

auto EncodedTextInputStream::position() const -> ByteIndex {
    if (!supportsPositioning()) {
        return StreamPositioning::position();
    }
    const auto lock = std::scoped_lock{_mutex};
    return positionLocked();
}

auto EncodedTextInputStream::setPosition(const ByteIndex position) -> StreamPositionStatus {
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
    const auto effective = _decoder->effectiveEncoding();
    const auto result = _byteInputStream->setPosition(position);
    if (result.isSuccess()) {
        resetAfterPositioning(position, effective);
    }
    return result;
}

auto EncodedTextInputStream::movePosition(const StreamPositionOrigin origin, const ByteOffset offset)
    -> StreamPositionStatus {
    if (!supportsPositioning()) {
        return StreamPositioning::movePosition(origin, offset);
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamPositionStatus::Timeout;
    }
    auto target = ByteIndex{};
    if (origin == StreamPositionOrigin::Start) {
        target = ByteIndex{}.movedOrThrow(offset);
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
    const auto effective = _decoder->effectiveEncoding();
    const auto result = origin == StreamPositionOrigin::End ? _byteInputStream->movePosition(origin, offset)
                                                            : _byteInputStream->setPosition(target);
    if (result.isSuccess()) {
        target = _byteInputStream->position();
        resetAfterPositioning(target, effective);
    }
    return result;
}

void EncodedTextInputStream::discardBufferedInput() noexcept {
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock() || !_retainedText) {
        return;
    }
    _retainedText->clear();
    _byteInputFinished = false;
    if (_decoder) {
        _decoder->reset();
    }
}

void EncodedTextInputStream::setSensitive(const bool sensitive) noexcept {
    try {
        const auto lock = std::scoped_lock{_mutex};
        if (_retainedText->isSensitive() == sensitive) {
            return;
        }
        const auto byteStream = std::dynamic_pointer_cast<BufferedByteInputStream>(_byteInputStream);
        if (!byteStream) {
            return;
        }
        auto decoder =
            StringDecodeBuffer{streamBufferSizes(inputSettings().buffering()).decoder, _encoding, _bomMode, _mode};
        decoder.setSensitive(sensitive);
        byteStream->setSensitive(sensitive);
        _retainedText->setSensitive(sensitive);
        _decoder.emplace(std::move(decoder));
        _byteInputFinished = false;
    } catch (...) {
        _byteInputStream->abort();
    }
}

auto EncodedTextInputStream::deadlineFromNow() const -> ReadDeadline {
    return time::TimePoint::inFuture(inputSettings().timeout());
}

auto EncodedTextInputStream::prepareChunk(const CpLength maximum, const ReadDeadline deadline) -> StreamReadStatus {
    if (maximum.isZero()) {
        return _byteInputFinished && decoderIsEmpty() && _retainedText->isEmpty() ? StreamReadStatus::Finished
                                                                                  : StreamReadStatus::Data;
    }
    if (!_retainedText->isEmpty() || appendDecoded(maximum, false)) {
        return StreamReadStatus::Data;
    }
    while (!_byteInputFinished) {
        const auto status = fillDecodeBuffer(deadline);
        if (appendDecoded(maximum, false)) {
            return StreamReadStatus::Data;
        }
        if (status == StreamReadStatus::Timeout) {
            return status;
        }
    }
    return StreamReadStatus::Finished;
}

auto EncodedTextInputStream::prepareLine(const CpLength maximum, const ReadDeadline deadline) -> StreamReadStatus {
    if (maximum.isZero()) {
        return StreamReadStatus::Data;
    }
    while (!_retainedText->lineIsComplete(maximum)) {
        const auto remaining = maximum - _retainedText->length();
        if (appendDecoded(remaining, true)) {
            continue;
        }
        if (_byteInputFinished) {
            return _retainedText->isEmpty() ? StreamReadStatus::Finished : StreamReadStatus::Data;
        }
        const auto status = fillDecodeBuffer(deadline);
        if (status == StreamReadStatus::Timeout) {
            return status;
        }
    }
    return StreamReadStatus::Data;
}

auto EncodedTextInputStream::prepareAll(const CpLength maximum, const ReadDeadline deadline) -> StreamReadStatus {
    if (maximum.isZero()) {
        return StreamReadStatus::Data;
    }
    while (_retainedText->length() < maximum) {
        const auto remaining = maximum - _retainedText->length();
        if (appendDecoded(remaining, false)) {
            continue;
        }
        if (_byteInputFinished) {
            return _retainedText->isEmpty() ? StreamReadStatus::Finished : StreamReadStatus::Data;
        }
        const auto status = fillDecodeBuffer(deadline);
        if (status == StreamReadStatus::Timeout) {
            return status;
        }
    }
    return StreamReadStatus::Data;
}

auto EncodedTextInputStream::appendDecoded(const CpLength maximum, const bool stopAtLineEnd) -> bool {
    if (maximum.isZero()) {
        return false;
    }
    auto [text, textLength] =
        text::impl::UnsafeDecodeBufferAccess{*_decoder}.takeStringWithLength(maximum, stopAtLineEnd);
    if (text.isEmpty()) {
        return false;
    }
    _retainedText->append(std::move(text), textLength, stopAtLineEnd);
    return true;
}

auto EncodedTextInputStream::fillDecodeBuffer(const ReadDeadline deadline) -> StreamReadStatus {
    auto access = text::impl::UnsafeDecodeBufferAccess{*_decoder};
    const auto destination = access.writableSpan();
    if (destination.empty()) {
        throwError(
            "Failed to decode text from the input stream."_el,
            "The text decode buffer is full but does not contain a complete character."_el);
    }
    const auto result = _byteInputStream->readUntil(destination, deadline);
    if (result == StreamReadStatus::Finished) {
        _byteInputFinished = true;
        finishDecoder();
        return StreamReadStatus::Finished;
    }
    if (result == StreamReadStatus::Data) {
        access.commitWritten(result.data());
        return StreamReadStatus::Data;
    }
    return StreamReadStatus::Timeout;
}

auto EncodedTextInputStream::decoderIsEmpty() const noexcept -> bool {
    return _decoder->isEmpty();
}

auto EncodedTextInputStream::decoderHasCharacter() const noexcept -> bool {
    return const_cast<StringDecodeBuffer &>(*_decoder).decodableCharacters(CpLength::one()) > CpLength{};
}

void EncodedTextInputStream::finishDecoder() {
    _decoder->finish();
}

void EncodedTextInputStream::resetDecoder(const std::optional<StringEncoding> continuationEncoding) {
    _decoder.emplace(streamBufferSizes(inputSettings().buffering()).decoder, _encoding, _bomMode, _mode);
    _decoder->setSensitive(_retainedText->isSensitive());
    if (continuationEncoding) {
        text::impl::UnsafeDecodeBufferAccess{*_decoder}.resetForContinuation(*continuationEncoding);
    }
}

auto EncodedTextInputStream::positionLocked() const -> ByteIndex {
    const auto consumed =
        text::impl::UnsafeDecodeBufferAccess{const_cast<StringDecodeBuffer &>(*_decoder)}.consumedByteLength();
    return _positionBase.advancedOrThrow(consumed);
}

auto EncodedTextInputStream::canContinueAtNonzeroPosition() const noexcept -> bool {
    if (_encoding != StringEncoding::Utf16 && _encoding != StringEncoding::Utf32) {
        return true;
    }
    return text::impl::UnsafeDecodeBufferAccess{const_cast<StringDecodeBuffer &>(*_decoder)}.isBomResolved();
}

void EncodedTextInputStream::resetAfterPositioning(const ByteIndex position, const StringEncoding effectiveEncoding) {
    _retainedText->clear();
    _byteInputFinished = false;
    _positionBase = position;
    resetDecoder(position.isZero() ? std::nullopt : std::optional<StringEncoding>{effectiveEncoding});
}

auto EncodedTextInputStream::takeResult(const StreamReadStatus status, const CpLength maximum, const bool line)
    -> StreamReadResult<String> {
    if (status != StreamReadStatus::Data) {
        return {status, {}};
    }
    return {status, _retainedText->take(maximum, line)};
}

}
