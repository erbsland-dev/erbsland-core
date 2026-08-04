// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatTraits.hpp"

#include "../FormatArgument.hpp"
#include "../FormatAs.hpp"
#include "../u16/U16String.hpp"
#include "../u16/U16StringEditor.hpp"
#include "../u16/U16StringLiteral.hpp"
#include "../u32/U32String.hpp"
#include "../u32/U32StringEditor.hpp"
#include "../u32/U32StringLiteral.hpp"
#include "../u8/U8String.hpp"
#include "../u8/U8StringEditor.hpp"
#include "../u8/U8StringLiteral.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../unit/ExitCode.hpp"
#include "../../unit/IntegerUnitAmount.hpp"
#include "../../unit/IntegerUnitIndex.hpp"
#include "../../unit/IntegerUnitOffset.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

namespace erbsland::text::impl {

/// Always `false`, to produce a deferred static assertion for unsupported types.
template <typename>
inline constexpr auto cCannotMakeDefaultFormatArgument = false;

/// Convert an integer unit index to an unsigned runtime format argument.
/// @tested{FormatAsTest}
template <unit::impl::ValidIntegerUnit tIntegerUnit>
[[nodiscard]] auto makeDefaultFormatArgument(const unit::IntegerUnitIndex<tIntegerUnit> value) -> FormatArgument {
    return FormatArgument{static_cast<uint64_t>(value.toRawValue())};
}

/// Convert an integer unit amount to an unsigned runtime format argument.
/// @tested{FormatAsTest}
template <unit::impl::ValidIntegerUnit tIntegerUnit>
[[nodiscard]] auto makeDefaultFormatArgument(const unit::IntegerUnitAmount<tIntegerUnit> value) -> FormatArgument {
    return FormatArgument{static_cast<uint64_t>(value.toRawValue())};
}

/// Convert an integer unit offset to a signed runtime format argument.
/// @tested{FormatAsTest}
template <unit::impl::ValidIntegerUnit tIntegerUnit>
[[nodiscard]] auto makeDefaultFormatArgument(const unit::IntegerUnitOffset<tIntegerUnit> value) -> FormatArgument {
    return FormatArgument{static_cast<int64_t>(value.toRawValue())};
}

/// Convert a built-in formatting value to a runtime format argument.
/// @tested{FormatAsTest}
template <typename T>
[[nodiscard]] auto makeDefaultFormatArgument(const T &value) -> FormatArgument {
    if constexpr (std::same_as<T, bool>) {
        return FormatArgument{value};
    } else if constexpr (FormatSignedIntegerArgument<T>) {
        return FormatArgument{static_cast<int64_t>(math::toNativeInteger(value))};
    } else if constexpr (FormatUnsignedIntegerArgument<T>) {
        return FormatArgument{static_cast<uint64_t>(math::toNativeInteger(value))};
    } else if constexpr (std::same_as<T, unit::ExitCode>) {
        return FormatArgument{static_cast<int64_t>(value.toRawValue())};
    } else if constexpr (std::same_as<T, float>) {
        return FormatArgument{static_cast<double>(value)};
    } else if constexpr (std::same_as<T, double>) {
        return FormatArgument{value};
    } else if constexpr (std::same_as<T, Char>) {
        return FormatArgument{value};
    } else if constexpr (FormatCharacterArgument<T>) {
        return FormatArgument{Char{static_cast<char32_t>(value)}};
    } else if constexpr (std::same_as<T, U8String>) {
        return FormatArgument{value};
    } else if constexpr (std::same_as<T, U8StringEditor>) {
        return FormatArgument{U8String{value}};
    } else if constexpr (std::same_as<T, U8StringLiteral<char>> || std::same_as<T, U8StringLiteral<char8_t>>) {
        return FormatArgument{U8String{value}};
    } else if constexpr (std::same_as<T, std::string_view>) {
        return FormatArgument{U8String{U8StringEditor{value}}};
    } else if constexpr (std::same_as<T, std::string>) {
        return FormatArgument{U8String{U8StringEditor{std::string_view{value}}}};
    } else if constexpr (std::same_as<T, std::u8string_view>) {
        return FormatArgument{U8String{U8StringEditor{value}}};
    } else if constexpr (std::same_as<T, std::u8string>) {
        return FormatArgument{U8String{U8StringEditor{std::u8string_view{value}}}};
    } else if constexpr (std::is_array_v<T> && std::same_as<std::remove_extent_t<T>, char>) {
        return FormatArgument{U8String{U8StringEditor{std::string_view{value, std::extent_v<T> - 1U}}}};
    } else if constexpr (std::is_array_v<T> && std::same_as<std::remove_extent_t<T>, char8_t>) {
        return FormatArgument{U8String{U8StringEditor{std::u8string_view{value, std::extent_v<T> - 1U}}}};
    } else if constexpr (std::same_as<T, U16String>) {
        return FormatArgument{value};
    } else if constexpr (std::same_as<T, U16StringEditor>) {
        return FormatArgument{U16String{value}};
    } else if constexpr (std::same_as<T, U16StringLiteral>) {
        return FormatArgument{U16String{value}};
    } else if constexpr (std::same_as<T, std::u16string_view>) {
        return FormatArgument{U16String{U16StringEditor{value}}};
    } else if constexpr (std::same_as<T, std::u16string>) {
        return FormatArgument{U16String{U16StringEditor{std::u16string_view{value}}}};
    } else if constexpr (std::is_array_v<T> && std::same_as<std::remove_extent_t<T>, char16_t>) {
        return FormatArgument{U16String{U16StringEditor{std::u16string_view{value, std::extent_v<T> - 1U}}}};
    } else if constexpr (std::same_as<T, U32String>) {
        return FormatArgument{value};
    } else if constexpr (std::same_as<T, U32StringEditor>) {
        return FormatArgument{U32String{value}};
    } else if constexpr (std::same_as<T, U32StringLiteral>) {
        return FormatArgument{U32String{value}};
    } else if constexpr (std::same_as<T, std::u32string_view>) {
        return FormatArgument{U32String{U32StringEditor{value}}};
    } else if constexpr (std::same_as<T, std::u32string>) {
        return FormatArgument{U32String{U32StringEditor{std::u32string_view{value}}}};
    } else if constexpr (std::is_array_v<T> && std::same_as<std::remove_extent_t<T>, char32_t>) {
        return FormatArgument{U32String{U32StringEditor{std::u32string_view{value, std::extent_v<T> - 1U}}}};
    } else if constexpr (std::same_as<T, mem::ByteBlock>) {
        return FormatArgument{value};
    } else {
        static_assert(
            cCannotMakeDefaultFormatArgument<T>, "Unsupported format argument type. Add a FormatAs specialization.");
    }
}

}
