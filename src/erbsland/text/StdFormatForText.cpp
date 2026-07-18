// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StdFormatForText.hpp"

#include "StringConverter.hpp"

#include "u32/U32StringEditor.hpp"
#include "u8/U8StringEditor.hpp"

using namespace erbsland::text;

auto std::formatter<U8StringEditor, char>::format(const U8StringEditor &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<U8String, char>::format(const U8String &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<U16StringEditor, char>::format(const U16StringEditor &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<U16String, char>::format(const U16String &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<U32StringEditor, char>::format(const U32StringEditor &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<U32String, char>::format(const U32String &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<Char, char>::format(const Char &character, std::format_context &ctx) const
    -> std::format_context::iterator {
    const auto text = U8String::fromCharacter(character);
    return std::formatter<std::string>::format(StringConverter{text}.toStdString(), ctx);
}
