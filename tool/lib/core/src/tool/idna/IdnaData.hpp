// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IdnaRange.hpp"

#include "../unicode/UcdDataset.hpp"
#include "../unicode/UcdRecord.hpp"
#include "../unicode/UnicodeData.hpp"

#include <erbsland/LineIndex.hpp>
#include <erbsland/List.hpp>

#include <exception>

namespace tool::idna {

/// Derive RFC 5892 statuses and contextual attributes entirely from verified UCD inputs.
/// @tested{IdnaDataTest IdnaDataFullTest}
class IdnaData final {
public:
    /// Number of Unicode code points represented by the source-derived attribute table.
    static constexpr auto cCodePointCount = 0x110000U;
    /// Attribute bit marking code points whose general category differs from Cn.
    static constexpr auto cAssigned = 1U << 16U;
    /// Attribute bit marking membership in the RFC 5892 LetterDigits category.
    static constexpr auto cLetterDigits = 1U << 17U;
    /// Attribute bit marking code points changed by NFKC case folding.
    static constexpr auto cUnstable = 1U << 18U;
    /// Attribute bit marking the RFC 5892 ignorable-property union.
    static constexpr auto cIgnorable = 1U << 19U;
    /// Attribute bit marking membership in an RFC 5892 ignorable block.
    static constexpr auto cIgnorableBlock = 1U << 20U;
    /// Attribute bit marking leading, vowel, or trailing Hangul Jamo.
    static constexpr auto cOldHangul = 1U << 21U;
    /// Attribute bit marking Unicode noncharacters.
    static constexpr auto cNoncharacter = 1U << 22U;

public:
    /// Derive the complete Unicode snapshot.
    /// @param dataset Verified Unicode input files.
    /// @throws el::ApplicationError On malformed or inconsistent UCD records.
    explicit IdnaData(const unicode::UcdDataset &dataset);
    /// Derive attributes while sharing already validated UnicodeData records.
    /// @param dataset Verified Unicode property sources.
    /// @param unicodeData Validated records from the same dataset.
    /// @throws el::ApplicationError On malformed or inconsistent property records.
    IdnaData(const unicode::UcdDataset &dataset, const unicode::UnicodeData &unicodeData);

public: // accessors
    /// Get attributes for a code point; disallowed and unassigned entries return zero.
    /// @param codePoint Unicode code point, or an out-of-range value.
    /// @return The encoded runtime attributes.
    [[nodiscard]] auto attributes(char32_t codePoint) const -> unsigned;
    /// Get merged ranges split at page boundaries.
    [[nodiscard]] auto ranges() const -> el::List<IdnaRange>;
    /// Build page offsets for the compact ranges.
    /// @param ranges Sorted compact ranges.
    /// @return Offsets including the final sentinel.
    /// @throws el::ApplicationError If the range count exceeds 16-bit offsets.
    [[nodiscard]] static auto pageOffsets(const el::List<IdnaRange> &ranges) -> el::List<el::ItemIndex>;
    /// Apply RFC exception precedence and the ordered property decisions.
    /// @param codePoint Unicode code point.
    /// @param properties Input flags assembled from the UCD.
    /// @return PVALID=1, CONTEXTJ=2, CONTEXTO=3, or zero for other statuses.
    [[nodiscard]] static auto deriveStatus(char32_t codePoint, unsigned properties) -> unsigned;

private:
    /// Build the merged ranges from UCD records.
    /// @param dataset UCD dataset to read the text.
    /// @param unicodeData Validated records from the same dataset.
    void readUnicodeData(const unicode::UcdDataset &dataset, const unicode::UnicodeData &unicodeData);
    /// Apply assignment, category, bidi, and combining attributes to a record range.
    /// @param record The validated UnicodeData record.
    /// @param first The first covered code point.
    /// @param last The last covered code point.
    /// @param filename The source path for diagnostics.
    void applyUnicodeRecord(
        const unicode::UcdRecord &record, char32_t first, char32_t last, const el::String &filename);
    /// Apply flags to ranges matching a named UCD property.
    /// @param dataset The verified source dataset.
    /// @param filename The property input filename.
    /// @param property The property name to select.
    /// @param bits The flags to add to matching ranges.
    void applyProperty(
        const unicode::UcdDataset &dataset, const el::String &filename, const el::String &property, unsigned bits);
    /// Read selected script attributes from the source snapshot.
    /// @param dataset The verified source dataset.
    /// @param filename The scripts input filename.
    void applyScripts(const unicode::UcdDataset &dataset, const el::String &filename);
    /// Read source joining types and reject unknown values.
    /// @param dataset The verified source dataset.
    /// @param filename The joining types input filename.
    void applyJoiningTypes(const unicode::UcdDataset &dataset, const el::String &filename);
    /// Merge property flags into every code point in an inclusive range.
    /// @param first The first covered code point.
    /// @param last The last covered code point.
    /// @param bits The flags to merge.
    void addBits(char32_t first, char32_t last, unsigned bits);

private:
    /// Report invalid source data or compact representation limits.
    /// @param description The reason derivation failed.
    /// @param filename The source path when known.
    /// @param lineIndex The zero-based source line when known.
    /// @param cause The underlying parsing failure when present.
    [[noreturn]] static void throwDataError(
        el::String description,
        const el::String &filename = {},
        el::LineIndex lineIndex = {},
        const std::exception_ptr &cause = {});

private:
    el::List<unsigned> _properties; ///< UCD flags during construction, runtime attributes afterward.
};

}
