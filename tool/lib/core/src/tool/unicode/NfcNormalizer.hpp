// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UcdDataset.hpp"
#include "UcdReader.hpp"
#include "UnicodeData.hpp"

#include <erbsland/LineIndex.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Map.hpp>
#include <erbsland/Set.hpp>
#include <erbsland/String.hpp>

#include <span>
#include <utility>
#include <vector>

namespace tool::unicode {

/// Source-backed NFC normalization without runtime Unicode tables or fixed working buffers.
/// @tested{UnicodeSourceTest UnicodeSourceFullTest}
class NfcNormalizer final {
public:
    /// Load canonical mappings, combining classes, categories, and composition exclusions.
    /// @param dataset Verified Unicode source files.
    /// @throws el::ApplicationError On malformed or cyclic source data.
    explicit NfcNormalizer(const UcdDataset &dataset);
    /// Load normalization data while sharing validated UnicodeData records.
    /// @param dataset Verified normalization source files.
    /// @param unicodeData Validated records from the same dataset.
    /// @throws el::ApplicationError On malformed or cyclic normalization data.
    NfcNormalizer(const UcdDataset &dataset, const UnicodeData &unicodeData);

public: // normalization
    /// Normalize valid UTF-8 text to NFC using the loaded snapshot.
    /// @param text Input text.
    /// @return Canonically normalized text.
    /// @throws el::ApplicationError If the input is invalid UTF-8.
    [[nodiscard]] auto normalize(const el::String &text) const -> el::String;
    /// Test whether the source category is a combining mark.
    /// @param codePoint Unicode code point.
    [[nodiscard]] auto isMark(char32_t codePoint) const -> bool { return _marks.contains(codePoint); }

private:
    /// Load combining classes, mark categories, and a record's decomposition.
    /// @param record The validated UnicodeData record.
    /// @param reader The source-aware code point parser.
    void readUnicodeRecord(const UcdRecord &record, const UcdReader &reader);
    /// Validate decomposition syntax and retain canonical mappings.
    /// @param record The UnicodeData record.
    /// @param reader The source-aware code point parser.
    void readDecomposition(const UcdRecord &record, const UcdReader &reader);
    /// Load composition exclusions and derive canonical composition pairs.
    /// @param dataset Verified normalization source files.
    void readCompositions(const UcdDataset &dataset);
    /// Fully decompose a scalar and insert its results in canonical order.
    /// @param character The input scalar.
    /// @param decomposed The accumulating decomposed sequence.
    void decompose(char32_t character, std::vector<char32_t> &decomposed) const;
    /// Insert one scalar in canonical combining-class order.
    /// @param decomposed The accumulating decomposed sequence.
    /// @param value The scalar to insert.
    void appendOrdered(std::vector<char32_t> &decomposed, char32_t value) const;
    /// Compose a canonically ordered sequence into NFC text.
    /// @param decomposed The canonically ordered scalars.
    /// @return The normalized text.
    [[nodiscard]] auto composeSequence(std::span<const char32_t> decomposed) const -> el::String;
    /// Reject cyclic canonical decompositions before normalization.
    /// @throws el::ApplicationError If a decomposition cycle is present.
    void validateGraph() const;
    /// Find a canonical composition, including algorithmic Hangul.
    /// @param first The starter code point.
    /// @param second The following code point.
    /// @return The composed scalar, or zero if no composition exists.
    [[nodiscard]] auto compose(char32_t first, char32_t second) const -> char32_t;
    /// Expand an algorithmically decomposable Hangul syllable.
    /// @param value The scalar to decompose.
    /// @return Its Jamo sequence, or an empty list for a non-Hangul scalar.
    [[nodiscard]] static auto decomposeHangul(char32_t value) -> el::List<char32_t>;
    /// Report malformed normalization data with optional source context.
    /// @param description The reason normalization data is invalid.
    /// @param source The input path when known.
    /// @param line The zero-based source line when known.
    [[noreturn]] static void throwError(
        const el::String &description, const el::String &source = {}, el::LineIndex line = {});

private:
    el::String _source;                                             ///< UnicodeData diagnostic path.
    el::Map<char32_t, el::LineIndex> _locations;                    ///< Decomposition source locations.
    el::Map<char32_t, el::List<char32_t>> _mappings;                ///< Canonical direct decompositions.
    el::Map<char32_t, unsigned> _classes;                           ///< Nonzero canonical combining classes.
    el::Map<std::pair<char32_t, char32_t>, char32_t> _compositions; ///< Canonical composition pairs.
    el::Set<char32_t> _marks;                                       ///< Source-defined mark categories.
};

}
