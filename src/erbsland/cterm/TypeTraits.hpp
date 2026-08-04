// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block_fwd.hpp"
#include "BlockAttributes_fwd.hpp"
#include "BlockString_fwd.hpp"
#include "BlockStringEditor_fwd.hpp"
#include "BlockStyle_fwd.hpp"
#include "Color.hpp"

#include "../text/String.hpp"
#include "../text/StringEditor.hpp"
#include "../text/StringLiteral.hpp"
#include "../text/u32/U32String.hpp"
#include "../text/u32/U32StringEditor.hpp"
#include "../text/u32/U32StringLiteral.hpp"
#include "../text/u8/U8StringLiteral.hpp"

#include <concepts>
#include <type_traits>

namespace erbsland::cterm {

/// A value accepted by `Terminal::print()` and `Terminal::printLine()` or `BlockStringEditor::append()`.
template <typename T>
concept PrintableArg = std::constructible_from<Foreground, T> || std::constructible_from<Background, T> ||
    std::same_as<Color, std::remove_cvref_t<T>> || std::same_as<BlockAttributes, std::remove_cvref_t<T>> ||
    std::constructible_from<BlockAttributes, T> || std::same_as<BlockStyle, std::remove_cvref_t<T>> ||
    std::same_as<Block, std::remove_cvref_t<T>> || std::same_as<BlockStringEditor, std::remove_cvref_t<T>> ||
    std::same_as<BlockString, std::remove_cvref_t<T>> || std::same_as<text::StringEditor, std::remove_cvref_t<T>> ||
    std::same_as<text::String, std::remove_cvref_t<T>> || std::same_as<text::StringLiteral, std::remove_cvref_t<T>> ||
    std::same_as<text::U32StringEditor, std::remove_cvref_t<T>> ||
    std::same_as<text::U32String, std::remove_cvref_t<T>> ||
    std::same_as<text::U32StringLiteral, std::remove_cvref_t<T>> ||
    std::same_as<text::U8StringLiteral<char>, std::remove_cvref_t<T>> ||
    std::same_as<text::U8StringLiteral<char8_t>, std::remove_cvref_t<T>>;

template <typename T>
concept ColorArg = std::constructible_from<Color, T>;

/// Constructor arguments that can build a `Color` without matching the explicit `BlockAttributes` overloads.
template <typename... T>
concept CharColorConstructorArgs = sizeof...(T) >= 1 && std::constructible_from<Color, T...> &&
    (!std::same_as<std::remove_cvref_t<T>, BlockAttributes> && ...);

}
