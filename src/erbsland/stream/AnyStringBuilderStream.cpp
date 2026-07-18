// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AnyStringBuilderStream.hpp"

#include "../text/AnyStringEditor.hpp"
#include "../text/Literals.hpp"
#include "../text/u16/U16StringEditor.hpp"
#include "../text/u32/U32StringEditor.hpp"
#include "../text/u8/U8StringEditor.hpp"

namespace erbsland::stream {

using namespace text::literals;

using namespace text;

AnyStringBuilderStream::AnyStringBuilderStream(StringKind stringKind, ConstructionToken) : _builder{stringKind} {
}

auto AnyStringBuilderStream::kind() const noexcept -> StringKind {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.kind();
}
auto AnyStringBuilderStream::length() const noexcept -> unit::CpLength {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.length();
}
auto AnyStringBuilderStream::isEmpty() const noexcept -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.isEmpty();
}
void AnyStringBuilderStream::clear() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    _builder.clear();
}
auto AnyStringBuilderStream::toU8String() const -> U8String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toU8String();
}
auto AnyStringBuilderStream::toString() const -> U8String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toString();
}

auto AnyStringBuilderStream::toU16String() const -> U16String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toU16String();
}

auto AnyStringBuilderStream::toU32String() const -> U32String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toU32String();
}

auto AnyStringBuilderStream::toAnyString() const -> AnyString {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toAnyString();
}

auto AnyStringBuilderStream::toU8StringEditor() const -> U8StringEditor {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toU8StringEditor();
}

auto AnyStringBuilderStream::toStringEditor() const -> StringEditor {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toStringEditor();
}

auto AnyStringBuilderStream::toU16StringEditor() const -> U16StringEditor {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toU16StringEditor();
}

auto AnyStringBuilderStream::toU32StringEditor() const -> U32StringEditor {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toU32StringEditor();
}

auto AnyStringBuilderStream::toAnyStringEditor() const -> AnyStringEditor {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.toAnyStringEditor();
}

auto AnyStringBuilderStream::takeU8String() -> U8String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeU8String();
}

auto AnyStringBuilderStream::takeString() -> String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeString();
}

auto AnyStringBuilderStream::takeU16String() -> U16String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeU16String();
}

auto AnyStringBuilderStream::takeU32String() -> U32String {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeU32String();
}

auto AnyStringBuilderStream::takeAnyString() -> AnyString {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeAnyString();
}

auto AnyStringBuilderStream::takeU8StringEditor() -> U8StringEditor {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeU8StringEditor();
}

auto AnyStringBuilderStream::takeStringEditor() -> StringEditor {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeStringEditor();
}

auto AnyStringBuilderStream::takeU16StringEditor() -> U16StringEditor {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeU16StringEditor();
}

auto AnyStringBuilderStream::takeU32StringEditor() -> U32StringEditor {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeU32StringEditor();
}

auto AnyStringBuilderStream::takeAnyStringEditor() -> AnyStringEditor {
    const auto lock = std::scoped_lock{_mutex};
    return _builder.takeAnyStringEditor();
}

auto AnyStringBuilderStream::state() const noexcept -> StreamState {
    const auto lock = std::scoped_lock{_mutex};
    return _state;
}

auto AnyStringBuilderStream::isReady() const noexcept -> bool {
    const auto lock = std::scoped_lock{_mutex};
    return _state == StreamState::Open;
}

auto AnyStringBuilderStream::waitForReady() -> StreamWaitStatus {
    return isReady() ? StreamWaitStatus::Ready : StreamWaitStatus::Timeout;
}

auto AnyStringBuilderStream::flush() -> StreamWriteStatus {
    return isOpen() ? StreamWriteStatus::Success : StreamWriteStatus::Timeout;
}

auto AnyStringBuilderStream::close() -> StreamCloseStatus {
    const auto lock = std::scoped_lock{_mutex};
    _state = StreamState::Closed;
    return StreamCloseStatus::Closed;
}

void AnyStringBuilderStream::abort() noexcept {
    const auto lock = std::scoped_lock{_mutex};
    _state = StreamState::Closed;
}

auto AnyStringBuilderStream::encoding() const noexcept -> StringEncoding {
    const auto lock = std::scoped_lock{_mutex};
    switch (_builder.kind()) {
    case StringKind::U8:
        return StringEncoding::Utf8;
    case StringKind::U16:
        return StringEncoding::Utf16;
    case StringKind::U32:
        return StringEncoding::Utf32;
    }
    return StringEncoding::Utf8;
}

auto AnyStringBuilderStream::effectiveEncoding() const noexcept -> StringEncoding {
    return encoding();
}

auto AnyStringBuilderStream::write(Char character) -> StreamWriteStatus {
    const auto lock = std::scoped_lock{_mutex};
    if (_state != StreamState::Open) {
        throwError("Failed to write to the string builder stream."_el, "The string builder stream is closed."_el);
    }
    _builder.append(character);
    return StreamWriteStatus::Success;
}

auto AnyStringBuilderStream::write(const String &text) -> StreamWriteStatus {
    const auto lock = std::scoped_lock{_mutex};
    if (_state != StreamState::Open) {
        throwError("Failed to write to the string builder stream."_el, "The string builder stream is closed."_el);
    }
    _builder.append(text);
    return StreamWriteStatus::Success;
}

auto AnyStringBuilderStream::writeLine() -> StreamWriteStatus {
    const auto lock = std::scoped_lock{_mutex};
    if (_state != StreamState::Open) {
        throwError("Failed to write to the string builder stream."_el, "The string builder stream is closed."_el);
    }
    _builder.append(U'\n');
    return StreamWriteStatus::Success;
}

auto AnyStringBuilderStream::writeLine(const String &text) -> StreamWriteStatus {
    const auto lock = std::scoped_lock{_mutex};
    if (_state != StreamState::Open) {
        throwError("Failed to write to the string builder stream."_el, "The string builder stream is closed."_el);
    }
    _builder.append(text);
    _builder.append(U'\n');
    return StreamWriteStatus::Success;
}

}
