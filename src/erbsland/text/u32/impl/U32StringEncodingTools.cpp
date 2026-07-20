// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringEncodingTools.hpp"

#include "U32Encoding.hpp"
#include "U32Writer.hpp"

#include "../U32StringEditor.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteReader.hpp"
#include "../../../mem/ByteWriter.hpp"
#include "../../../mem/impl/RingBufferWriter.hpp"
#include "../../../mem/RingBuffer.hpp"
#include "../../Char.hpp"
#include "../../impl/StringEncodingWriter.hpp"
#include "../../impl/ThrowHelper.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u16/impl/U16Writer.hpp"
#include "../../u8/impl/U8Encoding.hpp"
#include "../../u8/impl/U8Writer.hpp"

#include <span>

namespace erbsland::text::impl {

using mem::ByteBlock;
using mem::ByteReader;
using mem::ByteWriter;
using mem::Endianness;

auto U32StringEncodingTools::encodeUtf8(
    const std::span<const char32_t> data, const StringBomMode bomMode, const EncodingErrorMode errorMode) -> ByteBlock {
    auto writer = ByteWriter{};
    auto u8Writer = U8Writer{writer};
    if (StringEncoding{StringEncoding::Utf8}.writesBom(bomMode)) {
        u8Writer.writeBom();
    }
    utf32::forEachDecodedCharacter(data, errorMode, [&](const Char character) -> void { u8Writer.write(character); });
    return writer.toByteBlock();
}

auto U32StringEncodingTools::encodeUtf16(
    const std::span<const char32_t> data,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> ByteBlock {
    auto writer = ByteWriter{};
    writer.setEndianness(encoding.endianness());
    auto u16Writer = U16Writer{writer};
    if (encoding.writesBom(bomMode)) {
        u16Writer.writeBom();
    }
    utf32::forEachDecodedCharacter(data, errorMode, [&](const Char character) -> void { u16Writer.write(character); });
    return writer.toByteBlock();
}

auto U32StringEncodingTools::encodeUtf32(
    const std::span<const char32_t> data,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> ByteBlock {
    auto writer = ByteWriter{};
    writer.setEndianness(encoding.endianness());
    auto u32Writer = U32Writer{writer};
    if (encoding.writesBom(bomMode)) {
        u32Writer.writeBom();
    }
    utf32::forEachDecodedCharacter(data, errorMode, [&](const Char character) -> void { u32Writer.write(character); });
    return writer.toByteBlock();
}

auto U32StringEncodingTools::resolveBomLayout(
    const ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode) -> DecodeLayout {
    const auto hasUtf32LeBom =
        data.startsWith(StringEncoding{StringEncoding::Utf32LittleEndian}.bomBytes(StringBomMode::Require));
    const auto hasUtf32BeBom =
        data.startsWith(StringEncoding{StringEncoding::Utf32BigEndian}.bomBytes(StringBomMode::Require));
    const auto hasUtf8Bom = data.startsWith(StringEncoding{StringEncoding::Utf8}.bomBytes(StringBomMode::Require));
    const auto hasUtf16LeBom = !hasUtf32LeBom &&
        data.startsWith(StringEncoding{StringEncoding::Utf16LittleEndian}.bomBytes(StringBomMode::Require));
    const auto hasUtf16BeBom =
        data.startsWith(StringEncoding{StringEncoding::Utf16BigEndian}.bomBytes(StringBomMode::Require));
    const auto hasBom = hasUtf32LeBom || hasUtf32BeBom || hasUtf8Bom || hasUtf16LeBom || hasUtf16BeBom;
    if (!hasBom) {
        if (bomMode == StringBomMode::Require) {
            throwEncodingError("Missing byte order mark");
        }
        return DecodeLayout{.endianness = encoding.endianness(), .start = 0U};
    }
    if (bomMode == StringBomMode::Reject) {
        throwEncodingError("Unexpected byte order mark");
    }

    if (hasUtf8Bom) {
        if (encoding != StringEncoding::Utf8) {
            throwEncodingError("Unexpected UTF-8 byte order mark");
        }
        return DecodeLayout{
            .endianness = Endianness::Little,
            .start = StringEncoding{StringEncoding::Utf8}.bomLength(StringBomMode::Require).toSizeT()};
    }
    if (hasUtf16LeBom) {
        if (!encoding.isUtf16() || encoding == StringEncoding::Utf16BigEndian) {
            throwEncodingError("Unexpected UTF-16 little endian byte order mark");
        }
        return DecodeLayout{
            .endianness = Endianness::Little,
            .start = StringEncoding{StringEncoding::Utf16}.bomLength(StringBomMode::Require).toSizeT()};
    }
    if (hasUtf16BeBom) {
        if (!encoding.isUtf16() || encoding == StringEncoding::Utf16LittleEndian) {
            throwEncodingError("Unexpected UTF-16 big endian byte order mark");
        }
        return DecodeLayout{
            .endianness = Endianness::Big,
            .start = StringEncoding{StringEncoding::Utf16}.bomLength(StringBomMode::Require).toSizeT()};
    }
    if (hasUtf32LeBom) {
        if (!encoding.isUtf32() || encoding == StringEncoding::Utf32BigEndian) {
            throwEncodingError("Unexpected UTF-32 little endian byte order mark");
        }
        return DecodeLayout{
            .endianness = Endianness::Little,
            .start = StringEncoding{StringEncoding::Utf32}.bomLength(StringBomMode::Require).toSizeT()};
    }
    if (hasUtf32BeBom) {
        if (!encoding.isUtf32() || encoding == StringEncoding::Utf32LittleEndian) {
            throwEncodingError("Unexpected UTF-32 big endian byte order mark");
        }
        return DecodeLayout{
            .endianness = Endianness::Big,
            .start = StringEncoding{StringEncoding::Utf32}.bomLength(StringBomMode::Require).toSizeT()};
    }
    return DecodeLayout{.endianness = encoding.endianness(), .start = 0U};
}

auto U32StringEncodingTools::decodeUtf8(
    const ByteBlock &data, const DecodeLayout layout, const EncodingErrorMode errorMode) -> U32StringEditor {
    switch (errorMode) {
    case EncodingErrorMode::Throw:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            utf8::forEachDecodedCharacter<EncodingErrorMode::Throw>(reader, function);
        });
    case EncodingErrorMode::Ignore:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            utf8::forEachDecodedCharacter<EncodingErrorMode::Ignore>(reader, function);
        });
    case EncodingErrorMode::Replace:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            utf8::forEachDecodedCharacter<EncodingErrorMode::Replace>(reader, function);
        });
    }
    return {};
}

auto U32StringEncodingTools::decodeUtf16(
    const ByteBlock &data, const DecodeLayout layout, const EncodingErrorMode errorMode) -> U32StringEditor {
    switch (errorMode) {
    case EncodingErrorMode::Throw:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf16::forEachDecodedCharacter<EncodingErrorMode::Throw>(reader, function);
        });
    case EncodingErrorMode::Ignore:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf16::forEachDecodedCharacter<EncodingErrorMode::Ignore>(reader, function);
        });
    case EncodingErrorMode::Replace:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf16::forEachDecodedCharacter<EncodingErrorMode::Replace>(reader, function);
        });
    }
    return {};
}

auto U32StringEncodingTools::decodeUtf32(
    const ByteBlock &data, const DecodeLayout layout, const EncodingErrorMode errorMode) -> U32StringEditor {
    switch (errorMode) {
    case EncodingErrorMode::Throw:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf32::forEachValidatedCharacter<EncodingErrorMode::Throw>(reader, function);
        });
    case EncodingErrorMode::Ignore:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf32::forEachValidatedCharacter<EncodingErrorMode::Ignore>(reader, function);
        });
    case EncodingErrorMode::Replace:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf32::forEachValidatedCharacter<EncodingErrorMode::Replace>(reader, function);
        });
    }
    return {};
}

auto U32StringEncodingTools::encode(
    const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode) const -> ByteBlock {
    switch (encoding.toRawValue()) {
    case StringEncoding::Utf8:
        return encodeUtf8(_data.dataSpan(), bomMode, errorMode);
    case StringEncoding::Utf16:
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian:
        return encodeUtf16(_data.dataSpan(), encoding, bomMode, errorMode);
    case StringEncoding::Utf32:
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian:
        return encodeUtf32(_data.dataSpan(), encoding, bomMode, errorMode);
    }
    return {};
}

auto U32StringEncodingTools::encodedLength(
    const StringEncoding encoding, const StringBomMode bomMode, const EncodingErrorMode errorMode) const
    -> unit::ByteLength {
    auto result = encoding.bomLength(bomMode);
    utf32::forEachDecodedCharacter(_data.dataSpan(), errorMode, [&](const Char character) -> void {
        result.addOrThrow(character.encodedBytes(encoding));
    });
    return result;
}

auto U32StringEncodingTools::encodeTo(
    mem::RingBuffer &buffer,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) const -> util::Result {
    const auto length = encodedLength(encoding, bomMode, errorMode);
    if (isFailure(buffer.reserveAdditional(length))) {
        return util::Result::Failure;
    }

    auto ringWriter = mem::impl::RingBufferWriter{buffer};
    auto writer = StringEncodingWriter{ringWriter, encoding};
    if (encoding.writesBom(bomMode)) {
        writer.writeBom();
    }
    utf32::forEachDecodedCharacter(
        _data.dataSpan(), errorMode, [&](const Char character) -> void { writer.write(character); });
    if (ringWriter.position().distanceFromZero() != length) {
        throw err::LogicError{"Encoded byte length does not match the calculated length."};
    }
    ringWriter.commit();
    return util::Result::Success;
}

auto U32StringEncodingTools::decode(
    const ByteBlock &data,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) -> U32StringEditor {
    const auto layout = resolveBomLayout(data, encoding, bomMode);
    switch (encoding.toRawValue()) {
    case StringEncoding::Utf8:
        return decodeUtf8(data, layout, errorMode);
    case StringEncoding::Utf16:
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian:
        return decodeUtf16(data, layout, errorMode);
    case StringEncoding::Utf32:
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian:
        return decodeUtf32(data, layout, errorMode);
    }
    return {};
}

}
