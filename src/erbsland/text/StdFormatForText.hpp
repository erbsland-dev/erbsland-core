// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char_fwd.hpp"

#include "u16/U16String_fwd.hpp"
#include "u16/U16StringEditor_fwd.hpp"
#include "u32/U32String_fwd.hpp"
#include "u32/U32StringEditor_fwd.hpp"
#include "u8/U8String_fwd.hpp"
#include "u8/U8StringEditor_fwd.hpp"

#include <format>

template <>
struct std::formatter<erbsland::text::U8StringEditor> : std::formatter<std::string> {
    auto format(const erbsland::text::U8StringEditor &str, std::format_context &ctx) const
        -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::U8String> : std::formatter<std::string> {
    auto format(const erbsland::text::U8String &str, std::format_context &ctx) const -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::U16StringEditor> : std::formatter<std::string> {
    auto format(const erbsland::text::U16StringEditor &str, std::format_context &ctx) const
        -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::U16String> : std::formatter<std::string> {
    auto format(const erbsland::text::U16String &str, std::format_context &ctx) const -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::U32StringEditor> : std::formatter<std::string> {
    auto format(const erbsland::text::U32StringEditor &str, std::format_context &ctx) const
        -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::U32String> : std::formatter<std::string> {
    auto format(const erbsland::text::U32String &str, std::format_context &ctx) const -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::Char> : std::formatter<std::string> {
    auto format(const erbsland::text::Char &str, std::format_context &ctx) const -> std::format_context::iterator;
};
