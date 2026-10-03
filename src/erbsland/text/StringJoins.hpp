// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/StringJoinTools.hpp"

namespace erbsland::text {

/// Concatenate Core strings, editors, and literals of one encoding without a separator.
/// @seedoc{/reference/text/strings}
/// @tparam tFirst The first concrete Core string, editor, or literal type.
/// @tparam tRest The remaining input types, all with the same encoding as the first.
/// @param first The first part; a single input shares its storage with the result.
/// @param rest The remaining parts in output order; inputs are not modified.
/// @return An owning read-only string of the input encoding, preserving UTF-8 sensitivity.
/// @tested{StringJoinsTest}
template <impl::StringJoinPart tFirst, impl::StringJoinPart... tRest>
    requires(std::same_as<impl::JoinedStringFor<tFirst>, impl::JoinedStringFor<tRest>> && ...)
[[nodiscard]] auto join(const tFirst &first, const tRest &...rest) -> impl::JoinedStringFor<tFirst> {
    using Result = impl::JoinedStringFor<tFirst>;
    if constexpr (sizeof...(tRest) == 0) {
        return Result{first};
    } else {
        return Result::fromJoined({Result{first}, Result{rest}...});
    }
}

/// Concatenate a fixed list of UTF-8 strings, editors, or literals without a separator.
/// @seedoc{/reference/text/strings}
/// @tparam tSize The positive number of parts, deduced from the braced list or fixed array.
/// @param parts The same-encoding parts in output order; inputs are not modified.
/// @return An owning UTF-8 string preserving UTF-8 sensitivity.
/// @tested{StringJoinsTest}
template <std::size_t tSize>
[[nodiscard]] auto join(const U8String (&parts)[tSize]) -> U8String {
    return impl::joinStringArray(parts, std::make_index_sequence<tSize>{});
}

/// Concatenate a fixed list of UTF-16 strings, editors, or literals without a separator.
/// @seedoc{/reference/text/strings}
/// @tparam tSize The positive number of parts, deduced from the braced list or fixed array.
/// @param parts The same-encoding parts in output order; inputs are not modified.
/// @return An owning UTF-16 string.
/// @tested{StringJoinsTest}
template <std::size_t tSize>
[[nodiscard]] auto join(const U16String (&parts)[tSize]) -> U16String {
    return impl::joinStringArray(parts, std::make_index_sequence<tSize>{});
}

/// Concatenate a fixed list of UTF-32 strings, editors, or literals without a separator.
/// @seedoc{/reference/text/strings}
/// @tparam tSize The positive number of parts, deduced from the braced list or fixed array.
/// @param parts The same-encoding parts in output order; inputs are not modified.
/// @return An owning UTF-32 string.
/// @tested{StringJoinsTest}
template <std::size_t tSize>
[[nodiscard]] auto join(const U32String (&parts)[tSize]) -> U32String {
    return impl::joinStringArray(parts, std::make_index_sequence<tSize>{});
}

}
