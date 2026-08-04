// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WidthTraits_fwd.hpp"

namespace app::string::impl {

/// Provide UTF-8 string operations for the profiler.
template <>
struct WidthTraits<StringWidth::U8> {
    using String = el::U8String;
    using Editor = el::U8StringEditor;
    using DataIndex = el::ByteIndex;
    using DataLength = el::ByteLength;
    using DataRange = el::ByteRange;
    static constexpr auto Width = StringWidth::U8;
    static constexpr auto UnitBytes = std::uint64_t{1U};

    /// Get the UTF-8 data length.
    [[nodiscard]] static auto dataLength(const auto &value) -> std::uint64_t { return value.length().toRawValue(); }
    /// Convert a character index into a UTF-8 data index.
    [[nodiscard]] static auto dataIndex(const auto &value, const std::uint64_t cpIndex) -> DataIndex {
        return value.indexAt(el::CpIndex::fromSizeT(static_cast<std::size_t>(cpIndex)));
    }
    /// Create a UTF-8 data range from character positions.
    [[nodiscard]] static auto dataRange(const auto &value, const std::uint64_t cpStart, const std::uint64_t cpCount)
        -> DataRange {
        return DataRange{dataIndex(value, cpStart), dataIndex(value, cpStart + cpCount)};
    }
    /// Convert a UTF-8 data index into a character index.
    [[nodiscard]] static auto characterIndex(const auto &value, const DataIndex index) -> std::uint64_t {
        return value.toCharIndex(index).toRawValue();
    }
    /// Test whether a UTF-8 value is valid.
    [[nodiscard]] static auto isValid(const auto &value) noexcept -> bool { return value.isValidUtf8(); }
};

/// Provide UTF-16 string operations for the profiler.
template <>
struct WidthTraits<StringWidth::U16> {
    using String = el::U16String;
    using Editor = el::U16StringEditor;
    using DataIndex = el::U16DataIndex;
    using DataLength = el::U16DataLength;
    using DataRange = el::U16DataRange;
    static constexpr auto Width = StringWidth::U16;
    static constexpr auto UnitBytes = std::uint64_t{2U};

    /// Get the UTF-16 data length.
    [[nodiscard]] static auto dataLength(const auto &value) -> std::uint64_t { return value.length().toRawValue(); }
    /// Convert a character index into a UTF-16 data index.
    [[nodiscard]] static auto dataIndex(const auto &value, const std::uint64_t cpIndex) -> DataIndex {
        return value.indexAt(el::CpIndex::fromSizeT(static_cast<std::size_t>(cpIndex)));
    }
    /// Create a UTF-16 data range from character positions.
    [[nodiscard]] static auto dataRange(const auto &value, const std::uint64_t cpStart, const std::uint64_t cpCount)
        -> DataRange {
        return DataRange{dataIndex(value, cpStart), dataIndex(value, cpStart + cpCount)};
    }
    /// Convert a UTF-16 data index into a character index.
    [[nodiscard]] static auto characterIndex(const auto &value, const DataIndex index) -> std::uint64_t {
        return value.toCharIndex(index).toRawValue();
    }
    /// Test whether a UTF-16 value is valid.
    [[nodiscard]] static auto isValid(const auto &value) noexcept -> bool { return value.isValidUtf16(); }
};

/// Provide UTF-32 string operations for the profiler.
template <>
struct WidthTraits<StringWidth::U32> {
    using String = el::U32String;
    using Editor = el::U32StringEditor;
    using DataIndex = el::CpIndex;
    using DataLength = el::CpLength;
    using DataRange = el::CpRange;
    static constexpr auto Width = StringWidth::U32;
    static constexpr auto UnitBytes = std::uint64_t{4U};

    /// Get the UTF-32 data length.
    [[nodiscard]] static auto dataLength(const auto &value) -> std::uint64_t { return value.length().toRawValue(); }
    /// Convert a character index into a UTF-32 data index.
    [[nodiscard]] static auto dataIndex(const auto &, const std::uint64_t cpIndex) -> DataIndex {
        return el::CpIndex::fromSizeT(static_cast<std::size_t>(cpIndex));
    }
    /// Create a UTF-32 data range from character positions.
    [[nodiscard]] static auto dataRange(const auto &, const std::uint64_t cpStart, const std::uint64_t cpCount)
        -> DataRange {
        return DataRange{
            el::CpIndex::fromSizeT(static_cast<std::size_t>(cpStart)),
            el::CpLength::fromSizeT(static_cast<std::size_t>(cpCount))};
    }
    /// Convert a UTF-32 data index into a character index.
    [[nodiscard]] static auto characterIndex(const auto &, const DataIndex index) -> std::uint64_t {
        return index.toRawValue();
    }
    /// Test whether a UTF-32 value is valid.
    [[nodiscard]] static auto isValid(const auto &value) noexcept -> bool { return value.isValidUtf32(); }
};

}
