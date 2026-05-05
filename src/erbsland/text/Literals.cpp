// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Literals.hpp"

#include "u16/U16String.hpp"
#include "u16/U16StringView.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringView.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringView.hpp"

namespace erbsland::text::literals {

auto operator""_elv(const char *data, const std::size_t size) noexcept -> U8StringView {
    return U8StringView{impl::createU8StringLiteral(data, size)};
}

auto operator""_elv(const char8_t *data, const std::size_t size) noexcept -> U8StringView {
    return U8StringView{impl::createU8StringLiteral(data, size)};
}

auto operator""_els(const char *data, const std::size_t size) -> U8String {
    return U8String{impl::createU8StringLiteral(data, size)};
}

auto operator""_els(const char8_t *data, const std::size_t size) -> U8String {
    return U8String{impl::createU8StringLiteral(data, size)};
}

auto operator""_elv(const char16_t *data, const std::size_t size) noexcept -> U16StringView {
    return U16StringView{impl::createU16StringLiteral(data, size)};
}

auto operator""_els(const char16_t *data, const std::size_t size) -> U16String {
    return U16String{impl::createU16StringLiteral(data, size)};
}

auto operator""_elv(const char32_t *data, const std::size_t size) noexcept -> U32StringView {
    return U32StringView{impl::createU32StringLiteral(data, size)};
}

auto operator""_els(const char32_t *data, const std::size_t size) -> U32String {
    return U32String{impl::createU32StringLiteral(data, size)};
}

}
