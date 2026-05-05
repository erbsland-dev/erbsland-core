// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardTextOutputStream.hpp"

#include "../../err/StreamError.hpp"

#include <string_view>
#include <utility>

namespace erbsland::stream::impl {

StandardTextOutputStream::StandardTextOutputStream(NativeOutputStreamPtr nativeOutputStream) :
    _nativeOutputStream{std::move(nativeOutputStream)} {
    if (!_nativeOutputStream) {
        throw err::StreamError{"Native output stream is missing."};
    }
}

auto StandardTextOutputStream::encoding() const noexcept -> text::StringEncoding {
    return text::StringEncoding::Utf8;
}

auto StandardTextOutputStream::effectiveEncoding() const noexcept -> text::StringEncoding {
    return text::StringEncoding::Utf8;
}

auto StandardTextOutputStream::isOpen() const noexcept -> bool {
    return true;
}

void StandardTextOutputStream::flush() {
    const auto lock = std::lock_guard{_mutex};
    _nativeOutputStream->flush();
}

void StandardTextOutputStream::close() {
}

void StandardTextOutputStream::write(const text::Char character) {
    const auto text = text::String::fromCharacter(character);

    const auto lock = std::lock_guard{_mutex};
    _nativeOutputStream->writeText(text);
}

void StandardTextOutputStream::write(const text::StringView &text) {
    const auto lock = std::lock_guard{_mutex};
    _nativeOutputStream->writeText(text);
}

void StandardTextOutputStream::writeLine() {
    constexpr auto lineEnding = std::string_view{"\n"};

    const auto lock = std::lock_guard{_mutex};
    _nativeOutputStream->writeBytes(std::span<const char>{lineEnding.data(), lineEnding.size()});
}

void StandardTextOutputStream::writeLine(const text::StringView &text) {
    constexpr auto lineEnding = std::string_view{"\n"};

    const auto lock = std::lock_guard{_mutex};
    _nativeOutputStream->writeText(text);
    _nativeOutputStream->writeBytes(std::span<const char>{lineEnding.data(), lineEnding.size()});
}

}
