// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Char.hpp"
#include "../CharRange.hpp"
#include "../StringSide.hpp"
#include "../u16/U16String.hpp"
#include "../u16/U16StringView.hpp"
#include "../u32/U32String.hpp"
#include "../u32/U32StringView.hpp"
#include "../u8/U8String.hpp"
#include "../u8/U8StringView.hpp"

#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/CpRange.hpp"
#include "../../unit/U16DataIndex.hpp"
#include "../../unit/U16DataLength.hpp"
#include "../../unit/U16DataRange.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <utility>

namespace erbsland::text::impl {

inline constexpr auto cNoStringPatternDivider = std::numeric_limits<std::size_t>::max();

enum class StringPatternElementKind : std::uint8_t {
    Character,
    OneChar,
    Set,
};

struct StringPatternElement final {
    StringPatternElementKind kind{StringPatternElementKind::Character};
    Char character{};
    std::uint16_t rangeOffset{};
    std::uint16_t rangeCount{};
};

struct StringPatternView final {
    std::span<const StringPatternElement> elements{};
    std::span<const CharRange> ranges{};
    std::size_t divider{cNoStringPatternDivider};
};

class StringPatternData;

using StringPatternDataPtr = std::shared_ptr<const StringPatternData>;

class StringPatternData {
    template <typename tStringView>
    struct MatchResult final {
        using Index = decltype(std::declval<tStringView>().indexAt(StringSide::Front));
        bool matched{};
        Index frontEnd{};
        Index suffixStart{};
    };

public:
    StringPatternData() = default;
    virtual ~StringPatternData() = default;
    StringPatternData(const StringPatternData &) = default;
    StringPatternData(StringPatternData &&) = default;
    auto operator=(const StringPatternData &) -> StringPatternData & = default;
    auto operator=(StringPatternData &&) -> StringPatternData & = default;

public:
    [[nodiscard]] virtual auto view() const noexcept -> StringPatternView = 0;

public:
    [[nodiscard]] auto matches(const U8StringView &text) const noexcept -> bool;
    [[nodiscard]] auto matches(const U16StringView &text) const noexcept -> bool;
    [[nodiscard]] auto matches(const U32StringView &text) const noexcept -> bool;
    auto trim(U8StringView &text) const noexcept -> bool;
    auto trim(U16StringView &text) const noexcept -> bool;
    auto trim(U32StringView &text) const noexcept -> bool;
    auto trim(U8String &text) const -> bool;
    auto trim(U16String &text) const -> bool;
    auto trim(U32String &text) const -> bool;
    [[nodiscard]] auto trimmed(const U8StringView &text) const noexcept -> U8StringView;
    [[nodiscard]] auto trimmed(const U16StringView &text) const noexcept -> U16StringView;
    [[nodiscard]] auto trimmed(const U32StringView &text) const noexcept -> U32StringView;
    [[nodiscard]] auto split(const U8StringView &text) const noexcept -> std::pair<U8StringView, U8StringView>;
    [[nodiscard]] auto split(const U16StringView &text) const noexcept -> std::pair<U16StringView, U16StringView>;
    [[nodiscard]] auto split(const U32StringView &text) const noexcept -> std::pair<U32StringView, U32StringView>;
    [[nodiscard]] auto length(const U8StringView &text) const noexcept -> unit::ByteLength;
    [[nodiscard]] auto length(const U16StringView &text) const noexcept -> unit::U16DataLength;
    [[nodiscard]] auto length(const U32StringView &text) const noexcept -> unit::CpLength;
    [[nodiscard]] auto index(const U8StringView &text) const noexcept -> unit::ByteIndex;
    [[nodiscard]] auto index(const U16StringView &text) const noexcept -> unit::U16DataIndex;
    [[nodiscard]] auto index(const U32StringView &text) const noexcept -> unit::CpIndex;

private:
    template <typename tLength, typename tIndex>
    [[nodiscard]] static auto lengthFromStart(tIndex index) noexcept -> tLength;
    template <typename tLength, typename tIndex>
    [[nodiscard]] static auto lengthToEnd(tLength length, tIndex index) noexcept -> tLength;
    template <typename tStringView>
    [[nodiscard]] auto match(const StringPatternView &patternView, const tStringView &text) const noexcept
        -> MatchResult<tStringView>;
    template <typename tStringView, typename tIndex>
    [[nodiscard]] auto matchFront(
        const StringPatternView &patternView,
        const tStringView &text,
        std::size_t begin,
        std::size_t end,
        tIndex &index) const noexcept -> bool;
    template <typename tStringView, typename tIndex>
    [[nodiscard]] auto matchBack(
        const StringPatternView &patternView,
        const tStringView &text,
        std::size_t begin,
        std::size_t end,
        tIndex &index) const noexcept -> bool;
    [[nodiscard]] static auto matchesElement(
        const StringPatternView &patternView, const StringPatternElement &element, Char character) noexcept -> bool;
};

}

#include "StringPatternData.tpp"
