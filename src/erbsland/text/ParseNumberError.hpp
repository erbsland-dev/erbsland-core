// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReadNumberStatus.hpp"
#include "String_fwd.hpp"

#include "../err/ParseError.hpp"

namespace erbsland::text {

/// A number parse error exception.
/// This exception adds the reader status that caused the failure.
/// @tested{StringCharReaderTest}
class ParseNumberError : public err::ParseError {
public:
    /// Create a number parse error with a reason and status.
    /// @param reason The reason for the parse error.
    /// @param status The reader status that caused the error.
    /// @param position The optional code-point position of the parse error.
    explicit ParseNumberError(
        String reason, ReadNumberStatus status, unit::CpIndex position = unit::CpIndex::noIndex()) noexcept;

    // defaults
    ~ParseNumberError() override = default;

public: // accessors
    /// Get the reader status that caused the error.
    [[nodiscard]] auto status() const noexcept -> ReadNumberStatus { return _status; }

private:
    ReadNumberStatus _status{ReadNumberStatus::Success}; ///< The reader status that caused the error.
};

}
