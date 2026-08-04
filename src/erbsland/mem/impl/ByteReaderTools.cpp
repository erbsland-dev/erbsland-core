// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteReaderTools.hpp"

#include "ByteComparisonTools.hpp"
#include "ByteDataView.hpp"
#include "ByteReadTools.hpp"
#include "ByteTextTools.hpp"

#include "../ByteReader.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../err/OverflowError.hpp"
#include "../../err/ParseError.hpp"
#include "../../math/impl/Throw.hpp"
#include "../../text/StringDecoder.hpp"

namespace erbsland::mem::impl {

using namespace text::literals;

auto ByteReaderTools::readTextOrThrow(const ByteTextOptions &options) -> text::String {
    auto cursor = _reader.position();
    auto result = text::String{};
    if (options.format() == ByteTextFormat::PaddedField) {
        if (!options.length().isFinite()) {
            throw err::OutOfRangeError("A padded text field requires a finite byte length"_el);
        }
        result = readTextFromFieldOrThrow(options, options.length(), cursor);
    } else {
        result = readDynamicTextOrThrow(options, cursor);
    }
    _reader._position = cursor;
    return result;
}

auto ByteReaderTools::readTextFromFieldOrThrow(
    const ByteTextOptions &options, const unit::ByteLength fieldLength, unit::ByteIndex &cursor) -> text::String {
    if (!canRead(cursor, fieldLength)) {
        throw err::OutOfRangeError("The padded text field is incomplete"_el);
    }
    const auto fieldEnd = cursor.toSizeT() + fieldLength.toSizeT();
    const auto textTools = ByteTextTools{options};
    const auto unitSize = textTools.unitSize();
    const auto encodedMark = textTools.encodedEndMark();

    auto payloadLength = unit::ByteLength{};
    if (options.countFormat().has_value()) {
        const auto count = readCountOrThrow(*options.countFormat(), cursor);
        payloadLength = textTools.byteLengthFromCountOrThrow(count);
        const auto requiredLength = payloadLength.added(encodedMark.length());
        if (!requiredLength.isFinite() || cursor.toSizeT() > fieldEnd ||
            requiredLength.toSizeT() > fieldEnd - cursor.toSizeT()) {
            throw err::OutOfRangeError("Text count exceeds the field length"_el);
        }
    } else if (!encodedMark.isEmpty()) {
        const auto searchLength = unit::ByteLength::fromSizeT(fieldEnd - cursor.toSizeT());
        const auto searchView = ByteDataView{_reader.dataView().dataSpan(), unit::ByteRange{cursor, searchLength}};
        const auto markPosition = ByteComparisonTools{searchView}.find(ByteDataView{encodedMark.span()});
        if (markPosition.isNoIndex() || markPosition.toSizeT() % unitSize != 0U) {
            throw err::ParseError{"Text end mark is missing"_el};
        }
        payloadLength = markPosition.distanceFromZero();
    } else {
        payloadLength = unit::ByteLength::fromSizeT(fieldEnd - cursor.toSizeT());
    }

    const auto payload = readBytesOrThrow(payloadLength, cursor);
    if (!encodedMark.isEmpty() && readBytesOrThrow(encodedMark.length(), cursor) != encodedMark) {
        throw err::ParseError{"Text end mark does not match"_el};
    }
    const auto result = text::StringDecoder{payload}.decode(options.encoding(), text::StringBomMode::Reject);
    cursor = unit::ByteIndex::fromSizeT(fieldEnd);
    return result;
}

auto ByteReaderTools::readDynamicTextOrThrow(const ByteTextOptions &options, unit::ByteIndex &cursor) -> text::String {
    const auto textTools = ByteTextTools{options};
    const auto unitSize = textTools.unitSize();
    const auto encodedMark = textTools.encodedEndMark();
    auto payloadLength = options.length();
    if (options.countFormat().has_value()) {
        const auto count = readCountOrThrow(*options.countFormat(), cursor);
        payloadLength = textTools.byteLengthFromCountOrThrow(count);
        if (options.length().isFinite() && payloadLength > options.length()) {
            throw err::OutOfRangeError("Text count exceeds the configured maximum length"_el);
        }
    } else if (!encodedMark.isEmpty()) {
        const auto remainingLength = unit::ByteLength::fromSizeT(_reader.length().toSizeT() - cursor.toSizeT());
        const auto maximumFrameLength =
            options.length().isFinite() ? options.length().added(encodedMark.length()) : remainingLength;
        const auto searchLength = maximumFrameLength < remainingLength ? maximumFrameLength : remainingLength;
        const auto searchView = ByteDataView{_reader.dataView().dataSpan(), unit::ByteRange{cursor, searchLength}};
        const auto markPosition = ByteComparisonTools{searchView}.find(ByteDataView{encodedMark.span()});
        if (markPosition.isNoIndex() || markPosition.toSizeT() % unitSize != 0U) {
            throw err::ParseError{"Text end mark is missing"_el};
        }
        payloadLength = markPosition.distanceFromZero();
    } else if (!payloadLength.isFinite()) {
        throw err::OutOfRangeError("Unframed dynamic text requires an exact finite length"_el);
    }

    const auto payload = readBytesOrThrow(payloadLength, cursor);
    if (!encodedMark.isEmpty() && readBytesOrThrow(encodedMark.length(), cursor) != encodedMark) {
        throw err::ParseError{"Text end mark does not match"_el};
    }
    return text::StringDecoder{payload}.decode(options.encoding(), text::StringBomMode::Reject);
}

auto ByteReaderTools::readCountOrThrow(const ByteIntegerFormat format, unit::ByteIndex &cursor) const -> uint64_t {
    const auto value = ByteReadTools{_reader.dataView()}.getIntegerOrThrow(cursor, format, _reader.endianness());
    if (value.isNegative) {
        throw err::OverflowError("A negative text count does not fit an unsigned value"_el);
    }
    cursor = cursor + value.byteLength;
    return value.magnitude;
}

auto ByteReaderTools::readBytesOrThrow(const unit::ByteLength length, unit::ByteIndex &cursor) const -> ByteBlock {
    if (!canRead(cursor, length)) {
        throw err::OutOfRangeError("Not enough bytes to read the requested byte block"_el);
    }
    const auto result = _reader._block.slice(cursor, length);
    cursor = cursor + length;
    return result;
}

auto ByteReaderTools::canRead(const unit::ByteIndex cursor, const unit::ByteLength length) const noexcept -> bool {
    if (!cursor.isValid() || !length.isFinite()) {
        return false;
    }
    const auto availableLength = _reader.length().toSizeT();
    return cursor.toSizeT() <= availableLength && length.toSizeT() <= availableLength - cursor.toSizeT();
}

}
