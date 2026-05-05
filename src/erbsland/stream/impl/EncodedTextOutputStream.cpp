// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EncodedTextOutputStream.hpp"

#include "../../err/StreamError.hpp"
#include "../../mem/ByteBlockView.hpp"
#include "../../text/StringEncoder.hpp"

#include <string_view>
#include <utility>

namespace erbsland::stream::impl {

EncodedTextOutputStream::EncodedTextOutputStream(
    ByteOutputStreamPtr byteOutputStream, const text::StringEncoding encoding, const text::StringBomMode bomMode) :
    _byteOutputStream{std::move(byteOutputStream)},
    _encoding{encoding},
    _effectiveEncoding{effectiveEncodingFor(encoding)},
    _bomMode{bomMode} {
    if (!_byteOutputStream) {
        throw err::StreamError{"Byte output stream is missing."};
    }
}

auto EncodedTextOutputStream::encoding() const noexcept -> text::StringEncoding {
    return _encoding;
}

auto EncodedTextOutputStream::effectiveEncoding() const noexcept -> text::StringEncoding {
    return _effectiveEncoding;
}

auto EncodedTextOutputStream::isOpen() const noexcept -> bool {
    return _byteOutputStream && _byteOutputStream->isOpen();
}

void EncodedTextOutputStream::flush() {
    _byteOutputStream->flush();
}

void EncodedTextOutputStream::close() {
    _byteOutputStream->close();
}

void EncodedTextOutputStream::write(const text::Char character) {
    const auto text = text::String::fromCharacter(character.isValidUnicode() ? character : text::Char::replacement());
    write(text);
}

void EncodedTextOutputStream::write(const text::StringView &text) {
    const auto data = text::StringEncoder{text}.encode(_encoding, bomModeForNextWrite());
    _byteOutputStream->write(data);
}

void EncodedTextOutputStream::writeLine() {
    write(text::Char{U'\n'});
}

void EncodedTextOutputStream::writeLine(const text::StringView &text) {
    write(text);
    writeLine();
}

auto EncodedTextOutputStream::bomModeForNextWrite() noexcept -> text::StringBomMode {
    if (_bomWritten) {
        return text::StringBomMode::Reject;
    }
    _bomWritten = true;
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
