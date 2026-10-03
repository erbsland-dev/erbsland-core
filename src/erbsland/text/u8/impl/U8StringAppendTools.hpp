// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringDataView.hpp"
#include "U8StringSharedStorage.hpp"

#include "../../../mem/ByteSpan.hpp"
#include "../../../unit/CpCount.hpp"
#include "../../../unit/ItemCount.hpp"
#include "../../Char.hpp"
#include "../../impl/StringAppendTools.hpp"
#include "../../u16/impl/U16StringDataView_fwd.hpp"
#include "../../u32/impl/U32StringDataView_fwd.hpp"

#include <cstddef>
#include <span>

namespace erbsland::text::impl {

/// Append algorithms for `U8StringEditor`.
/// Source views are borrowed; these tools do not retain their storage.
/// An owning source string may share the destination allocation on entry: its ownership
/// keeps the input alive while copy-on-write detaches the destination before writing.
/// @tested{U8StringModifierTest UnsafeU8StringEditorAccessTest StringAppendAliasingTest}
class U8StringAppendTools final : public StringAppendTools {
public:
    /// Create append tools for `storage`.
    explicit U8StringAppendTools(U8StringSharedStorage &storage) noexcept : _storage{storage} {}

    // defaults
    ~U8StringAppendTools() override = default;

public:
    using StringAppendTools::append;
    /// Append the exact span bytes without validation, preserving embedded nulls and BOMs.
    /// @pre The caller must retain the source across destination detachment or growth.
    /// Source data must not overlap the destination write range.
    /// @param text The source bytes to append.
    /// @return The appended code-point count, counting malformed sequences as replacement characters.
    auto append(std::span<const char> text) -> unit::CpCount;
    /// @overload
    /// @pre The caller must retain the source across destination detachment or growth.
    /// Source data must not overlap the destination write range.
    auto append(std::span<const std::byte> text) -> unit::CpCount;
    /// @overload
    /// @pre The caller must retain the source across destination detachment or growth.
    /// Source data must not overlap the destination write range.
    auto append(mem::ConstByteSpan text) -> unit::CpCount;
    /// Append UTF-8 bytes from a data view.
    /// @pre The caller must retain the source across destination detachment or growth.
    /// Source data must not overlap the destination write range.
    /// @param text The source data view to append.
    /// @return The appended code-point count, counting malformed sequences as replacement characters.
    auto append(const U8StringDataView &text) -> unit::CpCount;
    /// Append UTF-8 bytes from a data view multiple times.
    /// @pre The caller must retain the source across destination detachment or growth.
    /// Source data must not overlap the destination write range.
    /// @param text The source data view to append.
    /// @param count The number of repetitions.
    /// @return The total appended code-point count, counting malformed sequences as replacement characters.
    auto append(const U8StringDataView &text, unit::ItemCount count) -> unit::CpCount;
    /// Append UTF-16 text decoded with replacement.
    /// @pre The caller must retain the source across destination detachment or growth.
    /// Source data must not overlap the destination write range.
    auto append(const U16StringDataView &text) -> unit::CpCount;
    /// Append UTF-16 text decoded with replacement multiple times.
    /// @pre The caller must retain the source across destination detachment or growth.
    /// Source data must not overlap the destination write range.
    auto append(const U16StringDataView &text, unit::ItemCount count) -> unit::CpCount;
    /// Append UTF-32 text decoded with replacement.
    /// @pre The caller must retain the source across destination detachment or growth.
    /// Source data must not overlap the destination write range.
    auto append(const U32StringDataView &text) -> unit::CpCount;
    /// Append UTF-32 text decoded with replacement multiple times.
    /// @pre The caller must retain the source across destination detachment or growth.
    /// Source data must not overlap the destination write range.
    auto append(const U32StringDataView &text, unit::ItemCount count) -> unit::CpCount;
    /// Append one Unicode code point.
    auto append(Char character) -> unit::CpCount override;
    /// Append one Unicode code point multiple times.
    auto append(Char character, unit::CpCount count) -> unit::CpCount;

public: // implement StringAppendTools
    auto append(const U8String &text) -> unit::CpCount override;
    auto append(const U16String &text) -> unit::CpCount override;
    auto append(const U32String &text) -> unit::CpCount override;

private:
    /// Encoded and character lengths resulting from one append operation.
    struct AppendSummary {
        std::size_t encodedLength{};
        unit::CpCount characterCount{};
    };

    /// Summarize UTF-16 source converted to UTF-8.
    [[nodiscard]] static auto summarizeForUtf8(std::span<const char16_t> source) -> AppendSummary;
    /// Summarize UTF-32 source converted to UTF-8.
    [[nodiscard]] static auto summarizeForUtf8(std::span<const char32_t> source) -> AppendSummary;

private:
    U8StringSharedStorage &_storage;
};

}
