// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaseSensitivity.hpp"
#include "Char.hpp"
#include "StringConverter.hpp"

#include "u16/U16String.hpp"
#include "u16/U16StringEditor.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringEditor.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringEditor.hpp"

#include <format>
#include <string>

template <>
struct std::formatter<erbsland::text::U8StringEditor> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::text::U8StringEditor &str, std::format_context &ctx) const
        -> std::format_context::iterator {
        return Base::format(erbsland::text::StringConverter{str}.toStdString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::text::U8String> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::text::U8String &str, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(erbsland::text::StringConverter{str}.toStdString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::text::U16StringEditor> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::text::U16StringEditor &str, std::format_context &ctx) const
        -> std::format_context::iterator {
        return Base::format(erbsland::text::StringConverter{str}.toStdString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::text::U16String> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::text::U16String &str, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(erbsland::text::StringConverter{str}.toStdString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::text::U32StringEditor> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::text::U32StringEditor &str, std::format_context &ctx) const
        -> std::format_context::iterator {
        return Base::format(erbsland::text::StringConverter{str}.toStdString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::text::U32String> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::text::U32String &str, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(erbsland::text::StringConverter{str}.toStdString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::text::Char> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::text::Char &character, std::format_context &ctx) const
        -> std::format_context::iterator {
        const auto text = erbsland::text::U8String::fromCharacter(character);
        return Base::format(erbsland::text::StringConverter{text}.toStdString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::text::CaseSensitivity> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::text::CaseSensitivity value, std::format_context &ctx) const
        -> std::format_context::iterator {
        return Base::format(value.toString(), ctx);
    }
};
