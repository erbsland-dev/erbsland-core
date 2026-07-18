// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringBuilder.hpp"

#include "../U32String.hpp"

#include "../../AnyStringEditor.hpp"
#include "../../impl/ByteBlockFormatter.hpp"
#include "../../StringConverter.hpp"
#include "../../u16/U16String.hpp"
#include "../../u16/U16StringConstIterator.hpp"
#include "../../u16/U16StringEditor.hpp"
#include "../../u8/U8String.hpp"
#include "../../u8/U8StringConstIterator.hpp"
#include "../../u8/U8StringEditor.hpp"

#include <utility>

namespace erbsland::text::impl {

U32StringBuilder::U32StringBuilder(const unit::CpLength capacity) {
    _text.reserve(capacity);
}

auto U32StringBuilder::clone() const -> U32StringBuilder * {
    return new U32StringBuilder{*this};
}

auto U32StringBuilder::kind() const noexcept -> StringKind {
    return StringKind::U32;
}

auto U32StringBuilder::length() const noexcept -> unit::CpLength {
    return _text.length();
}

auto U32StringBuilder::isEmpty() const noexcept -> bool {
    return _text.isEmpty();
}

void U32StringBuilder::clear() noexcept {
    _text.clear();
}

void U32StringBuilder::append(const Char character) {
    _text.append(character);
}

void U32StringBuilder::append(const Char character, unit::CpLength count) {
    _text.append(character, count);
}

void U32StringBuilder::append(const U8String &text) {
    for (const auto character : text) {
        append(character);
    }
}

void U32StringBuilder::append(const U8String &text, const unit::ElementCount count) {
    const auto convertedText = StringConverter{text}.toU32String();
    _text.append(U32String{convertedText}, count);
}

void U32StringBuilder::append(const U16String &text) {
    for (const auto character : text) {
        append(character);
    }
}

void U32StringBuilder::append(const U16String &text, const unit::ElementCount count) {
    const auto convertedText = StringConverter{text}.toU32String();
    _text.append(U32String{convertedText}, count);
}

void U32StringBuilder::append(const U32String &text) {
    _text.append(text);
}

void U32StringBuilder::append(const U32String &text, const unit::ElementCount count) {
    _text.append(text, count);
}

void U32StringBuilder::appendByteBlock(const mem::ByteBlock &bytes, const ByteFormat &format) {
    formatByteBlock(*this, bytes, format);
}

auto U32StringBuilder::toU8StringEditor() const -> U8StringEditor {
    return U8StringEditor{StringConverter{_text}.toU8String()};
}

auto U32StringBuilder::toU16StringEditor() const -> U16StringEditor {
    return U16StringEditor{StringConverter{_text}.toU16String()};
}

auto U32StringBuilder::toU32StringEditor() const -> U32StringEditor {
    return _text;
}

auto U32StringBuilder::takeU8StringEditor() -> U8StringEditor {
    auto result = toU8StringEditor();
    clear();
    return result;
}

auto U32StringBuilder::takeU16StringEditor() -> U16StringEditor {
    auto result = toU16StringEditor();
    clear();
    return result;
}

auto U32StringBuilder::takeU32StringEditor() -> U32StringEditor {
    auto result = std::move(_text);
    clear();
    return result;
}

auto U32StringBuilder::toAnyStringEditor() const -> AnyStringEditor {
    return AnyStringEditor{_text};
}

auto U32StringBuilder::takeAnyStringEditor() -> AnyStringEditor {
    auto result = AnyStringEditor{std::move(_text)};
    clear();
    return result;
}

}
