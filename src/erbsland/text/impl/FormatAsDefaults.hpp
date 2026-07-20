// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../FormatAs.hpp"
#include "../impl/FormatTraits.hpp"
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace erbsland::text {

template <>
struct FormatAsBool<bool> : FormatAs<bool, bool> {
    [[nodiscard]] auto format(const bool value) const -> bool { return value; }
};

template <impl::FormatSignedIntegerArgument T>
struct FormatAsInt64<T> : FormatAs<T, int64_t> {
    [[nodiscard]] auto format(const T value) const -> int64_t {
        return static_cast<int64_t>(math::toNativeInteger(value));
    }
};

template <impl::FormatUnsignedIntegerArgument T>
struct FormatAsUInt64<T> : FormatAs<T, uint64_t> {
    [[nodiscard]] auto format(const T value) const -> uint64_t {
        return static_cast<uint64_t>(math::toNativeInteger(value));
    }
};

template <unit::impl::ValidIntegerUnit tIntegerUnit>
struct FormatAsUInt64<unit::IntegerUnitIndex<tIntegerUnit>> : FormatAs<unit::IntegerUnitIndex<tIntegerUnit>, uint64_t> {
    [[nodiscard]] auto format(const unit::IntegerUnitIndex<tIntegerUnit> value) const -> uint64_t {
        return static_cast<uint64_t>(value.toRawValue());
    }
};

template <unit::impl::ValidIntegerUnit tIntegerUnit>
struct FormatAsUInt64<unit::IntegerUnitAmount<tIntegerUnit>>
    : FormatAs<unit::IntegerUnitAmount<tIntegerUnit>, uint64_t> {
    [[nodiscard]] auto format(const unit::IntegerUnitAmount<tIntegerUnit> value) const -> uint64_t {
        return static_cast<uint64_t>(value.toRawValue());
    }
};

template <unit::impl::ValidIntegerUnit tIntegerUnit>
struct FormatAsInt64<unit::IntegerUnitOffset<tIntegerUnit>> : FormatAs<unit::IntegerUnitOffset<tIntegerUnit>, int64_t> {
    [[nodiscard]] auto format(const unit::IntegerUnitOffset<tIntegerUnit> value) const -> int64_t {
        return static_cast<int64_t>(value.toRawValue());
    }
};

template <>
struct FormatAsInt64<unit::ExitCode> : FormatAs<unit::ExitCode, int64_t> {
    [[nodiscard]] auto format(const unit::ExitCode value) const -> int64_t {
        return static_cast<int64_t>(value.toRawValue());
    }
};

template <>
struct FormatAsDouble<float> : FormatAs<float, double> {
    [[nodiscard]] auto format(const float value) const -> double { return static_cast<double>(value); }
};

template <>
struct FormatAsDouble<double> : FormatAs<double, double> {
    [[nodiscard]] auto format(const double value) const -> double { return value; }
};

template <>
struct FormatAsChar<Char> : FormatAs<Char, Char> {
    [[nodiscard]] auto format(const Char value) const -> Char { return value; }
};

template <impl::FormatCharacterArgument T>
struct FormatAsChar<T> : FormatAs<T, Char> {
    [[nodiscard]] auto format(const T value) const -> Char { return Char{static_cast<char32_t>(value)}; }
};

template <>
struct FormatAsU8Text<U8String> : FormatAs<U8String, U8String> {
    [[nodiscard]] auto format(const U8String &value) const -> U8String { return value; }
};

template <>
struct FormatAsU8Text<U8StringEditor> : FormatAs<U8StringEditor, U8String> {
    [[nodiscard]] auto format(const U8StringEditor &value) const -> U8String { return value; }
};

template <>
struct FormatAsU8Text<U8StringLiteral<char>> : FormatAs<U8StringLiteral<char>, U8String> {
    [[nodiscard]] auto format(const U8StringLiteral<char> &value) const -> U8String { return value; }
};

template <>
struct FormatAsU8Text<U8StringLiteral<char8_t>> : FormatAs<U8StringLiteral<char8_t>, U8String> {
    [[nodiscard]] auto format(const U8StringLiteral<char8_t> &value) const -> U8String { return value; }
};

template <>
struct FormatAsU8Text<std::string_view> : FormatAs<std::string_view, U8StringEditor> {
    [[nodiscard]] auto format(const std::string_view value) const -> U8StringEditor { return U8StringEditor{value}; }
};

template <>
struct FormatAsU8Text<std::string> : FormatAs<std::string, U8StringEditor> {
    [[nodiscard]] auto format(const std::string &value) const -> U8StringEditor {
        return U8StringEditor{std::string_view{value}};
    }
};

template <>
struct FormatAsU8Text<std::u8string_view> : FormatAs<std::u8string_view, U8StringEditor> {
    [[nodiscard]] auto format(const std::u8string_view value) const -> U8StringEditor { return U8StringEditor{value}; }
};

template <>
struct FormatAsU8Text<std::u8string> : FormatAs<std::u8string, U8StringEditor> {
    [[nodiscard]] auto format(const std::u8string &value) const -> U8StringEditor {
        return U8StringEditor{std::u8string_view{value}};
    }
};

template <std::size_t N>
struct FormatAsU8Text<char[N]> : FormatAs<char[N], U8String> {
    [[nodiscard]] auto format(const char (&value)[N]) const -> U8String { return U8StringLiteral{value, N - 1U}; }
};

template <std::size_t N>
struct FormatAsU8Text<char8_t[N]> : FormatAs<char8_t[N], U8String> {
    [[nodiscard]] auto format(const char8_t (&value)[N]) const -> U8String { return U8StringLiteral{value, N - 1U}; }
};

template <>
struct FormatAsU16Text<U16String> : FormatAs<U16String, U16String> {
    [[nodiscard]] auto format(const U16String &value) const -> U16String { return value; }
};

template <>
struct FormatAsU16Text<U16StringEditor> : FormatAs<U16StringEditor, U16String> {
    [[nodiscard]] auto format(const U16StringEditor &value) const -> U16String { return value; }
};

template <>
struct FormatAsU16Text<U16StringLiteral> : FormatAs<U16StringLiteral, U16String> {
    [[nodiscard]] auto format(const U16StringLiteral &value) const -> U16String { return value; }
};

template <>
struct FormatAsU16Text<std::u16string_view> : FormatAs<std::u16string_view, U16StringEditor> {
    [[nodiscard]] auto format(const std::u16string_view value) const -> U16StringEditor {
        return U16StringEditor{value};
    }
};

template <>
struct FormatAsU16Text<std::u16string> : FormatAs<std::u16string, U16StringEditor> {
    [[nodiscard]] auto format(const std::u16string &value) const -> U16StringEditor {
        return U16StringEditor{std::u16string_view{value}};
    }
};

template <std::size_t N>
struct FormatAsU16Text<char16_t[N]> : FormatAs<char16_t[N], U16String> {
    [[nodiscard]] auto format(const char16_t (&value)[N]) const -> U16String { return U16StringLiteral{value, N - 1U}; }
};

template <>
struct FormatAsU32Text<U32String> : FormatAs<U32String, U32String> {
    [[nodiscard]] auto format(const U32String &value) const -> U32String { return value; }
};

template <>
struct FormatAsU32Text<U32StringEditor> : FormatAs<U32StringEditor, U32String> {
    [[nodiscard]] auto format(const U32StringEditor &value) const -> U32String { return value; }
};

template <>
struct FormatAsU32Text<U32StringLiteral> : FormatAs<U32StringLiteral, U32String> {
    [[nodiscard]] auto format(const U32StringLiteral &value) const -> U32String { return value; }
};

template <>
struct FormatAsU32Text<std::u32string_view> : FormatAs<std::u32string_view, U32StringEditor> {
    [[nodiscard]] auto format(const std::u32string_view value) const -> U32StringEditor {
        return U32StringEditor{value};
    }
};

template <>
struct FormatAsU32Text<std::u32string> : FormatAs<std::u32string, U32StringEditor> {
    [[nodiscard]] auto format(const std::u32string &value) const -> U32StringEditor {
        return U32StringEditor{std::u32string_view{value}};
    }
};

template <std::size_t N>
struct FormatAsU32Text<char32_t[N]> : FormatAs<char32_t[N], U32String> {
    [[nodiscard]] auto format(const char32_t (&value)[N]) const -> U32String { return U32StringLiteral{value, N - 1U}; }
};

template <>
struct FormatAsBytes<mem::ByteBlock> : FormatAs<mem::ByteBlock, mem::ByteBlock> {
    [[nodiscard]] auto format(const mem::ByteBlock &value) const -> mem::ByteBlock { return value; }
};

}
