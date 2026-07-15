// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringBuilderStream.hpp"

#include "../text/AnyString.hpp"
#include "../text/Literals.hpp"
#include "../text/u16/U16String.hpp"
#include "../text/u32/U32String.hpp"
#include "../text/u8/U8String.hpp"

namespace erbsland::stream {

using namespace text::literals;

StringBuilderStream::StringBuilderStream(text::StringKind stringKind, ConstructionToken) : _builder{stringKind} {
}

auto StringBuilderStream::kind() const noexcept -> text::StringKind {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.kind();
}
auto StringBuilderStream::length() const noexcept -> unit::CpLength {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.length();
}
auto StringBuilderStream::isEmpty() const noexcept -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.isEmpty();
}
void StringBuilderStream::clear() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    _builder.clear();
}
auto StringBuilderStream::toU8String() const -> text::U8String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toU8String();
}
auto StringBuilderStream::toString() const -> text::U8String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toString();
}

auto StringBuilderStream::toU16String() const -> text::U16String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toU16String();
}

auto StringBuilderStream::toU32String() const -> text::U32String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toU32String();
}

auto StringBuilderStream::toAnyString() const -> text::AnyString {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toAnyString();
}

auto StringBuilderStream::takeU8String() -> text::U8String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeU8String();
}

auto StringBuilderStream::takeString() -> text::String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeString();
}

auto StringBuilderStream::takeU16String() -> text::U16String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeU16String();
}

auto StringBuilderStream::takeU32String() -> text::U32String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeU32String();
}

auto StringBuilderStream::takeAnyString() -> text::AnyString {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeAnyString();
}

auto StringBuilderStream::state() const noexcept -> StreamState {
    const auto lock = std::scoped_lock{_mutex};
    return _state;
}

auto StringBuilderStream::isReady() const noexcept -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return _state == StreamState::Open;
}

auto StringBuilderStream::waitForReady() -> StreamWaitStatus {
    return isReady() ? StreamWaitStatus::Ready : StreamWaitStatus::Timeout;
}

auto StringBuilderStream::flush() -> StreamWriteStatus {
    return isOpen() ? StreamWriteStatus::Success : StreamWriteStatus::Timeout;
}

auto StringBuilderStream::close() -> StreamCloseStatus {
    const auto lock = std::scoped_lock{_mutex};
    _state = StreamState::Closed;
    return StreamCloseStatus::Closed;
}

void StringBuilderStream::abort() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    _state = StreamState::Closed;
}

auto StringBuilderStream::encoding() const noexcept -> text::StringEncoding {
    const auto lock = std::scoped_lock{_mutex};
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

auto StringBuilderStream::write(text::Char character) -> StreamWriteStatus {
    const auto lock = std::scoped_lock{_mutex};
    if (_state != StreamState::Open) {
        throwError("Failed to write to the string builder stream."_el, "The string builder stream is closed."_el);
    }
    _builder.append(character);
    return StreamWriteStatus::Success;
}

auto StringBuilderStream::write(const text::StringView &text) -> StreamWriteStatus {
    const auto lock = std::scoped_lock{_mutex};
    if (_state != StreamState::Open) {
        throwError("Failed to write to the string builder stream."_el, "The string builder stream is closed."_el);
    }
    _builder.append(text);
    return StreamWriteStatus::Success;
}

auto StringBuilderStream::writeLine() -> StreamWriteStatus {
    const auto lock = std::scoped_lock{_mutex};
    if (_state != StreamState::Open) {
        throwError("Failed to write to the string builder stream."_el, "The string builder stream is closed."_el);
    }
    _builder.append(U'\n');
    return StreamWriteStatus::Success;
}

auto StringBuilderStream::writeLine(const text::StringView &text) -> StreamWriteStatus {
    const auto lock = std::scoped_lock{_mutex};
    if (_state != StreamState::Open) {
        throwError("Failed to write to the string builder stream."_el, "The string builder stream is closed."_el);
    }
    _builder.append(text);
    _builder.append(U'\n');
    return StreamWriteStatus::Success;
}

}
