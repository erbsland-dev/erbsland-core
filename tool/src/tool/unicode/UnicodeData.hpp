// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UcdDataset.hpp"
#include "UcdRecord.hpp"

#include <erbsland/List.hpp>

namespace tool::unicode {

/// Validated UnicodeData records, with First/Last pairs expanded into ranges.
/// @tested{UnicodeSourceTest IdnaDataTest IdnaDataFullTest}
class UnicodeData final {
public:
    /// Parse and validate the UnicodeData input.
    /// @param dataset Verified source dataset.
    /// @throws el::ApplicationError If a record or range is malformed.
    explicit UnicodeData(const UcdDataset &dataset);

public: // accessors
    /// Get the validated records in code-point order.
    [[nodiscard]] auto records() const noexcept -> const el::List<UcdRecord> & { return _records; }

private:
    /// Append a regular record or merge a matching First/Last pair.
    /// @param record The validated, ordered source record.
    /// @param pending The pending First record, updated when a range starts.
    /// @param inRange Whether a range is open, updated as endpoints are consumed.
    void appendRecord(UcdRecord record, UcdRecord &pending, bool &inRange);
    /// Validate category, bidi class, and canonical combining class fields.
    /// @param record The source record to validate.
    void validateProperties(const UcdRecord &record) const;
    /// Require equal properties at both endpoints of a source range.
    /// @param first The First record.
    /// @param last The matching Last record.
    void validateRange(const UcdRecord &first, const UcdRecord &last) const;
    /// Report a malformed source record.
    /// @param record The record supplying diagnostic line information.
    /// @param description The reason the record is invalid.
    [[noreturn]] void throwError(const UcdRecord &record, const el::String &description) const;

private:
    el::List<UcdRecord> _records; ///< Single-point records and merged source ranges.
    el::String _source;           ///< Source path for diagnostics.
};

}
