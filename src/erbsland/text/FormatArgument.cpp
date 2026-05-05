// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatArgument.hpp"

namespace erbsland::text {

auto FormatArgument::kind() const noexcept -> FormatArgumentKind {
    return static_cast<FormatArgumentKind>(_value.index());
}

auto FormatArgument::u8Text() const -> U8StringView {
    return std::get<U8StringView>(_value);
}

auto FormatArgument::u16Text() const -> U16StringView {
    return std::get<U16StringView>(_value);
}

auto FormatArgument::u32Text() const -> U32StringView {
    return std::get<U32StringView>(_value);
}

auto FormatArgument::signedInteger() const -> int64_t {
    return std::get<int64_t>(_value);
}

auto FormatArgument::unsignedInteger() const -> uint64_t {
    return std::get<uint64_t>(_value);
}

auto FormatArgument::floatingPoint() const -> double {
    return std::get<double>(_value);
}

auto FormatArgument::boolean() const -> bool {
    return std::get<bool>(_value);
}

auto FormatArgument::character() const -> Char {
    return std::get<Char>(_value);
}

}
