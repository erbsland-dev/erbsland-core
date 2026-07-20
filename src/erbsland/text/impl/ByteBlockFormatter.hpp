// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringAppendTools.hpp"

#include "../ByteFormat.hpp"
#include "../IntegerFormat.hpp"
#include "../String.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::text::impl {

/// Stateful renderer for byte-block output items.
/// @tested{ByteFormatTest AnyStringBuilderStreamTest}
class ByteBlockFormatter final {
public:
    /// Create a renderer.
    ByteBlockFormatter(StringAppendTools &sink, const mem::ByteBlock &bytes, const ByteFormat &format);

public:
    /// Render the configured byte block.
    /// @return The number of code points appended to the sink.
    auto format() -> unit::CpLength;

private:
    [[nodiscard]] auto effectiveTruncateMode() const noexcept -> TruncateMode;
    void appendByteRange(unit::ByteIndex begin, unit::ByteIndex end, unit::ByteLength itemCount);
    void appendTextItem(const String &text, unit::ByteIndex sourceIndex, bool lastItem);
    void appendText(const String &text);
    void beginItem(unit::ByteIndex sourceIndex);
    void finishItem(bool lastItem);

private:
    StringAppendTools &_sink;
    const mem::ByteBlock &_bytes;
    const ByteFormat &_format;
    IntegerFormat _byteFormat;
    IntegerFormat _offsetFormat;
    unit::ByteLength _currentColumn;
    unit::ByteLength _currentGroupByte;
    unit::ElementCount _currentLine;
    unit::ByteIndex _itemIndex;
    unit::CpLength _appendedLength;
};

/// Emit a formatted byte block as decoded characters.
/// @param sink The decoded-character sink.
/// @param bytes The bytes to format.
/// @param format The byte format to apply.
/// @return The number of code points appended to the sink.
/// @tested{ByteFormatTest AnyStringBuilderStreamTest}
auto formatByteBlock(StringAppendTools &sink, const mem::ByteBlock &bytes, const ByteFormat &format) -> unit::CpLength;

}
