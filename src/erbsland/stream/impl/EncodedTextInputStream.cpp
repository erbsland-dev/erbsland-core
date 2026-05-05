// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EncodedTextInputStream.hpp"

#include "../../err/StreamError.hpp"
#include "../../mem/ByteBlockView.hpp"
#include "../../text/StringDecoder.hpp"

#include <utility>

namespace erbsland::stream::impl {

EncodedTextInputStream::EncodedTextInputStream(
    ByteInputStreamPtr byteInputStream,
    const text::StringEncoding encoding,
    const text::StringBomMode bomMode,
    const text::EncodingErrorMode errorMode) :
    _byteInputStream{std::move(byteInputStream)},
    _encoding{encoding},
    _effectiveEncoding{resolveEffectiveEncoding(mem::ByteBlock{}, encoding)},
    _bomMode{bomMode},
    _errorMode{errorMode} {
    if (!_byteInputStream) {
        throw err::StreamError{"Byte input stream is missing."};
    }
}

auto EncodedTextInputStream::encoding() const noexcept -> text::StringEncoding {
    return _encoding;
}

auto EncodedTextInputStream::effectiveEncoding() const noexcept -> text::StringEncoding {
    return _effectiveEncoding;
}

auto EncodedTextInputStream::isOpen() const noexcept -> bool {
    return _byteInputStream && _byteInputStream->isOpen();
}

void EncodedTextInputStream::close() {
    if (_byteInputStream) {
        _byteInputStream->close();
    }
}

auto EncodedTextInputStream::readChar() -> std::optional<text::Char> {
    load();
    const auto character = _reader.read();
    if (character.isEndOfData()) {
        return std::nullopt;
    }
    return character;
}

auto EncodedTextInputStream::read(const unit::CpLength maximum) -> std::optional<text::String> {
    load();
    return readIntoBuilder(maximum);
}

auto EncodedTextInputStream::readLine(const unit::CpLength maximum) -> std::optional<text::String> {
    load();
    if (_reader.isAtEnd()) {
        return std::nullopt;
    }
    if (maximum.isZero()) {
        return text::String{};
    }

    auto builder = text::StringBuilder{};
    auto count = unit::CpLength::zero();
    while (count < maximum && !_reader.isAtEnd()) {
        const auto character = _reader.read();
        builder.append(character);
        ++count;
        if (character == U'\n') {
            break;
        }
        if (character == U'\r') {
            if (count < maximum) {
                if (_reader.readIf(U'\n')) {
                    builder.append(U'\n');
                }
            }
            break;
        }
    }
    return builder.takeU8String();
}

auto EncodedTextInputStream::readAll(const unit::CpLength maximum) -> text::String {
    load();
    const auto result = readIntoBuilder(maximum);
    return result.value_or(text::String{});
}

void EncodedTextInputStream::load() {
    if (_loaded) {
        return;
    }
    const auto data = _byteInputStream->readAll();
    _effectiveEncoding = resolveEffectiveEncoding(data, _encoding);
    _text = text::StringDecoder{data}.decode(_encoding, _bomMode, _errorMode);
    _reader = text::StringCharReader{_text};
    _loaded = true;
}

auto EncodedTextInputStream::readIntoBuilder(const unit::CpLength maximum) -> std::optional<text::String> {
    if (_reader.isAtEnd()) {
        return std::nullopt;
    }
    if (maximum.isZero()) {
        return text::String{};
    }

    auto builder = text::StringBuilder{};
    auto count = unit::CpLength::zero();
    while (count < maximum && !_reader.isAtEnd()) {
        builder.append(_reader.read());
        ++count;
    }
    return builder.takeU8String();
}

auto EncodedTextInputStream::resolveEffectiveEncoding(
    const mem::ByteBlock &data, const text::StringEncoding encoding) noexcept -> text::StringEncoding {
    const auto view = mem::ByteBlockView{data};
    if (view.startsWith({mem::Byte{0xFFU}, mem::Byte{0xFEU}, mem::Byte{0x00U}, mem::Byte{0x00U}})) {
        return text::StringEncoding::Utf32LittleEndian;
    }
    if (view.startsWith({mem::Byte{0x00U}, mem::Byte{0x00U}, mem::Byte{0xFEU}, mem::Byte{0xFFU}})) {
        return text::StringEncoding::Utf32BigEndian;
    }
    if (view.startsWith({mem::Byte{0xFFU}, mem::Byte{0xFEU}})) {
        return text::StringEncoding::Utf16LittleEndian;
    }
    if (view.startsWith({mem::Byte{0xFEU}, mem::Byte{0xFFU}})) {
        return text::StringEncoding::Utf16BigEndian;
    }

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
