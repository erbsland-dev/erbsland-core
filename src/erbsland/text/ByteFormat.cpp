// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteFormat.hpp"

#include "StringConverter.hpp"
#include "StringView.hpp"

namespace erbsland::text {

using namespace literals;

struct ByteFormat::Private {
    Private() : byteSeparator{" "_el}, offsetSeparator{" | "_el}, lineSuffix{"\n"_el} {}

    ByteFormatFlags flags;                                    ///< The active format flags.
    LetterCase letterCase{LetterCase::Lowercase};             ///< The case for ASCII letters.
    unit::ByteLength bytesPerLine{unit::ByteLength{32U}};     ///< The number of bytes per line.
    unit::ByteLength byteGroupSize{unit::ByteLength{4U}};     ///< The number of bytes in a group.
    unit::ElementCount lineGroupSize{unit::ElementCount{8U}}; ///< The number of lines in a group.
    StringView byteSeparator;                                 ///< The separator between bytes or byte groups.
    StringView offsetSeparator;                               ///< The separator between the offset and bytes.
    StringView linePrefix;                                    ///< The prefix inserted before each byte-data line.
    StringView lineSuffix;                                    ///< The suffix inserted after each byte-data line.
    unit::ByteIndex startOffset{unit::ByteIndex::zero()};     ///< The start offset for the dump.
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

auto ByteFormat::bytesPerLine() const noexcept -> unit::ByteLength {
    return _p->bytesPerLine;
}

auto ByteFormat::byteGroupSize() const noexcept -> unit::ByteLength {
    return _p->byteGroupSize;
}

auto ByteFormat::lineGroupSize() const noexcept -> unit::ElementCount {
    return _p->lineGroupSize;
}

auto ByteFormat::byteSeparator() const noexcept -> const StringView & {
    return _p->byteSeparator;
}

auto ByteFormat::setByteSeparator(const StringView &byteSeparator) noexcept -> ByteFormat & {
    _p->byteSeparator = byteSeparator;
    return *this;
}

auto ByteFormat::startOffset() const noexcept -> unit::ByteIndex {
    return _p->startOffset;
}

auto ByteFormat::setStartOffset(unit::ByteIndex startOffset) noexcept -> ByteFormat & {
    _p->startOffset = startOffset;
    return *this;
}

auto ByteFormat::setBytesPerLine(const unit::ByteLength bytesPerLine) noexcept -> ByteFormat & {
    _p->bytesPerLine = atLeastOne(bytesPerLine);
    return *this;
}

auto ByteFormat::setByteGroupSize(const unit::ByteLength byteGroupSize) noexcept -> ByteFormat & {
    _p->byteGroupSize = atLeastOne(byteGroupSize);
    return *this;
}

auto ByteFormat::setLineGroupSize(const unit::ElementCount lineGroupSize) noexcept -> ByteFormat & {
    _p->lineGroupSize = atLeastOne(lineGroupSize);
    return *this;
}

auto ByteFormat::offsetSeparator() const noexcept -> const StringView & {
    return _p->offsetSeparator;
}

auto ByteFormat::setOffsetSeparator(const StringView &offsetSeparator) -> ByteFormat & {
    _p->offsetSeparator = offsetSeparator;
    return *this;
}

auto ByteFormat::linePrefix() const noexcept -> const StringView & {
    return _p->linePrefix;
}

auto ByteFormat::setLinePrefix(const StringView &linePrefix) -> ByteFormat & {
    _p->linePrefix = linePrefix;
    return *this;
}

auto ByteFormat::lineSuffix() const noexcept -> const StringView & {
    return _p->lineSuffix;
}

auto ByteFormat::setLineSuffix(const StringView &lineSuffix) -> ByteFormat & {
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

auto ByteFormat::atLeastOne(const unit::ByteLength value) noexcept -> unit::ByteLength {
    return value.isZero() ? unit::ByteLength::one() : value;
}

auto ByteFormat::atLeastOne(const unit::ElementCount value) noexcept -> unit::ElementCount {
    return value.isZero() ? unit::ElementCount::one() : value;
}

}
