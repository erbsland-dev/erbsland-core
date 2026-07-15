// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AnsiSequence.hpp"

#include "../../text/StringFormat.hpp"

namespace erbsland::cterm::impl::ansi_sequence {

auto color(const int code) -> text::String {
    static const auto cFormat = text::StringFormat{"\x1b[{}m"};
    return cFormat.build(code);
}

auto color(const int foregroundCode, const int backgroundCode) -> text::String {
    static const auto cFormat = text::StringFormat{"\x1b[{};{}m"};
    return cFormat.build(foregroundCode, backgroundCode);
}

auto cursorPosition(const int row, const int column) -> text::String {
    static const auto cFormat = text::StringFormat{"\x1b[{};{}H"};
    return cFormat.build(row, column);
}

auto moveLeft(const int count) -> text::String {
    static const auto cFormat = text::StringFormat{"\x1b[{}D"};
    return cFormat.build(count);
}

auto moveRight(const int count) -> text::String {
    static const auto cFormat = text::StringFormat{"\x1b[{}C"};
    return cFormat.build(count);
}

auto moveUp(const int count) -> text::String {
    static const auto cFormat = text::StringFormat{"\x1b[{}A"};
    return cFormat.build(count);
}

auto moveDown(const int count) -> text::String {
    static const auto cFormat = text::StringFormat{"\x1b[{}B"};
    return cFormat.build(count);
}

}
