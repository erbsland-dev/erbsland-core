// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlockFormatter.hpp"

#include "IntegerAppend.hpp"

namespace erbsland::text::impl {

using namespace unit;

ByteBlockFormatter::ByteBlockFormatter(StringAppendTools &sink, const mem::ByteBlock &bytes, const ByteFormat &format) :
    _sink{sink},
    _bytes{bytes},
    _format{format},
    _byteFormat{IntegerFormat::hexadecimal()
            .setFlags(IntegerFormatFlag::ZeroFill)
            .setLetterCase(format.letterCase())
            .setFieldWidth(CpCount{2U})},
    _offsetFormat{IntegerFormat::hexadecimal()
            .setFlags(IntegerFormatFlag::ZeroFill)
            .setLetterCase(format.letterCase())
            .setFieldWidth(CpCount{8U})} {
}

auto ByteBlockFormatter::format() -> CpCount {
    if (_bytes.isEmpty() || _format.maximum().isZero()) {
        return _appendedLength;
    }
    const auto byteCount = _bytes.length();
    if (_format.maximum().isInfinite() || _format.maximum() >= _bytes.length()) {
        appendByteRange(ByteIndex::zero(), ByteIndex::end(byteCount), byteCount);
        return _appendedLength;
    }

    const auto maximum = _format.maximum();
    const auto hasEllipsis = !_format.ellipsis().isEmpty();
    auto ellipsisCount = ByteCount::zero();
    if (hasEllipsis) {
        ellipsisCount = ByteCount::one();
    }
    const auto retainedCount = maximum - ellipsisCount;
    const auto truncateMode = effectiveTruncateMode();
    auto prefixCount = ByteCount::zero();
    auto suffixCount = ByteCount::zero();
    switch (truncateMode) {
    case TruncateMode::Begin:
        suffixCount = retainedCount;
        break;
    case TruncateMode::Middle:
        prefixCount = (retainedCount + ByteCount{1U}) / 2U;
        suffixCount = retainedCount - prefixCount;
        break;
    case TruncateMode::End:
    default:
        prefixCount = retainedCount;
        break;
    }

    const auto itemCount = maximum;
    appendByteRange(ByteIndex::zero(), ByteIndex::end(prefixCount), itemCount);
    if (hasEllipsis) {
        const auto sourceIndex =
            ByteIndex::end(truncateMode == TruncateMode::Begin ? byteCount - suffixCount : prefixCount);
        ++_itemIndex;
        appendTextItem(_format.ellipsis(), sourceIndex, !_itemIndex.isWithin(itemCount));
    }
    appendByteRange(ByteIndex::end(byteCount - suffixCount), ByteIndex::end(byteCount), itemCount);
    return _appendedLength;
}

auto ByteBlockFormatter::effectiveTruncateMode() const noexcept -> TruncateMode {
    const auto unsupportedFlags = _format.flags() & ~ByteFormatFlags{ByteFormatFlag::Separator};
    if (unsupportedFlags.hasAny()) {
        return TruncateMode::End;
    }
    return _format.truncateMode();
}

void ByteBlockFormatter::appendByteRange(const ByteIndex begin, const ByteIndex end, const ByteCount itemCount) {
    for (auto index = begin; index < end; ++index) {
        beginItem(index);
        _appendedLength += appendInteger(_sink, _bytes.get(index).toUInt8(), _byteFormat);
        ++_itemIndex;
        finishItem(!_itemIndex.isWithin(itemCount));
    }
}

void ByteBlockFormatter::appendTextItem(const String &text, const ByteIndex sourceIndex, const bool lastItem) {
    beginItem(sourceIndex);
    appendText(text);
    finishItem(lastItem);
}

void ByteBlockFormatter::appendText(const String &text) {
    _appendedLength += _sink.append(text);
}

void ByteBlockFormatter::beginItem(const ByteIndex sourceIndex) {
    if (_currentColumn.isZero()) {
        if (_format.hasFlag(ByteFormatFlag::Lines)) {
            appendText(_format.linePrefix());
        }
        if (_format.hasFlag(ByteFormatFlag::Offset)) {
            const auto offset = _format.startOffset() + sourceIndex.distanceFromZero();
            _appendedLength += appendInteger(_sink, offset.toRawValue(), _offsetFormat);
            appendText(_format.offsetSeparator());
        }
    } else if (_format.hasFlag(ByteFormatFlag::Separator)) {
        if (!_format.hasFlag(ByteFormatFlag::ByteGroups) || _currentGroupByte >= _format.byteGroupSize()) {
            appendText(_format.byteSeparator());
            _currentGroupByte = ByteCount::zero();
        }
    }
}

void ByteBlockFormatter::finishItem(const bool lastItem) {
    ++_currentColumn;
    ++_currentGroupByte;
    if (_format.hasFlag(ByteFormatFlag::Lines) && (_currentColumn >= _format.bytesPerLine() || lastItem)) {
        _currentColumn = ByteCount::zero();
        _currentGroupByte = ByteCount::zero();
        ++_currentLine;
        appendText(_format.lineSuffix());
        if (_format.hasFlag(ByteFormatFlag::LineGroups) && _currentLine >= _format.lineGroupSize() && !lastItem) {
            appendText(_format.lineSuffix());
            _currentLine = ItemCount::zero();
        }
    }
}

auto formatByteBlock(StringAppendTools &sink, const mem::ByteBlock &bytes, const ByteFormat &format) -> CpCount {
    return ByteBlockFormatter{sink, bytes, format}.format();
}

}
