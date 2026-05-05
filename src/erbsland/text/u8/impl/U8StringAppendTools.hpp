// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringDataView.hpp"
#include "U8StringSharedStorage.hpp"

#include "../../../unit/CpLength.hpp"
#include "../../../unit/ElementCount.hpp"
#include "../../Char.hpp"
#include "../../u16/impl/U16StringDataView_fwd.hpp"
#include "../../u32/impl/U32StringDataView_fwd.hpp"

#include <cstddef>
#include <span>

namespace erbsland::text::impl {

/// Append algorithms for `U8String`.
/// @tested{U8StringModifierTest}
class U8StringAppendTools final {
public:
    explicit U8StringAppendTools(U8StringSharedStorage &storage) noexcept : _storage{storage} {}

public:
    /// Append UTF-8 bytes from a data view.
    auto append(const U8StringDataView &text) -> unit::CpLength;
    /// Append UTF-8 bytes from a data view multiple times.
    auto append(const U8StringDataView &text, unit::ElementCount count) -> unit::CpLength;
    /// Append UTF-16 text decoded with replacement.
    auto append(const U16StringDataView &text) -> unit::CpLength;
    /// Append UTF-16 text decoded with replacement multiple times.
    auto append(const U16StringDataView &text, unit::ElementCount count) -> unit::CpLength;
    /// Append UTF-32 text decoded with replacement.
    auto append(const U32StringDataView &text) -> unit::CpLength;
    /// Append UTF-32 text decoded with replacement multiple times.
    auto append(const U32StringDataView &text, unit::ElementCount count) -> unit::CpLength;
    /// Append one Unicode code point.
    auto append(Char character) -> unit::CpLength;
    /// Append one Unicode code point multiple times.
    auto append(Char character, unit::CpLength count) -> unit::CpLength;

private:
    struct AppendSummary {
        std::size_t encodedLength{};
        unit::CpLength characterCount{};
    };

    [[nodiscard]] static auto countDecodedCharacters(std::span<const char> source) noexcept -> unit::CpLength;
    [[nodiscard]] static auto summarizeForUtf8(std::span<const char16_t> source) -> AppendSummary;
    [[nodiscard]] static auto summarizeForUtf8(std::span<const char32_t> source) -> AppendSummary;

private:
    U8StringSharedStorage &_storage;
};

}
