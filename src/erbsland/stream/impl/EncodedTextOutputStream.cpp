// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EncodedTextOutputStream.hpp"

#include "BufferedByteOutputStream.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEncoder.hpp"

#include <exception>
#include <string_view>
#include <utility>

namespace erbsland::stream::impl {

using namespace text::literals;
using namespace text;

EncodedTextOutputStream::EncodedTextOutputStream(
    ByteOutputStreamPtr byteOutputStream,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const bool initialBomAlreadyHandled) :
    _byteOutputStream{std::move(byteOutputStream)},
    _encoding{encoding},
    _effectiveEncoding{encoding.effectiveEncoding()},
    _bomMode{bomMode},
    _bomWritten{initialBomAlreadyHandled} {
    if (!_byteOutputStream) {
        throw StreamError{StreamErrorContext{
            "Failed to create the text output stream."_el, "The required byte output stream was not provided."_el}};
    }
    _bufferedByteOutputStream = dynamic_cast<BufferedByteOutputStream *>(_byteOutputStream.get());
}

auto EncodedTextOutputStream::createErrorContext() const noexcept -> StreamErrorContext {
    return _byteOutputStream->createErrorContext();
}

auto EncodedTextOutputStream::encoding() const noexcept -> StringEncoding {
    return _encoding;
}

auto EncodedTextOutputStream::effectiveEncoding() const noexcept -> StringEncoding {
    return _effectiveEncoding;
}

auto EncodedTextOutputStream::supportsPositioning() const noexcept -> bool {
    return _byteOutputStream->supportsPositioning();
}

auto EncodedTextOutputStream::position() const -> unit::ByteIndex {
    return _byteOutputStream->position();
}

auto EncodedTextOutputStream::setPosition(const unit::ByteIndex position) -> StreamPositionStatus {
    if (!supportsPositioning()) {
        return StreamPositioning::setPosition(position);
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamPositionStatus::Timeout;
    }
    const auto result = _byteOutputStream->setPosition(position);
    if (result.isSuccess()) {
        _bomWritten = !position.isZero();
    }
    return result;
}

auto EncodedTextOutputStream::movePosition(const StreamPositionOrigin origin, const unit::ByteOffset offset)
    -> StreamPositionStatus {
    if (!supportsPositioning()) {
        return StreamPositioning::movePosition(origin, offset);
    }
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamPositionStatus::Timeout;
    }
    const auto result = _byteOutputStream->movePosition(origin, offset);
    if (result.isSuccess()) {
        _bomWritten = !_byteOutputStream->position().isZero();
    }
    return result;
}

auto EncodedTextOutputStream::outputSettings() const noexcept -> const OutputStreamSettings & {
    return _byteOutputStream->outputSettings();
}

auto EncodedTextOutputStream::state() const noexcept -> StreamState {
    return _byteOutputStream->state();
}

auto EncodedTextOutputStream::isReady() const noexcept -> bool {
    return _byteOutputStream->isReady();
}

auto EncodedTextOutputStream::waitForReady() -> StreamWaitStatus {
    return _byteOutputStream->waitForReady();
}

auto EncodedTextOutputStream::flush() -> StreamWriteStatus {
    return _byteOutputStream->flush();
}

auto EncodedTextOutputStream::close() -> StreamCloseStatus {
    return _byteOutputStream->close();
}

void EncodedTextOutputStream::abort() noexcept {
    if (_byteOutputStream) {
        _byteOutputStream->abort();
    }
}

auto EncodedTextOutputStream::write(const Char character) -> StreamWriteStatus {
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamWriteStatus::Timeout;
    }
    const auto normalized = character.isValidUnicode() ? character : Char::replacement();
    const auto bomMode = bomModeForNextWrite();
    const auto result = _bufferedByteOutputStream != nullptr
        ? _bufferedByteOutputStream->writeEncodedCharacter(normalized, _encoding, bomMode)
        : _byteOutputStream->write(StringEncoder{normalized}.encode(_encoding, bomMode));
    if (result == StreamWriteStatus::Success) {
        _bomWritten = true;
    }
    return result;
}

auto EncodedTextOutputStream::write(const String &text) -> StreamWriteStatus {
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamWriteStatus::Timeout;
    }
    return writeLocked(text);
}

auto EncodedTextOutputStream::writeLocked(const String &text) -> StreamWriteStatus {
    const auto bomMode = bomModeForNextWrite();
    const auto result = _bufferedByteOutputStream != nullptr
        ? _bufferedByteOutputStream->writeEncodedText(text, _encoding, bomMode)
        : _byteOutputStream->write(StringEncoder{text}.encode(_encoding, bomMode));
    if (result == StreamWriteStatus::Success) {
        _bomWritten = true;
    }
    return result;
}

auto EncodedTextOutputStream::writeLine() -> StreamWriteStatus {
    return write(Char{U'\n'});
}

auto EncodedTextOutputStream::writeLine(const String &text) -> StreamWriteStatus {
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamWriteStatus::Timeout;
    }
    return writeLocked(String::fromJoined({text, "\n"_el}));
}

auto EncodedTextOutputStream::bomModeForNextWrite() const noexcept -> StringBomMode {
    if (_bomWritten) {
        return StringBomMode::Reject;
    }
    return _bomMode;
}

}
