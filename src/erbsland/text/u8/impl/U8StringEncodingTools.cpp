// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringEncodingTools.hpp"

#include "U8Encoding.hpp"
#include "U8Writer.hpp"

#include "../U8StringEditor.hpp"

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
#include "../../u32/impl/U32Encoding.hpp"
#include "../../u32/impl/U32Writer.hpp"

#include <cstdint>
#include <span>

namespace erbsland::text::impl {

using mem::ByteBlock;
using mem::ByteReader;
using mem::ByteWriter;
using mem::Endianness;

auto U8StringEncodingTools::encodeUtf8(const std::span<const char> data, const StringBomMode bomMode) -> ByteBlock {
    auto writer = ByteWriter{};
    auto u8Writer = U8Writer{writer};
    if (StringEncoding{StringEncoding::Utf8}.writesBom(bomMode)) {
        u8Writer.writeBom();
    }
    utf8::forEachDecodedCharacter<EncodingMode::Tolerant>(
        data, [&](const Char character) -> void { u8Writer.write(character); });
    return writer.toByteBlock();
}

auto U8StringEncodingTools::encodeUtf16(
    const std::span<const char> data, const StringEncoding encoding, const StringBomMode bomMode) -> ByteBlock {
    auto writer = ByteWriter{};
    writer.setEndianness(encoding.endianness());
    auto u16Writer = U16Writer{writer};
    if (encoding.writesBom(bomMode)) {
        u16Writer.writeBom();
    }
    utf8::forEachDecodedCharacter<EncodingMode::Tolerant>(
        data, [&](const Char character) -> void { u16Writer.write(character); });
    return writer.toByteBlock();
}

auto U8StringEncodingTools::encodeUtf32(
    const std::span<const char> data, const StringEncoding encoding, const StringBomMode bomMode) -> ByteBlock {
    auto writer = ByteWriter{};
    writer.setEndianness(encoding.endianness());
    auto u32Writer = U32Writer{writer};
    if (encoding.writesBom(bomMode)) {
        u32Writer.writeBom();
    }
    utf8::forEachDecodedCharacter<EncodingMode::Tolerant>(
        data, [&](const Char character) -> void { u32Writer.write(character); });
    return writer.toByteBlock();
}

auto U8StringEncodingTools::resolveBomLayout(
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

auto U8StringEncodingTools::decodeUtf8(const ByteBlock &data, const DecodeLayout layout, const EncodingMode mode)
    -> U8StringEditor {
    const auto sourceBytes = data.span().subspan(layout.start);
    const auto source = std::span<const char>{reinterpret_cast<const char *>(sourceBytes.data()), sourceBytes.size()};
    const auto copySource = [&source]() -> U8StringEditor {
        return U8StringEditor{U8StringSharedStorage::fromBytes(source)};
    };
    switch (mode) {
    case EncodingMode::Strict: {
        auto reader = ByteReader{data};
        reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
        utf8::forEachDecodedCharacter<EncodingMode::Strict>(reader, [](const Char) -> void {});
        return copySource();
    }
    case EncodingMode::Tolerant: {
        auto decodedSize = unit::ByteLength::zero();
        auto position = unit::ByteIndex::zero();
        auto sourceIsValid = true;
        while (position.toSizeT() < source.size()) {
            if (const auto character = utf8::tryDecodeChar(source, position); character.has_value()) {
                decodedSize += utf8::encodedLength(*character);
            } else {
                sourceIsValid = false;
                decodedSize += utf8::encodedLength(Char::replacement());
            }
        }
        if (sourceIsValid) {
            return copySource();
        }

        auto storage = U8StringSharedStorage::forSize(decodedSize.toSizeT());
        auto writer = U8Writer{std::span{storage.dataForWrite(), storage.dataSize()}};
        utf8::forEachDecodedCharacter<EncodingMode::Tolerant>(
            source, [&writer](const Char character) -> void { writer.write(character); });
        return U8StringEditor{std::move(storage)};
    }
    }
    return {};
}

auto U8StringEncodingTools::decodeUtf16(const ByteBlock &data, const DecodeLayout layout, const EncodingMode mode)
    -> U8StringEditor {
    switch (mode) {
    case EncodingMode::Strict:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf16::forEachDecodedCharacter<EncodingMode::Strict>(reader, function);
        });
    case EncodingMode::Tolerant:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf16::forEachDecodedCharacter<EncodingMode::Tolerant>(reader, function);
        });
    }
    return {};
}

auto U8StringEncodingTools::decodeUtf32(const ByteBlock &data, const DecodeLayout layout, const EncodingMode mode)
    -> U8StringEditor {
    switch (mode) {
    case EncodingMode::Strict:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf32::forEachValidatedCharacter<EncodingMode::Strict>(reader, function);
        });
    case EncodingMode::Tolerant:
        return decodeFromCharacters([&](auto function) -> void {
            auto reader = ByteReader{data};
            reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
            reader.setEndianness(layout.endianness);
            utf32::forEachValidatedCharacter<EncodingMode::Tolerant>(reader, function);
        });
    }
    return {};
}

auto U8StringEncodingTools::encode(const StringEncoding encoding, const StringBomMode bomMode) const -> ByteBlock {
    switch (encoding.toRawValue()) {
    case StringEncoding::Utf8:
        return encodeUtf8(_data.dataSpan(), bomMode);
    case StringEncoding::Utf16:
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian:
        return encodeUtf16(_data.dataSpan(), encoding, bomMode);
    case StringEncoding::Utf32:
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian:
        return encodeUtf32(_data.dataSpan(), encoding, bomMode);
    }
    return {};
}

auto U8StringEncodingTools::encodedLength(const StringEncoding encoding, const StringBomMode bomMode) const
    -> unit::ByteLength {
    auto result = encoding.bomLength(bomMode);
    utf8::forEachDecodedCharacter<EncodingMode::Tolerant>(
        _data.dataSpan(), [&](const Char character) -> void { result.addOrThrow(character.encodedBytes(encoding)); });
    return result;
}

auto U8StringEncodingTools::encodeTo(
    mem::RingBuffer &buffer, const StringEncoding encoding, const StringBomMode bomMode) const -> util::Result {
    const auto length = encodedLength(encoding, bomMode);
    if (isFailure(buffer.reserveAdditional(length))) {
        return util::Result::Failure;
    }

    auto ringWriter = mem::impl::RingBufferWriter{buffer};
    auto writer = StringEncodingWriter{ringWriter, encoding};
    if (encoding.writesBom(bomMode)) {
        writer.writeBom();
    }
    utf8::forEachDecodedCharacter<EncodingMode::Tolerant>(
        _data.dataSpan(), [&](const Char character) -> void { writer.write(character); });
    if (ringWriter.position().distanceFromZero() != length) {
        throw err::LogicError{"Encoded byte length does not match the calculated length."};
    }
    ringWriter.commit();
    return util::Result::Success;
}

auto U8StringEncodingTools::decode(
    const ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode, const EncodingMode mode)
    -> U8StringEditor {
    const auto layout = resolveBomLayout(data, encoding, bomMode);
    switch (encoding.toRawValue()) {
    case StringEncoding::Utf8:
        return decodeUtf8(data, layout, mode);
    case StringEncoding::Utf16:
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian:
        return decodeUtf16(data, layout, mode);
    case StringEncoding::Utf32:
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian:
        return decodeUtf32(data, layout, mode);
    }
    return {};
}

void U8StringEncodingTools::validate(
    const ByteBlock &data, const StringEncoding encoding, const StringBomMode bomMode) {
    const auto layout = resolveBomLayout(data, encoding, bomMode);
    auto reader = ByteReader{data};
    reader.setPosition(unit::ByteIndex::fromSizeT(layout.start));
    reader.setEndianness(layout.endianness);
    const auto consume = [](const Char) -> void {};
    auto complete = false;
    switch (encoding.toRawValue()) {
    case StringEncoding::Utf8:
        complete = utf8::forEachDecodedCharacter<EncodingMode::Strict>(reader, consume);
        break;
    case StringEncoding::Utf16:
    case StringEncoding::Utf16LittleEndian:
    case StringEncoding::Utf16BigEndian:
        complete = utf16::forEachDecodedCharacter<EncodingMode::Strict>(reader, consume);
        break;
    case StringEncoding::Utf32:
    case StringEncoding::Utf32LittleEndian:
    case StringEncoding::Utf32BigEndian:
        complete = utf32::forEachValidatedCharacter<EncodingMode::Strict>(reader, consume);
        break;
    }
    if (!complete) {
        std::terminate();
    }
}

}
