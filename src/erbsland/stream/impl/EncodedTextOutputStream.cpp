// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EncodedTextOutputStream.hpp"

#include "BufferedByteOutputStream.hpp"

#include "../../mem/ByteBlockView.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringBuilder.hpp"
#include "../../text/StringEncoder.hpp"

#include <exception>
#include <string_view>
#include <utility>

namespace erbsland::stream::impl {

using namespace text::literals;

EncodedTextOutputStream::EncodedTextOutputStream(
    ByteOutputStreamPtr byteOutputStream,
    const text::StringEncoding encoding,
    const text::StringBomMode bomMode,
    const text::EncodingErrorMode errorMode,
    const bool initialBomAlreadyHandled) :
    _byteOutputStream{std::move(byteOutputStream)},
    _encoding{encoding},
    _effectiveEncoding{effectiveEncodingFor(encoding)},
    _bomMode{bomMode},
    _errorMode{errorMode},
    _bomWritten{initialBomAlreadyHandled} {
    if (!_byteOutputStream) {
        throw StreamError{StreamErrorContext{
            "Failed to create the text output stream."_el, "The required byte output stream was not provided."_el}};
    }
}

auto EncodedTextOutputStream::createErrorContext() const noexcept -> StreamErrorContext {
    return _byteOutputStream->createErrorContext();
}

auto EncodedTextOutputStream::encoding() const noexcept -> text::StringEncoding {
    return _encoding;
}

auto EncodedTextOutputStream::effectiveEncoding() const noexcept -> text::StringEncoding {
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

auto EncodedTextOutputStream::write(const text::Char character) -> StreamWriteStatus {
    const auto text = text::String::fromCharacter(character.isValidUnicode() ? character : text::Char::replacement());
    return write(text);
}

auto EncodedTextOutputStream::write(const text::StringView &text) -> StreamWriteStatus {
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamWriteStatus::Timeout;
    }
    return writeLocked(text);
}

auto EncodedTextOutputStream::writeLocked(const text::StringView &text) -> StreamWriteStatus {
    const auto bomMode = bomModeForNextWrite();
    auto result = StreamWriteStatus::Timeout;
    if (const auto buffered = std::dynamic_pointer_cast<BufferedByteOutputStream>(_byteOutputStream)) {
        result = buffered->writeEncodedText(text, _encoding, bomMode, _errorMode);
    } else {
        const auto data = text::StringEncoder{text}.encode(_encoding, bomMode, _errorMode);
        result = _byteOutputStream->write(data);
    }
    if (result == StreamWriteStatus::Success) {
        _bomWritten = true;
    }
    return result;
}

auto EncodedTextOutputStream::writeLine() -> StreamWriteStatus {
    return write(text::String::fromCharacter(U'\n'));
}

auto EncodedTextOutputStream::writeLine(const text::StringView &text) -> StreamWriteStatus {
    const auto lock = std::unique_lock{_mutex, std::try_to_lock};
    if (!lock.owns_lock()) {
        return StreamWriteStatus::Timeout;
    }
    auto builder = text::StringBuilder{};
    builder.append(text);
    builder.append(U'\n');
    return writeLocked(builder.takeString());
}

auto EncodedTextOutputStream::bomModeForNextWrite() const noexcept -> text::StringBomMode {
    if (_bomWritten) {
        return text::StringBomMode::Reject;
    }
    return _bomMode;
}

auto EncodedTextOutputStream::effectiveEncodingFor(const text::StringEncoding encoding) noexcept
    -> text::StringEncoding {
    switch (encoding) {
    case text::StringEncoding::Utf16:
        return text::StringEncoding::Utf16LittleEndian;
    case text::StringEncoding::Utf32:
        return text::StringEncoding::Utf32LittleEndian;
    default:
        return encoding;
    }
}

}
