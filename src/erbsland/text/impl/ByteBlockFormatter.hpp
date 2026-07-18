// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerAppend.hpp"

#include "../ByteFormat.hpp"
#include "../String.hpp"
#include "../u8/U8StringConstIterator.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteReader.hpp"

namespace erbsland::text::impl {

/// Append decoded text to a character sink.
/// @param sink The decoded-character sink.
/// @param text The text to append.
/// @tested{ByteFormatTest}
template <typename tSink>
void appendByteBlockText(tSink &sink, const String &text) {
    for (const auto character : text) {
        sink.append(character);
    }
}

/// Emit a formatted byte block as decoded characters.
/// @param sink The decoded-character sink.
/// @param bytes The bytes to format.
/// @param format The byte format to apply.
/// @tested{ByteFormatTest}
template <typename tSink>
void formatByteBlock(tSink &sink, const mem::ByteBlock &bytes, const ByteFormat &format) {
    if (bytes.isEmpty()) {
        return;
    }

    auto byteFormat = IntegerFormat::hexadecimal()
                          .setFlags(IntegerFormatFlag::ZeroFill)
                          .setLetterCase(format.letterCase())
                          .setFieldWidth(unit::CpLength{2U});
    auto offsetFormat = IntegerFormat::hexadecimal()
                            .setFlags(IntegerFormatFlag::ZeroFill)
                            .setLetterCase(format.letterCase())
                            .setFieldWidth(unit::CpLength{8U});
    auto offset = format.startOffset();
    auto currentColumn = unit::ByteLength::zero();
    auto currentGroupByte = unit::ByteLength::zero();
    auto currentLine = unit::ElementCount::zero();
    const auto bytesPerLine = format.bytesPerLine();
    const auto byteGroupSize = format.byteGroupSize();
    const auto lineGroupSize = format.lineGroupSize();

    auto byteReader = mem::ByteReader{bytes};
    while (!byteReader.isAtEnd()) {
        const auto currentByte = byteReader.readByte();
        const auto lastByte = byteReader.isAtEnd();

        if (currentColumn.isZero()) {
            if (format.hasFlag(ByteFormatFlag::Lines)) {
                appendByteBlockText(sink, format.linePrefix());
            }
            if (format.hasFlag(ByteFormatFlag::Offset)) {
                appendInteger(sink, offset.toRawValue(), offsetFormat);
                appendByteBlockText(sink, format.offsetSeparator());
            }
        } else if (format.hasFlag(ByteFormatFlag::Separator)) {
            if (!format.hasFlag(ByteFormatFlag::ByteGroups) || currentGroupByte >= byteGroupSize) {
                appendByteBlockText(sink, format.byteSeparator());
                currentGroupByte = unit::ByteLength::zero();
            }
        }

        appendInteger(sink, currentByte.toUInt8(), byteFormat);
        ++offset;
        ++currentColumn;
        ++currentGroupByte;

        if (format.hasFlag(ByteFormatFlag::Lines) && (currentColumn >= bytesPerLine || lastByte)) {
            currentColumn = unit::ByteLength::zero();
            currentGroupByte = unit::ByteLength::zero();
            ++currentLine;
            appendByteBlockText(sink, format.lineSuffix());
            if (format.hasFlag(ByteFormatFlag::LineGroups) && currentLine >= lineGroupSize && !lastByte) {
                appendByteBlockText(sink, format.lineSuffix());
                currentLine = unit::ElementCount::zero();
            }
        }
    }
}

}
