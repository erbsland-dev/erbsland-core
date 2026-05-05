// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringBuilderStream.hpp"

#include "impl/PrintContextToBuilder.hpp"

#include "../text/AnyString.hpp"
#include "../text/u16/U16String.hpp"
#include "../text/u32/U32String.hpp"
#include "../text/u8/U8String.hpp"

namespace erbsland::stream {

StringBuilderStream::StringBuilderStream(text::StringKind stringKind) : _builder{stringKind} {
}

auto StringBuilderStream::kind() const noexcept -> text::StringKind {
    return _builder.kind();
}
auto StringBuilderStream::length() const noexcept -> unit::CpLength {
    return _builder.length();
}
auto StringBuilderStream::isEmpty() const noexcept -> bool {
    return _builder.isEmpty();
}
void StringBuilderStream::clear() noexcept {
    _builder.clear();
}
auto StringBuilderStream::toU8String() const -> text::U8String {
    return _builder.toU8String();
}
auto StringBuilderStream::toString() const -> text::U8String {
    return _builder.toString();
}

auto StringBuilderStream::toU16String() const -> text::U16String {
    return _builder.toU16String();
}

auto StringBuilderStream::toU32String() const -> text::U32String {
    return _builder.toU32String();
}

auto StringBuilderStream::toAnyString() const -> text::AnyString {
    return _builder.toAnyString();
}

auto StringBuilderStream::takeU8String() -> text::U8String {
    return _builder.takeU8String();
}

auto StringBuilderStream::takeString() -> text::String {
    return _builder.takeString();
}

auto StringBuilderStream::takeU16String() -> text::U16String {
    return _builder.takeU16String();
}

auto StringBuilderStream::takeU32String() -> text::U32String {
    return _builder.takeU32String();
}

auto StringBuilderStream::takeAnyString() -> text::AnyString {
    return _builder.takeAnyString();
}

auto StringBuilderStream::isOpen() const noexcept -> bool {
    return true;
}

void StringBuilderStream::flush() {
    // ignore
}

void StringBuilderStream::close() {
    // ignore
}

auto StringBuilderStream::encoding() const noexcept -> text::StringEncoding {
    switch (_builder.kind()) {
    case text::StringKind::U8:
        return text::StringEncoding::Utf8;
    case text::StringKind::U16:
        return text::StringEncoding::Utf16;
    case text::StringKind::U32:
        return text::StringEncoding::Utf32;
    }
    return text::StringEncoding::Utf8;
}

auto StringBuilderStream::effectiveEncoding() const noexcept -> text::StringEncoding {
    return encoding();
}

void StringBuilderStream::write(text::Char character) {
    _builder.append(character);
}

void StringBuilderStream::write(const text::StringView &text) {
    _builder.append(text);
}

void StringBuilderStream::writeLine() {
    _builder.append(U'\n');
}

void StringBuilderStream::writeLine(const text::StringView &text) {
    _builder.append(text);
    _builder.append(U'\n');
}

auto StringBuilderStream::createPrintContext() -> TextPrintContextPtr {
    return std::make_unique<impl::PrintContextToBuilder>(_builder);
}

}
