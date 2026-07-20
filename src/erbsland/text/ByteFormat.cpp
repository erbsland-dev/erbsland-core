// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteFormat.hpp"

#include "String.hpp"
#include "StringConverter.hpp"

namespace erbsland::text {

using namespace literals;
using unit::ByteIndex;
using unit::ByteLength;
using unit::ElementCount;

struct ByteFormat::Private {
    Private() : byteSeparator{" "_el}, offsetSeparator{" | "_el}, lineSuffix{"\n"_el} {}

    ByteFormatFlags flags;                        ///< The active format flags.
    LetterCase letterCase{LetterCase::Lowercase}; ///< The case for ASCII letters.
    ByteLength bytesPerLine{ByteLength{32U}};     ///< The number of bytes per line.
    ByteLength byteGroupSize{ByteLength{4U}};     ///< The number of bytes in a group.
    ElementCount lineGroupSize{ElementCount{8U}}; ///< The number of lines in a group.
    String byteSeparator;                         ///< The separator between bytes or byte groups.
    String offsetSeparator;                       ///< The separator between the offset and bytes.
    String linePrefix;                            ///< The prefix inserted before each byte-data line.
    String lineSuffix;                            ///< The suffix inserted after each byte-data line.
    ByteIndex startOffset{ByteIndex::zero()};     ///< The start offset for the dump.
    ByteLength maximum{ByteLength::infinite()};   ///< The maximum byte-like output item count.
    TruncateMode truncateMode{TruncateMode::End}; ///< The truncation mode.
    String ellipsis;                              ///< The text inserted for omitted bytes.
};

ByteFormat::ByteFormat() : _p{std::make_unique<Private>()} {
}

ByteFormat::ByteFormat(const ByteFormatFlags flags) : ByteFormat{} {
    _p->flags = flags;
}

ByteFormat::~ByteFormat() {
}

ByteFormat::ByteFormat(const ByteFormat &other) : _p{std::make_unique<Private>(*other._p)} {
}

auto ByteFormat::operator=(const ByteFormat &other) -> ByteFormat & {
    if (this == &other) {
        return *this;
    }
    _p = std::make_unique<Private>(*other._p);
    return *this;
}

auto ByteFormat::flags() const noexcept -> ByteFormatFlags {
    return _p->flags;
}

auto ByteFormat::setFlags(ByteFormatFlags flags) noexcept -> ByteFormat & {
    _p->flags = flags;
    return *this;
}

auto ByteFormat::addFlags(ByteFormatFlags flags) noexcept -> ByteFormat & {
    _p->flags.set(flags);
    return *this;
}

auto ByteFormat::clearFlags(ByteFormatFlags flags) noexcept -> ByteFormat & {
    _p->flags.clear(flags);
    return *this;
}

auto ByteFormat::hasFlag(ByteFormatFlag flag) const noexcept -> bool {
    return _p->flags.isSet(flag);
}

auto ByteFormat::letterCase() const noexcept -> LetterCase {
    return _p->letterCase;
}

auto ByteFormat::setLetterCase(LetterCase letterCase) noexcept -> ByteFormat & {
    _p->letterCase = letterCase;
    return *this;
}

auto ByteFormat::bytesPerLine() const noexcept -> ByteLength {
    return _p->bytesPerLine;
}

auto ByteFormat::byteGroupSize() const noexcept -> ByteLength {
    return _p->byteGroupSize;
}

auto ByteFormat::lineGroupSize() const noexcept -> ElementCount {
    return _p->lineGroupSize;
}

auto ByteFormat::byteSeparator() const noexcept -> const String & {
    return _p->byteSeparator;
}

auto ByteFormat::setByteSeparator(const String &byteSeparator) noexcept -> ByteFormat & {
    _p->byteSeparator = byteSeparator;
    return *this;
}

auto ByteFormat::startOffset() const noexcept -> ByteIndex {
    return _p->startOffset;
}

auto ByteFormat::setStartOffset(ByteIndex startOffset) noexcept -> ByteFormat & {
    _p->startOffset = startOffset;
    return *this;
}

auto ByteFormat::maximum() const noexcept -> ByteLength {
    return _p->maximum;
}

auto ByteFormat::setMaximum(const ByteLength maximum) noexcept -> ByteFormat & {
    _p->maximum = maximum;
    return *this;
}

auto ByteFormat::truncateMode() const noexcept -> TruncateMode {
    return _p->truncateMode;
}

auto ByteFormat::setTruncateMode(const TruncateMode truncateMode) noexcept -> ByteFormat & {
    _p->truncateMode = truncateMode;
    return *this;
}

auto ByteFormat::ellipsis() const noexcept -> const String & {
    return _p->ellipsis;
}

auto ByteFormat::setEllipsis(const String &ellipsis) -> ByteFormat & {
    _p->ellipsis = ellipsis;
    return *this;
}

auto ByteFormat::setBytesPerLine(const ByteLength bytesPerLine) noexcept -> ByteFormat & {
    _p->bytesPerLine = atLeastOne(bytesPerLine);
    return *this;
}

auto ByteFormat::setByteGroupSize(const ByteLength byteGroupSize) noexcept -> ByteFormat & {
    _p->byteGroupSize = atLeastOne(byteGroupSize);
    return *this;
}

auto ByteFormat::setLineGroupSize(const ElementCount lineGroupSize) noexcept -> ByteFormat & {
    _p->lineGroupSize = atLeastOne(lineGroupSize);
    return *this;
}

auto ByteFormat::offsetSeparator() const noexcept -> const String & {
    return _p->offsetSeparator;
}

auto ByteFormat::setOffsetSeparator(const String &offsetSeparator) -> ByteFormat & {
    _p->offsetSeparator = offsetSeparator;
    return *this;
}

auto ByteFormat::linePrefix() const noexcept -> const String & {
    return _p->linePrefix;
}

auto ByteFormat::setLinePrefix(const String &linePrefix) -> ByteFormat & {
    _p->linePrefix = linePrefix;
    return *this;
}

auto ByteFormat::lineSuffix() const noexcept -> const String & {
    return _p->lineSuffix;
}

auto ByteFormat::setLineSuffix(const String &lineSuffix) -> ByteFormat & {
    _p->lineSuffix = lineSuffix;
    return *this;
}

auto ByteFormat::defaultFormat() -> ByteFormat {
    return {};
}

auto ByteFormat::compact() -> ByteFormat {
    return {};
}

auto ByteFormat::separated() -> ByteFormat {
    return ByteFormat{ByteFormatFlag::Separator};
}

auto ByteFormat::memoryDump() -> ByteFormat {
    return ByteFormat{
        ByteFormatFlag::Separator | ByteFormatFlag::ByteGroups | ByteFormatFlag::Lines | ByteFormatFlag::Offset};
}

auto ByteFormat::forDiagnostic() -> ByteFormat {
    return ByteFormat::compact()
        .setMaximum(ByteLength{16U})
        .setTruncateMode(TruncateMode::Middle)
        .setEllipsis("..."_el);
}

auto ByteFormat::atLeastOne(const ByteLength value) noexcept -> ByteLength {
    return value.isZero() ? ByteLength::one() : value;
}

auto ByteFormat::atLeastOne(const ElementCount value) noexcept -> ElementCount {
    return value.isZero() ? ElementCount::one() : value;
}

}
