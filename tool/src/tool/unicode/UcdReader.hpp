// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UcdRecord.hpp"

#include <erbsland/LineIndex.hpp>
#include <erbsland/List.hpp>

namespace tool::unicode {

/// Strict, shared UCD record parsing independent of compiled Unicode property tables.
/// @tested{UcdReaderTest}
class UcdReader final {
public:
    /// Create a new reader instance.
    /// @param text Complete UCD text.
    /// @param filename Source filename used for error reporting.
    explicit UcdReader(el::String text, el::String filename);

public: // parsing
    /// Parse non-comment UCD records, trimming surrounding whitespace and preserving empty fields.
    /// @param minimumFields Required field count.
    /// @param exactFields Require exactly minimumFields when true.
    /// @return Parsed records in source order.
    /// @throws el::ApplicationError On malformed fields or invalid ranges.
    [[nodiscard]] auto parse(el::ItemCount minimumFields, bool exactFields = false) -> el::List<UcdRecord>;
    /// Parse one Unicode code-point value.
    /// @param text Hexadecimal value without a prefix.
    /// @return A code point in 0000..10FFFF, including surrogate entries in UCD data.
    /// @throws el::ApplicationError On invalid syntax or overflow.
    [[nodiscard]] auto codePoint(const el::String &text) const -> char32_t;

private:
    /// Throw a parser error.
    /// @param description Detailed description of the problem (why did it went wrong?).
    /// @param cause The optional underlying exception causing this error.
    [[noreturn]] void throwError(el::String description, const std::exception_ptr &cause = {}) const;

private:
    el::String _text;         ///< The complete UCD text.
    el::String _fileName;     ///< Filename for error reporting (e.g., "UnicodeData.txt").
    el::LineIndex _lineIndex; ///< The current line index in the input.
};

}
