// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace erbsland::util::impl {

/// Trait type for enum flags validation and type extraction.
/// @tparam tType The type to check.
/// @tparam tIsEnum Whether the type is an enum.
template <typename tType, bool tIsEnum = std::is_enum_v<tType>>
struct EnumFlagsTraits {
    static constexpr auto cIsValid = false;
};

/// Specialization for enum types.
/// @tparam tType The enum type.
template <typename tType>
struct EnumFlagsTraits<tType, true> {
    using Value = std::underlying_type_t<tType>;

    static constexpr auto cIsScoped = !std::is_convertible_v<tType, Value>;
    static constexpr auto cIsUnsigned = std::is_unsigned_v<Value>;
    static constexpr auto cIsValid = cIsScoped && cIsUnsigned;
};

/// Concept requiring a valid enum flags type (scoped, unsigned underlying type).
/// @tparam tType The type to check.
template <typename tType>
concept EnumFlagsEnum = EnumFlagsTraits<tType>::cIsValid;

/// Concept requiring a valid enum flags type with an `All` member.
/// @tparam tType The type to check.
template <typename tType>
concept EnumFlagsEnumWithAll = EnumFlagsEnum<tType> && requires {
    { tType::All } -> std::same_as<tType>;
};

/// Convert an enum flag to its raw underlying value.
/// @tparam tEnum The enum type.
/// @param value The enum flag value.
/// @return The raw underlying value.
template <EnumFlagsEnum tEnum>
[[nodiscard]] constexpr auto enumFlagsRawValue(tEnum value) noexcept -> typename EnumFlagsTraits<tEnum>::Value {
    return static_cast<typename EnumFlagsTraits<tEnum>::Value>(value);
}

}
