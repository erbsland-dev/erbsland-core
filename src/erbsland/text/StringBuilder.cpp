// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringBuilder.hpp"

#include "AnyString.hpp"
#include "AnyStringView.hpp"

#include "impl/FloatConversion.hpp"
#include "impl/StringBuilderFactory.hpp"
#include "u16/U16String.hpp"
#include "u16/U16StringLiteral.hpp"
#include "u16/U16StringView.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringLiteral.hpp"
#include "u32/U32StringView.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringLiteral.hpp"
#include "u8/U8StringView.hpp"

#include "../mem/ByteBlockView.hpp"

#include <utility>

namespace erbsland::text {

StringBuilder::StringBuilder() : StringBuilder{StringKind::U8} {
}

StringBuilder::StringBuilder(const StringKind kind) : _builder{impl::createStringBuilder(kind)} {
}

StringBuilder::StringBuilder(BuilderPtr builder) : _builder{std::move(builder)} {
}

auto StringBuilder::u8() -> StringBuilder {
    return StringBuilder{};
}

auto StringBuilder::u8(const unit::ByteLength capacity) -> StringBuilder {
    return StringBuilder{impl::createU8StringBuilder(capacity)};
}

auto StringBuilder::u16() -> StringBuilder {
    return StringBuilder{StringKind::U16};
}

auto StringBuilder::u16(const unit::U16DataLength capacity) -> StringBuilder {
    return StringBuilder{impl::createU16StringBuilder(capacity)};
}

auto StringBuilder::u32() -> StringBuilder {
    return StringBuilder{StringKind::U32};
}

auto StringBuilder::u32(const unit::CpLength capacity) -> StringBuilder {
    return StringBuilder{impl::createU32StringBuilder(capacity)};
}

auto StringBuilder::withCapacity(const StringKind kind, const unit::CpLength capacity) -> StringBuilder {
    return StringBuilder{impl::createStringBuilder(kind, capacity)};
}

auto StringBuilder::basedOn(const U8StringView &initial, const unit::ByteLength additionalCapacity) -> StringBuilder {
    auto result = StringBuilder::u8(initial.length() + additionalCapacity);
    result.append(initial);
    return result;
}

auto StringBuilder::basedOn(const U16StringView &initial, const unit::U16DataLength additionalCapacity)
    -> StringBuilder {
    auto result = StringBuilder::u16(initial.length() + additionalCapacity);
    result.append(initial);
    return result;
}

auto StringBuilder::basedOn(const U32StringView &initial, const unit::CpLength additionalCapacity) -> StringBuilder {
    auto result = StringBuilder::u32(initial.length() + additionalCapacity);
    result.append(initial);
    return result;
}

auto StringBuilder::kind() const noexcept -> StringKind {
    return _builder->kind();
}

auto StringBuilder::length() const noexcept -> unit::CpLength {
    return _builder->length();
}

auto StringBuilder::isEmpty() const noexcept -> bool {
    return _builder->isEmpty();
}

void StringBuilder::clear() noexcept {
    _builder->clear();
}

auto StringBuilder::append(const Char character) -> StringBuilder & {
    _builder->append(character);
    return *this;
}

auto StringBuilder::append(const char32_t codePoint) -> StringBuilder & {
    return append(Char{codePoint});
}

auto StringBuilder::append(const Char character, unit::CpLength count) -> StringBuilder & {
    _builder->append(character, count);
    return *this;
}

auto StringBuilder::append(const U8StringView &text) -> StringBuilder & {
    _builder->append(text);
    return *this;
}

auto StringBuilder::append(const U8StringView &text, const unit::ElementCount count) -> StringBuilder & {
    _builder->append(text, count);
    return *this;
}

auto StringBuilder::append(const U16StringView &text) -> StringBuilder & {
    _builder->append(text);
    return *this;
}

auto StringBuilder::append(const U16StringView &text, const unit::ElementCount count) -> StringBuilder & {
    _builder->append(text, count);
    return *this;
}

auto StringBuilder::append(const U32StringView &text) -> StringBuilder & {
    _builder->append(text);
    return *this;
}

auto StringBuilder::append(const U32StringView &text, const unit::ElementCount count) -> StringBuilder & {
    _builder->append(text, count);
    return *this;
}

auto StringBuilder::append(const U8StringLiteral<char> &text) -> StringBuilder & {
    return append(U8StringView{text});
}

auto StringBuilder::append(const U8StringLiteral<char8_t> &text) -> StringBuilder & {
    return append(U8StringView{text});
}

auto StringBuilder::append(const U16StringLiteral &text) -> StringBuilder & {
    return append(U16StringView{text});
}

auto StringBuilder::append(const U32StringLiteral &text) -> StringBuilder & {
    return append(U32StringView{text});
}

auto StringBuilder::appendAny(const AnyStringView &text) -> StringBuilder & {
    if (text.isEmpty() || !text.kind().has_value()) {
        return *this;
    }
    switch (text.kind().value()) {
    case StringKind::U8:
        return append(text.toU8StringView());
    case StringKind::U16:
        return append(text.toU16StringView());
    case StringKind::U32:
        return append(text.toU32StringView());
    }
    return *this;
}

auto StringBuilder::appendAny(const AnyString &text) -> StringBuilder & {
    if (text.isEmpty() || !text.kind().has_value()) {
        return *this;
    }
    switch (text.kind().value()) {
    case StringKind::U8:
        return append(U8StringView{text.toU8String()});
    case StringKind::U16:
        return append(U16StringView{text.toU16String()});
    case StringKind::U32:
        return append(U32StringView{text.toU32String()});
    }
    return *this;
}

template <impl::AnyFloatType T>
auto StringBuilder::appendFloat(const T value, const FloatFormat format) -> StringBuilder & {
    append(impl::formatFloat(static_cast<double>(value), format));
    return *this;
}

auto StringBuilder::appendByteBlock(const mem::ByteBlockView &bytes, const ByteFormat &format) -> StringBuilder & {
    if (bytes.isEmpty()) {
        return *this;
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

    mem::ByteReader byteReader{bytes};
    while (!byteReader.isAtEnd()) {
        const auto currentByte = byteReader.readByte();
        const auto lastByte = byteReader.isAtEnd();

        if (currentColumn.isZero()) {
            if (format.hasFlag(ByteFormatFlag::Lines)) {
                append(format.linePrefix());
            }
            if (format.hasFlag(ByteFormatFlag::Offset)) {
                appendInteger(offset.toRawValue(), offsetFormat);
                append(format.offsetSeparator());
            }
        } else if (format.hasFlag(ByteFormatFlag::Separator)) {
            if (!format.hasFlag(ByteFormatFlag::ByteGroups) || currentGroupByte >= byteGroupSize) {
                append(format.byteSeparator());
                currentGroupByte = unit::ByteLength::zero();
            }
        }

        appendInteger(currentByte.toUInt8(), byteFormat);
        ++offset;
        ++currentColumn;
        ++currentGroupByte;

        if (format.hasFlag(ByteFormatFlag::Lines) && (currentColumn >= bytesPerLine || lastByte)) {
            currentColumn = unit::ByteLength::zero();
            currentGroupByte = unit::ByteLength::zero();
            ++currentLine;
            append(format.lineSuffix());
            if (format.hasFlag(ByteFormatFlag::LineGroups) && currentLine >= lineGroupSize && !lastByte) {
                append(format.lineSuffix());
                currentLine = unit::ElementCount::zero();
            }
        }
    }
    return *this;
}

auto StringBuilder::toU8String() const -> U8String {
    return _builder->toU8String();
}

auto StringBuilder::toString() const -> String {
    return _builder->toU8String();
}

auto StringBuilder::toU16String() const -> U16String {
    return _builder->toU16String();
}

auto StringBuilder::toU32String() const -> U32String {
    return _builder->toU32String();
}

auto StringBuilder::toAnyString() const -> AnyString {
    return _builder->toAnyString();
}

template <>
auto StringBuilder::to<U8String>() const -> U8String {
    return toU8String();
}

template <>
auto StringBuilder::to<U16String>() const -> U16String {
    return toU16String();
}

template <>
auto StringBuilder::to<U32String>() const -> U32String {
    return toU32String();
}

auto StringBuilder::takeU8String() -> String {
    return _builder->takeU8String();
}

auto StringBuilder::takeString() -> U8String {
    return _builder->takeU8String();
}

auto StringBuilder::takeU16String() -> U16String {
    return _builder->takeU16String();
}

auto StringBuilder::takeU32String() -> U32String {
    return _builder->takeU32String();
}

auto StringBuilder::takeAnyString() -> AnyString {
    return _builder->takeAnyString();
}

template auto StringBuilder::appendFloat<float>(float value, FloatFormat format) -> StringBuilder &;
template auto StringBuilder::appendFloat<double>(double value, FloatFormat format) -> StringBuilder &;

}
