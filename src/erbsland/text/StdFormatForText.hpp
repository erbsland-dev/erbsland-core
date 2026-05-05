// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char_fwd.hpp"

#include "u16/U16String_fwd.hpp"
#include "u16/U16StringView_fwd.hpp"
#include "u32/U32String_fwd.hpp"
#include "u32/U32StringView_fwd.hpp"
#include "u8/U8String_fwd.hpp"
#include "u8/U8StringView_fwd.hpp"

#include <format>

template <>
struct std::formatter<erbsland::text::U8String> : std::formatter<std::string> {
    auto format(const erbsland::text::U8String &str, std::format_context &ctx) const -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::U8StringView> : std::formatter<std::string> {
    auto format(const erbsland::text::U8StringView &str, std::format_context &ctx) const
        -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::U16String> : std::formatter<std::string> {
    auto format(const erbsland::text::U16String &str, std::format_context &ctx) const -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::U16StringView> : std::formatter<std::string> {
    auto format(const erbsland::text::U16StringView &str, std::format_context &ctx) const
        -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::U32String> : std::formatter<std::string> {
    auto format(const erbsland::text::U32String &str, std::format_context &ctx) const -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::U32StringView> : std::formatter<std::string> {
    auto format(const erbsland::text::U32StringView &str, std::format_context &ctx) const
        -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::text::Char> : std::formatter<std::string> {
    auto format(const erbsland::text::Char &str, std::format_context &ctx) const -> std::format_context::iterator;
};
