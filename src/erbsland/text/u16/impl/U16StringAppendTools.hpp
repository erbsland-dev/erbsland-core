// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringDataView.hpp"
#include "U16StringSharedStorage.hpp"

#include "../../../unit/CpLength.hpp"
#include "../../../unit/ItemCount.hpp"
#include "../../Char.hpp"
#include "../../impl/StringAppendTools.hpp"
#include "../../u32/impl/U32StringDataView_fwd.hpp"
#include "../../u8/impl/U8StringDataView_fwd.hpp"

#include <cstddef>
#include <span>

namespace erbsland::text::impl {

/// Append algorithms for `U16StringEditor`.
/// @tested{U16StringTest}
class U16StringAppendTools final : public StringAppendTools {
public:
    /// Create append tools for `storage`.
    explicit U16StringAppendTools(U16StringSharedStorage &storage) noexcept : _storage{storage} {}

    // defaults
    ~U16StringAppendTools() override = default;

public:
    using StringAppendTools::append;
    /// Append UTF-16 bytes from a data view.
    auto append(const U16StringDataView &text) -> unit::CpLength;
    /// Append UTF-16 bytes from a data view multiple times.
    auto append(const U16StringDataView &text, unit::ItemCount count) -> unit::CpLength;
    /// Append UTF-8 text decoded with replacement.
    auto append(const U8StringDataView &text) -> unit::CpLength;
    /// Append UTF-8 text decoded with replacement multiple times.
    auto append(const U8StringDataView &text, unit::ItemCount count) -> unit::CpLength;
    /// Append UTF-32 text decoded with replacement.
    auto append(const U32StringDataView &text) -> unit::CpLength;
    /// Append UTF-32 text decoded with replacement multiple times.
    auto append(const U32StringDataView &text, unit::ItemCount count) -> unit::CpLength;
    /// Append one Unicode code point.
    auto append(Char character) -> unit::CpLength override;
    /// Append one Unicode code point multiple times.
    auto append(Char character, unit::CpLength count) -> unit::CpLength;

public: // implement StringAppendTools
    auto append(const U8String &text) -> unit::CpLength override;
    auto append(const U16String &text) -> unit::CpLength override;
    auto append(const U32String &text) -> unit::CpLength override;

private:
    /// Encoded and character lengths resulting from one append operation.
    struct AppendSummary {
        std::size_t encodedLength{};
        unit::CpLength characterCount{};
    };

    /// Summarize UTF-8 source converted to UTF-16.
    [[nodiscard]] static auto summarizeForUtf16(std::span<const char> source) -> AppendSummary;
    /// Summarize UTF-32 source converted to UTF-16.
    [[nodiscard]] static auto summarizeForUtf16(std::span<const char32_t> source) -> AppendSummary;

private:
    U16StringSharedStorage &_storage;
};

}
