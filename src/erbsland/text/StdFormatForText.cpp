// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StdFormatForText.hpp"

#include "StringConverter.hpp"

#include "u32/U32String.hpp"
#include "u8/U8String.hpp"

using namespace erbsland::text;

auto std::formatter<U8String, char>::format(const U8String &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<U8StringView, char>::format(const U8StringView &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<U16String, char>::format(const U16String &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<U16StringView, char>::format(const U16StringView &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<U32String, char>::format(const U32String &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<U32StringView, char>::format(const U32StringView &str, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(StringConverter{str}.toStdString(), ctx);
}

auto std::formatter<Char, char>::format(const Char &character, std::format_context &ctx) const
    -> std::format_context::iterator {
    const auto text = U8String::fromCharacter(character);
    return std::formatter<std::string>::format(StringConverter{text}.toStdString(), ctx);
}
