// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringBuilder.hpp"

#include "U16StringAppendTools.hpp"

#include "../U16StringView.hpp"

#include "../../AnyString.hpp"
#include "../../StringConverter.hpp"
#include "../../u32/U32String.hpp"
#include "../../u32/U32StringView.hpp"
#include "../../u8/U8String.hpp"
#include "../../u8/U8StringView.hpp"

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

void U16StringBuilder::append(const Char character) {
    _length += U16StringAppendTools{_text._storage}.append(character);
}

void U16StringBuilder::append(const Char character, unit::CpLength count) {
    _length += U16StringAppendTools{_text._storage}.append(character, count);
}

void U16StringBuilder::append(const U8StringView &text) {
    _length += U16StringAppendTools{_text._storage}.append(text.dataView());
}

void U16StringBuilder::append(const U8StringView &text, const unit::ElementCount count) {
    _length += U16StringAppendTools{_text._storage}.append(text.dataView(), count);
}

void U16StringBuilder::append(const U16StringView &text) {
    _length += U16StringAppendTools{_text._storage}.append(text.dataView());
}

void U16StringBuilder::append(const U16StringView &text, const unit::ElementCount count) {
    _length += U16StringAppendTools{_text._storage}.append(text.dataView(), count);
}

void U16StringBuilder::append(const U32StringView &text) {
    _length += U16StringAppendTools{_text._storage}.append(text.dataView());
}

void U16StringBuilder::append(const U32StringView &text, const unit::ElementCount count) {
    _length += U16StringAppendTools{_text._storage}.append(text.dataView(), count);
}

auto U16StringBuilder::toU8String() const -> U8String {
    return StringConverter{_text}.toU8String();
}

auto U16StringBuilder::toU16String() const -> U16String {
    return _text;
}

auto U16StringBuilder::toU32String() const -> U32String {
    return StringConverter{_text}.toU32String();
}

auto U16StringBuilder::takeU8String() -> U8String {
    auto result = toU8String();
    clear();
    return result;
}

auto U16StringBuilder::takeU16String() -> U16String {
    auto result = std::move(_text);
    clear();
    return result;
}

auto U16StringBuilder::takeU32String() -> U32String {
    auto result = toU32String();
    clear();
    return result;
}

auto U16StringBuilder::toAnyString() const -> AnyString {
    return AnyString{_text};
}

auto U16StringBuilder::takeAnyString() -> AnyString {
    auto result = AnyString{std::move(_text)};
    clear();
    return result;
}

}
