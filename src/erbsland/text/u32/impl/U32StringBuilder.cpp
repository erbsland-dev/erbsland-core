// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringBuilder.hpp"

#include "../U32StringView.hpp"

#include "../../AnyString.hpp"
#include "../../StringConverter.hpp"
#include "../../u16/U16String.hpp"
#include "../../u16/U16StringConstIterator.hpp"
#include "../../u16/U16StringView.hpp"
#include "../../u8/U8String.hpp"
#include "../../u8/U8StringConstIterator.hpp"
#include "../../u8/U8StringView.hpp"

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

void U32StringBuilder::append(const U8StringView &text) {
    for (const auto character : text) {
        append(character);
    }
}

void U32StringBuilder::append(const U8StringView &text, const unit::ElementCount count) {
    const auto convertedText = StringConverter{text}.toU32String();
    _text.append(U32StringView{convertedText}, count);
}

void U32StringBuilder::append(const U16StringView &text) {
    for (const auto character : text) {
        append(character);
    }
}

void U32StringBuilder::append(const U16StringView &text, const unit::ElementCount count) {
    const auto convertedText = StringConverter{text}.toU32String();
    _text.append(U32StringView{convertedText}, count);
}

void U32StringBuilder::append(const U32StringView &text) {
    _text.append(text);
}

void U32StringBuilder::append(const U32StringView &text, const unit::ElementCount count) {
    _text.append(text, count);
}

auto U32StringBuilder::toU8String() const -> U8String {
    return StringConverter{_text}.toU8String();
}

auto U32StringBuilder::toU16String() const -> U16String {
    return StringConverter{_text}.toU16String();
}

auto U32StringBuilder::toU32String() const -> U32String {
    return _text;
}

auto U32StringBuilder::takeU8String() -> U8String {
    auto result = toU8String();
    clear();
    return result;
}

auto U32StringBuilder::takeU16String() -> U16String {
    auto result = toU16String();
    clear();
    return result;
}

auto U32StringBuilder::takeU32String() -> U32String {
    auto result = std::move(_text);
    clear();
    return result;
}

auto U32StringBuilder::toAnyString() const -> AnyString {
    return AnyString{_text};
}

auto U32StringBuilder::takeAnyString() -> AnyString {
    auto result = AnyString{std::move(_text)};
    clear();
    return result;
}

}
