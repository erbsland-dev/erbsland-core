// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringBuilder.hpp"

#include "U16StringAppendTools.hpp"

#include "../U16String.hpp"

#include "../../AnyStringEditor.hpp"
#include "../../impl/ByteBlockFormatter.hpp"
#include "../../StringConverter.hpp"
#include "../../u32/U32String.hpp"
#include "../../u32/U32StringEditor.hpp"
#include "../../u8/U8String.hpp"
#include "../../u8/U8StringEditor.hpp"

#include <utility>

namespace erbsland::text::impl {

U16StringBuilder::U16StringBuilder(const unit::U16DataLength capacity) {
    _text.reserve(capacity);
}

auto U16StringBuilder::clone() const -> U16StringBuilder * {
    return new U16StringBuilder{*this};
}

auto U16StringBuilder::kind() const noexcept -> StringKind {
    return StringKind::U16;
}

auto U16StringBuilder::length() const noexcept -> unit::CpLength {
    return _length;
}

auto U16StringBuilder::isEmpty() const noexcept -> bool {
    return _length.isZero();
}

void U16StringBuilder::clear() noexcept {
    _text.clear();
    _length = unit::CpLength::zero();
}

auto U16StringBuilder::append(const Char character) -> unit::CpLength {
    const auto appendedLength = U16StringAppendTools{_text._storage}.append(character);
    _length += appendedLength;
    return appendedLength;
}

void U16StringBuilder::append(const Char character, unit::CpLength count) {
    _length += U16StringAppendTools{_text._storage}.append(character, count);
}

auto U16StringBuilder::append(const U8String &text) -> unit::CpLength {
    const auto appendedLength = U16StringAppendTools{_text._storage}.append(text.dataView());
    _length += appendedLength;
    return appendedLength;
}

void U16StringBuilder::append(const U8String &text, const unit::ItemCount count) {
    _length += U16StringAppendTools{_text._storage}.append(text.dataView(), count);
}

auto U16StringBuilder::append(const U16String &text) -> unit::CpLength {
    const auto appendedLength = U16StringAppendTools{_text._storage}.append(text.dataView());
    _length += appendedLength;
    return appendedLength;
}

void U16StringBuilder::append(const U16String &text, const unit::ItemCount count) {
    _length += U16StringAppendTools{_text._storage}.append(text.dataView(), count);
}

auto U16StringBuilder::append(const U32String &text) -> unit::CpLength {
    const auto appendedLength = U16StringAppendTools{_text._storage}.append(text.dataView());
    _length += appendedLength;
    return appendedLength;
}

void U16StringBuilder::append(const U32String &text, const unit::ItemCount count) {
    _length += U16StringAppendTools{_text._storage}.append(text.dataView(), count);
}

void U16StringBuilder::appendByteBlock(const mem::ByteBlock &bytes, const ByteFormat &format) {
    formatByteBlock(*this, bytes, format);
}

auto U16StringBuilder::toU8StringEditor() const -> U8StringEditor {
    return U8StringEditor{StringConverter{_text}.toU8String()};
}

auto U16StringBuilder::toU16StringEditor() const -> U16StringEditor {
    return _text;
}

auto U16StringBuilder::toU32StringEditor() const -> U32StringEditor {
    return U32StringEditor{StringConverter{_text}.toU32String()};
}

auto U16StringBuilder::takeU8StringEditor() -> U8StringEditor {
    auto result = toU8StringEditor();
    clear();
    return result;
}

auto U16StringBuilder::takeU16StringEditor() -> U16StringEditor {
    auto result = std::move(_text);
    clear();
    return result;
}

auto U16StringBuilder::takeU32StringEditor() -> U32StringEditor {
    auto result = toU32StringEditor();
    clear();
    return result;
}

auto U16StringBuilder::toAnyStringEditor() const -> AnyStringEditor {
    return AnyStringEditor{_text};
}

auto U16StringBuilder::takeAnyStringEditor() -> AnyStringEditor {
    auto result = AnyStringEditor{std::move(_text)};
    clear();
    return result;
}

}
