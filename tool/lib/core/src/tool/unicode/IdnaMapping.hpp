// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UcdDataset.hpp"
#include "UcdReader.hpp"
#include "UcdRecord.hpp"

#include <erbsland/List.hpp>
#include <erbsland/String.hpp>

namespace tool::unicode {

/// Nontransitional UTS #46 mapping from the official versioned source table.
/// @tested{UnicodeSourceTest PublicSuffixFullTest}
class IdnaMapping final {
public:
    /// Parse the verified mapping table.
    /// @param dataset Unicode sources containing IdnaMappingTable.txt.
    /// @throws el::ApplicationError On missing coverage or malformed mappings.
    explicit IdnaMapping(const UcdDataset &dataset);

public: // mapping
    /// Map Unicode text without consulting compiled Unicode tables.
    /// @param text Input text.
    /// @return Mapped text, before NFC normalization and IDNA validation.
    /// @throws el::ApplicationError For invalid text or disallowed characters.
    [[nodiscard]] auto map(const el::String &text) const -> el::String;

private:
    /// Decode a record's optional replacement and reject surrogate scalars.
    /// @param record The mapping record.
    /// @param reader The source-aware code point parser.
    /// @return The decoded replacement text.
    [[nodiscard]] auto readReplacement(const UcdRecord &record, const UcdReader &reader) const -> el::String;
    /// Locate the mapping range for a Unicode scalar.
    /// @param value The input character.
    /// @return The index of the covering record.
    [[nodiscard]] auto mappingIndex(el::Char value) const -> el::ItemIndex;
    /// Report malformed mapping input with its source location.
    /// @param description The reason mapping data is invalid.
    /// @param line The zero-based source line when known.
    [[noreturn]] void throwError(const el::String &description, el::LineIndex line = {}) const;

private:
    el::List<UcdRecord> _records; ///< Ordered mapping ranges with source fields.
    el::StringList _replacements; ///< Decoded replacements, indexed with the ranges.
    el::String _source;           ///< Mapping source path.
};

}
