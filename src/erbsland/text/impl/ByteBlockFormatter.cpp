// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteBlockFormatter.hpp"

#include "IntegerAppend.hpp"

namespace erbsland::text::impl {

ByteBlockFormatter::ByteBlockFormatter(StringAppendTools &sink, const mem::ByteBlock &bytes, const ByteFormat &format) :
    _sink{sink},
    _bytes{bytes},
    _format{format},
    _byteFormat{IntegerFormat::hexadecimal()
            .setFlags(IntegerFormatFlag::ZeroFill)
            .setLetterCase(format.letterCase())
            .setFieldWidth(unit::CpLength{2U})},
    _offsetFormat{IntegerFormat::hexadecimal()
            .setFlags(IntegerFormatFlag::ZeroFill)
            .setLetterCase(format.letterCase())
            .setFieldWidth(unit::CpLength{8U})} {
}

auto ByteBlockFormatter::format() -> unit::CpLength {
    if (_bytes.isEmpty() || _format.maximum().isZero()) {
        return _appendedLength;
    }
    const auto byteCount = _bytes.length();
    if (_format.maximum().isInfinite() || _format.maximum() >= _bytes.length()) {
        appendByteRange(unit::ByteIndex::zero(), unit::ByteIndex::end(byteCount), byteCount);
        return _appendedLength;
    }

    const auto maximum = _format.maximum();
    const auto hasEllipsis = !_format.ellipsis().isEmpty();
    const auto retainedCount = maximum - unit::ByteLength{hasEllipsis ? 1U : 0U};
    const auto truncateMode = effectiveTruncateMode();
    auto prefixCount = unit::ByteLength::zero();
    auto suffixCount = unit::ByteLength::zero();
    switch (truncateMode) {
    case TruncateMode::Begin:
        suffixCount = retainedCount;
        break;
    case TruncateMode::Middle:
        prefixCount = (retainedCount + unit::ByteLength{1U}) / 2U;
        suffixCount = retainedCount - prefixCount;
        break;
    case TruncateMode::End:
    default:
        prefixCount = retainedCount;
        break;
    }

    const auto itemCount = retainedCount + unit::ByteLength{hasEllipsis ? 1U : 0U};
    appendByteRange(unit::ByteIndex::zero(), unit::ByteIndex::end(prefixCount), itemCount);
    if (hasEllipsis) {
        const auto sourceIndex =
            unit::ByteIndex::end(truncateMode == TruncateMode::Begin ? byteCount - suffixCount : prefixCount);
        ++_itemIndex;
        appendTextItem(_format.ellipsis(), sourceIndex, !_itemIndex.isWithin(itemCount));
    }
    appendByteRange(unit::ByteIndex::end(byteCount - suffixCount), unit::ByteIndex::end(byteCount), itemCount);
    return _appendedLength;
}

auto ByteBlockFormatter::effectiveTruncateMode() const noexcept -> TruncateMode {
    const auto unsupportedFlags = _format.flags() & ~ByteFormatFlags{ByteFormatFlag::Separator};
    if (unsupportedFlags.hasAny()) {
        return TruncateMode::End;
    }
    return _format.truncateMode();
}

void ByteBlockFormatter::appendByteRange(
    const unit::ByteIndex begin, const unit::ByteIndex end, const unit::ByteLength itemCount) {
    for (auto index = begin; index < end; ++index) {
        beginItem(index);
        _appendedLength += appendInteger(_sink, _bytes.get(index).toUInt8(), _byteFormat);
        ++_itemIndex;
        finishItem(!_itemIndex.isWithin(itemCount));
    }
}

void ByteBlockFormatter::appendTextItem(const String &text, const unit::ByteIndex sourceIndex, const bool lastItem) {
    beginItem(sourceIndex);
    appendText(text);
    finishItem(lastItem);
}

void ByteBlockFormatter::appendText(const String &text) {
    _appendedLength += _sink.append(text);
}

void ByteBlockFormatter::beginItem(const unit::ByteIndex sourceIndex) {
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
            _currentGroupByte = unit::ByteLength::zero();
        }
    }
}

void ByteBlockFormatter::finishItem(const bool lastItem) {
    ++_currentColumn;
    ++_currentGroupByte;
    if (_format.hasFlag(ByteFormatFlag::Lines) && (_currentColumn >= _format.bytesPerLine() || lastItem)) {
        _currentColumn = unit::ByteLength::zero();
        _currentGroupByte = unit::ByteLength::zero();
        ++_currentLine;
        appendText(_format.lineSuffix());
        if (_format.hasFlag(ByteFormatFlag::LineGroups) && _currentLine >= _format.lineGroupSize() && !lastItem) {
            appendText(_format.lineSuffix());
            _currentLine = unit::ItemCount::zero();
        }
    }
}

auto formatByteBlock(StringAppendTools &sink, const mem::ByteBlock &bytes, const ByteFormat &format) -> unit::CpLength {
    return ByteBlockFormatter{sink, bytes, format}.format();
}

}
