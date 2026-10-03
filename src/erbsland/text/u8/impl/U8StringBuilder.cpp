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

using namespace unit;

U8StringBuilder::U8StringBuilder(const ByteCount capacity, const bool sensitive) : _sensitive{sensitive} {
    _text.reserve(sensitive && capacity.isZero() ? ByteCount::one() : capacity);
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

auto U8StringBuilder::length() const noexcept -> CpCount {
    return _length;
}

auto U8StringBuilder::isEmpty() const noexcept -> bool {
    return _length.isZero();
}

void U8StringBuilder::clear() noexcept {
    _text.clear();
    _length = CpCount::zero();
}

auto U8StringBuilder::append(const Char character) -> CpCount {
    const auto appendedLength = U8StringAppendTools{_text._storage}.append(character);
    _length += appendedLength;
    return appendedLength;
}

void U8StringBuilder::append(const Char character, CpCount count) {
    _length += U8StringAppendTools{_text._storage}.append(character, count);
}

auto U8StringBuilder::append(const U8String &text) -> CpCount {
    if (text.isSensitive()) {
        _text._storage.markAsSensitive();
    }
    const auto appendedLength = U8StringAppendTools{_text._storage}.append(text.dataView());
    _length += appendedLength;
    return appendedLength;
}

void U8StringBuilder::append(const U8String &text, const ItemCount count) {
    if (text.isSensitive()) {
        _text._storage.markAsSensitive();
    }
    _length += U8StringAppendTools{_text._storage}.append(text.dataView(), count);
}

auto U8StringBuilder::append(const U16String &text) -> CpCount {
    const auto appendedLength = U8StringAppendTools{_text._storage}.append(text.dataView());
    _length += appendedLength;
    return appendedLength;
}

void U8StringBuilder::append(const U16String &text, const ItemCount count) {
    _length += U8StringAppendTools{_text._storage}.append(text.dataView(), count);
}

auto U8StringBuilder::append(const U32String &text) -> CpCount {
    const auto appendedLength = U8StringAppendTools{_text._storage}.append(text.dataView());
    _length += appendedLength;
    return appendedLength;
}

void U8StringBuilder::append(const U32String &text, const ItemCount count) {
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
        _text.reserve(ByteCount{1U});
        _text.markAsSensitive();
    }
    _length = CpCount::zero();
}

}
