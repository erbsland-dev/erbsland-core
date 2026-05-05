// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringBuilder.hpp"

#include "U8StringAppendTools.hpp"

#include "../U8StringView.hpp"

#include "../../AnyString.hpp"
#include "../../StringConverter.hpp"
#include "../../u16/U16String.hpp"
#include "../../u16/U16StringView.hpp"
#include "../../u32/U32String.hpp"
#include "../../u32/U32StringView.hpp"

#include <utility>

namespace erbsland::text::impl {

U8StringBuilder::U8StringBuilder(const unit::ByteLength capacity) {
    _text.reserve(capacity);
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

void U8StringBuilder::append(const Char character) {
    _length += U8StringAppendTools{_text._storage}.append(character);
}

void U8StringBuilder::append(const Char character, unit::CpLength count) {
    _length += U8StringAppendTools{_text._storage}.append(character, count);
}

void U8StringBuilder::append(const U8StringView &text) {
    _length += U8StringAppendTools{_text._storage}.append(text.dataView());
}

void U8StringBuilder::append(const U8StringView &text, const unit::ElementCount count) {
    _length += U8StringAppendTools{_text._storage}.append(text.dataView(), count);
}

void U8StringBuilder::append(const U16StringView &text) {
    _length += U8StringAppendTools{_text._storage}.append(text.dataView());
}

void U8StringBuilder::append(const U16StringView &text, const unit::ElementCount count) {
    _length += U8StringAppendTools{_text._storage}.append(text.dataView(), count);
}

void U8StringBuilder::append(const U32StringView &text) {
    _length += U8StringAppendTools{_text._storage}.append(text.dataView());
}

void U8StringBuilder::append(const U32StringView &text, const unit::ElementCount count) {
    _length += U8StringAppendTools{_text._storage}.append(text.dataView(), count);
}

auto U8StringBuilder::toU8String() const -> U8String {
    return _text;
}

auto U8StringBuilder::toU16String() const -> U16String {
    return StringConverter{_text}.toU16String();
}

auto U8StringBuilder::toU32String() const -> U32String {
    return StringConverter{_text}.toU32String();
}

auto U8StringBuilder::takeU8String() -> U8String {
    auto result = std::move(_text);
    _text = U8String{};
    _length = unit::CpLength::zero();
    return result;
}

auto U8StringBuilder::takeU16String() -> U16String {
    auto result = toU16String();
    clear();
    return result;
}

auto U8StringBuilder::takeU32String() -> U32String {
    auto result = toU32String();
    clear();
    return result;
}

auto U8StringBuilder::toAnyString() const -> AnyString {
    return AnyString{_text};
}

auto U8StringBuilder::takeAnyString() -> AnyString {
    auto result = AnyString{std::move(_text)};
    clear();
    return result;
}

}
