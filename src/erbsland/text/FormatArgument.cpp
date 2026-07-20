// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatArgument.hpp"

namespace erbsland::text {

auto FormatArgument::kind() const noexcept -> FormatArgumentKind {
    return static_cast<FormatArgumentKind>(_value.index());
}

auto FormatArgument::u8Text() const -> U8String {
    return std::get<U8String>(_value);
}

auto FormatArgument::u16Text() const -> U16String {
    return std::get<U16String>(_value);
}

auto FormatArgument::u32Text() const -> U32String {
    return std::get<U32String>(_value);
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

auto FormatArgument::bytes() const -> mem::ByteBlock {
    return std::get<mem::ByteBlock>(_value);
}

}
