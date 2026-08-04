// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AnyStringBuilder.hpp"

#include "AnyString.hpp"
#include "AnyStringEditor.hpp"

#include "impl/AnyStringBuilderFactory.hpp"
#include "impl/FloatConversion.hpp"
#include "u16/U16String.hpp"
#include "u16/U16StringEditor.hpp"
#include "u16/U16StringLiteral.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringEditor.hpp"
#include "u32/U32StringLiteral.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringEditor.hpp"
#include "u8/U8StringLiteral.hpp"

#include "../mem/ByteBlock.hpp"

#include <utility>

namespace erbsland::text {

using unit::ByteLength;
using unit::CpLength;
using unit::ItemCount;
using unit::U16DataLength;

AnyStringBuilder::AnyStringBuilder() : AnyStringBuilder{StringKind::U8} {
}

AnyStringBuilder::AnyStringBuilder(const StringKind kind) : _builder{impl::createAnyStringBuilder(kind)} {
}

AnyStringBuilder::AnyStringBuilder(BuilderPtr builder) : _builder{std::move(builder)} {
}

auto AnyStringBuilder::u8() -> AnyStringBuilder {
    return AnyStringBuilder{};
}

auto AnyStringBuilder::u8(const ByteLength capacity) -> AnyStringBuilder {
    return AnyStringBuilder{impl::createU8StringBuilder(capacity)};
}

auto AnyStringBuilder::u16() -> AnyStringBuilder {
    return AnyStringBuilder{StringKind::U16};
}

auto AnyStringBuilder::u16(const U16DataLength capacity) -> AnyStringBuilder {
    return AnyStringBuilder{impl::createU16StringBuilder(capacity)};
}

auto AnyStringBuilder::u32() -> AnyStringBuilder {
    return AnyStringBuilder{StringKind::U32};
}

auto AnyStringBuilder::u32(const CpLength capacity) -> AnyStringBuilder {
    return AnyStringBuilder{impl::createU32StringBuilder(capacity)};
}

auto AnyStringBuilder::withCapacity(const StringKind kind, const CpLength capacity) -> AnyStringBuilder {
    return AnyStringBuilder{impl::createAnyStringBuilder(kind, capacity)};
}

auto AnyStringBuilder::basedOn(const U8String &initial, const ByteLength additionalCapacity) -> AnyStringBuilder {
    auto result = AnyStringBuilder::u8(initial.length() + additionalCapacity);
    result.append(initial);
    return result;
}

auto AnyStringBuilder::basedOn(const U16String &initial, const U16DataLength additionalCapacity) -> AnyStringBuilder {
    auto result = AnyStringBuilder::u16(initial.length() + additionalCapacity);
    result.append(initial);
    return result;
}

auto AnyStringBuilder::basedOn(const U32String &initial, const CpLength additionalCapacity) -> AnyStringBuilder {
    auto result = AnyStringBuilder::u32(initial.length() + additionalCapacity);
    result.append(initial);
    return result;
}

auto AnyStringBuilder::kind() const noexcept -> StringKind {
    return _builder->kind();
}

auto AnyStringBuilder::length() const noexcept -> CpLength {
    return _builder->length();
}

auto AnyStringBuilder::isEmpty() const noexcept -> bool {
    return _builder->isEmpty();
}

void AnyStringBuilder::clear() noexcept {
    _builder->clear();
}

auto AnyStringBuilder::append(const Char character) -> AnyStringBuilder & {
    _builder->append(character);
    return *this;
}

auto AnyStringBuilder::append(const char32_t codePoint) -> AnyStringBuilder & {
    return append(Char{codePoint});
}

auto AnyStringBuilder::append(const Char character, CpLength count) -> AnyStringBuilder & {
    _builder->append(character, count);
    return *this;
}

auto AnyStringBuilder::append(const U8String &text) -> AnyStringBuilder & {
    _builder->append(text);
    return *this;
}

auto AnyStringBuilder::append(const U8String &text, const ItemCount count) -> AnyStringBuilder & {
    _builder->append(text, count);
    return *this;
}

auto AnyStringBuilder::append(const U16String &text) -> AnyStringBuilder & {
    _builder->append(text);
    return *this;
}

auto AnyStringBuilder::append(const U16String &text, const ItemCount count) -> AnyStringBuilder & {
    _builder->append(text, count);
    return *this;
}

auto AnyStringBuilder::append(const U32String &text) -> AnyStringBuilder & {
    _builder->append(text);
    return *this;
}

auto AnyStringBuilder::append(const U32String &text, const ItemCount count) -> AnyStringBuilder & {
    _builder->append(text, count);
    return *this;
}

auto AnyStringBuilder::append(const U8StringLiteral<char> &text) -> AnyStringBuilder & {
    return append(U8String{text});
}

auto AnyStringBuilder::append(const U8StringLiteral<char8_t> &text) -> AnyStringBuilder & {
    return append(U8String{text});
}

auto AnyStringBuilder::append(const U16StringLiteral &text) -> AnyStringBuilder & {
    return append(U16String{text});
}

auto AnyStringBuilder::append(const U32StringLiteral &text) -> AnyStringBuilder & {
    return append(U32String{text});
}

auto AnyStringBuilder::appendAny(const AnyString &text) -> AnyStringBuilder & {
    if (text.isEmpty() || !text.kind().has_value()) {
        return *this;
    }
    switch (text.kind().value()) {
    case StringKind::U8:
        return append(text.toU8String());
    case StringKind::U16:
        return append(text.toU16String());
    case StringKind::U32:
        return append(text.toU32String());
    }
    return *this;
}

auto AnyStringBuilder::appendAny(const AnyStringEditor &text) -> AnyStringBuilder & {
    if (text.isEmpty() || !text.kind().has_value()) {
        return *this;
    }
    switch (text.kind().value()) {
    case StringKind::U8:
        return append(U8String{text.toU8String()});
    case StringKind::U16:
        return append(U16String{text.toU16String()});
    case StringKind::U32:
        return append(U32String{text.toU32String()});
    }
    return *this;
}

template <impl::AnyFloatType T>
auto AnyStringBuilder::appendFloat(const T value, const FloatFormat format) -> AnyStringBuilder & {
    const auto text = impl::formatFloat(static_cast<double>(value), format);
    append(U8String{text});
    return *this;
}

auto AnyStringBuilder::appendByteBlock(const mem::ByteBlock &bytes, const ByteFormat &format) -> AnyStringBuilder & {
    _builder->appendByteBlock(bytes, format);
    return *this;
}

auto AnyStringBuilder::toU8String() const -> U8String {
    return _builder->toU8StringEditor();
}

auto AnyStringBuilder::toString() const -> String {
    return _builder->toU8StringEditor();
}

auto AnyStringBuilder::toU16String() const -> U16String {
    return _builder->toU16StringEditor();
}

auto AnyStringBuilder::toU32String() const -> U32String {
    return _builder->toU32StringEditor();
}

auto AnyStringBuilder::toAnyString() const -> AnyString {
    return _builder->toAnyStringEditor();
}

auto AnyStringBuilder::toU8StringEditor() const -> U8StringEditor {
    return _builder->toU8StringEditor();
}

auto AnyStringBuilder::toStringEditor() const -> StringEditor {
    return _builder->toU8StringEditor();
}

auto AnyStringBuilder::toU16StringEditor() const -> U16StringEditor {
    return _builder->toU16StringEditor();
}

auto AnyStringBuilder::toU32StringEditor() const -> U32StringEditor {
    return _builder->toU32StringEditor();
}

auto AnyStringBuilder::toAnyStringEditor() const -> AnyStringEditor {
    return _builder->toAnyStringEditor();
}

template <>
auto AnyStringBuilder::toEditor<U8StringEditor>() const -> U8StringEditor {
    return toU8StringEditor();
}

template <>
auto AnyStringBuilder::toEditor<U16StringEditor>() const -> U16StringEditor {
    return toU16StringEditor();
}

template <>
auto AnyStringBuilder::toEditor<U32StringEditor>() const -> U32StringEditor {
    return toU32StringEditor();
}

auto AnyStringBuilder::takeU8String() -> U8String {
    return _builder->takeU8StringEditor();
}

auto AnyStringBuilder::takeString() -> String {
    return _builder->takeU8StringEditor();
}

auto AnyStringBuilder::takeU16String() -> U16String {
    return _builder->takeU16StringEditor();
}

auto AnyStringBuilder::takeU32String() -> U32String {
    return _builder->takeU32StringEditor();
}

auto AnyStringBuilder::takeAnyString() -> AnyString {
    return _builder->takeAnyStringEditor();
}

auto AnyStringBuilder::takeU8StringEditor() -> U8StringEditor {
    return _builder->takeU8StringEditor();
}

auto AnyStringBuilder::takeStringEditor() -> StringEditor {
    return _builder->takeU8StringEditor();
}

auto AnyStringBuilder::takeU16StringEditor() -> U16StringEditor {
    return _builder->takeU16StringEditor();
}

auto AnyStringBuilder::takeU32StringEditor() -> U32StringEditor {
    return _builder->takeU32StringEditor();
}

auto AnyStringBuilder::takeAnyStringEditor() -> AnyStringEditor {
    return _builder->takeAnyStringEditor();
}

template auto AnyStringBuilder::appendFloat<float>(float value, FloatFormat format) -> AnyStringBuilder &;
template auto AnyStringBuilder::appendFloat<double>(double value, FloatFormat format) -> AnyStringBuilder &;

}
