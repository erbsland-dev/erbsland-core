// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringBuilder.hpp"

#include "U8StringAppendTools.hpp"

#include "../U8String.hpp"

#include "../../AnyStringEditor.hpp"
#include "../../impl/ByteBlockFormatter.hpp"
#include "../../StringConverter.hpp"
#include "../../u16/U16String.hpp"
#include "../../u16/U16StringEditor.hpp"
#include "../../u32/U32String.hpp"
#include "../../u32/U32StringEditor.hpp"

#include <utility>

namespace erbsland::text::impl {

U8StringBuilder::U8StringBuilder(const unit::ByteLength capacity, const bool sensitive) : _sensitive{sensitive} {
    _text.reserve(sensitive && capacity.isZero() ? unit::ByteLength{1U} : capacity);
    if (sensitive) {
        _text.markAsSensitive();
    }
}

auto U8StringBuilder::clone() const -> U8StringBuilder * {
    return new U8StringBuilder{*this};
}

auto U8StringBuilder::kind() const noexcept -> StringKind {
    return StringKind::U8;
}

auto U8StringBuilder::length() const noexcept -> unit::CpLength {
    return _length;
}

auto U8StringBuilder::isEmpty() const noexcept -> bool {
    return _length.isZero();
}

void U8StringBuilder::clear() noexcept {
    _text.clear();
    _length = unit::CpLength::zero();
}

auto U8StringBuilder::append(const Char character) -> unit::CpLength {
    const auto appendedLength = U8StringAppendTools{_text._storage}.append(character);
    _length += appendedLength;
    return appendedLength;
}

void U8StringBuilder::append(const Char character, unit::CpLength count) {
    _length += U8StringAppendTools{_text._storage}.append(character, count);
}

auto U8StringBuilder::append(const U8String &text) -> unit::CpLength {
    const auto appendedLength = U8StringAppendTools{_text._storage}.append(text.dataView());
    _length += appendedLength;
    return appendedLength;
}

void U8StringBuilder::append(const U8String &text, const unit::ItemCount count) {
    _length += U8StringAppendTools{_text._storage}.append(text.dataView(), count);
}

auto U8StringBuilder::append(const U16String &text) -> unit::CpLength {
    const auto appendedLength = U8StringAppendTools{_text._storage}.append(text.dataView());
    _length += appendedLength;
    return appendedLength;
}

void U8StringBuilder::append(const U16String &text, const unit::ItemCount count) {
    _length += U8StringAppendTools{_text._storage}.append(text.dataView(), count);
}

auto U8StringBuilder::append(const U32String &text) -> unit::CpLength {
    const auto appendedLength = U8StringAppendTools{_text._storage}.append(text.dataView());
    _length += appendedLength;
    return appendedLength;
}

void U8StringBuilder::append(const U32String &text, const unit::ItemCount count) {
    _length += U8StringAppendTools{_text._storage}.append(text.dataView(), count);
}

void U8StringBuilder::appendByteBlock(const mem::ByteBlock &bytes, const ByteFormat &format) {
    formatByteBlock(*this, bytes, format);
}

auto U8StringBuilder::toU8StringEditor() const -> U8StringEditor {
    return _text;
}

auto U8StringBuilder::toU16StringEditor() const -> U16StringEditor {
    return U16StringEditor{StringConverter{_text}.toU16String()};
}

auto U8StringBuilder::toU32StringEditor() const -> U32StringEditor {
    return U32StringEditor{StringConverter{_text}.toU32String()};
}

auto U8StringBuilder::takeU8StringEditor() -> U8StringEditor {
    auto result = std::move(_text);
    resetAfterTake();
    return result;
}

auto U8StringBuilder::takeU16StringEditor() -> U16StringEditor {
    auto result = toU16StringEditor();
    clear();
    return result;
}

auto U8StringBuilder::takeU32StringEditor() -> U32StringEditor {
    auto result = toU32StringEditor();
    clear();
    return result;
}

auto U8StringBuilder::toAnyStringEditor() const -> AnyStringEditor {
    return AnyStringEditor{_text};
}

auto U8StringBuilder::takeAnyStringEditor() -> AnyStringEditor {
    auto result = AnyStringEditor{std::move(_text)};
    resetAfterTake();
    return result;
}

void U8StringBuilder::resetAfterTake() {
    _text = U8StringEditor{};
    if (_sensitive) {
        _text.reserve(unit::ByteLength{1U});
        _text.markAsSensitive();
    }
    _length = unit::CpLength::zero();
}

}
