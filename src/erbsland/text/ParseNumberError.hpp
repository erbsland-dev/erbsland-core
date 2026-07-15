// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReadNumberStatus.hpp"

#include "../err/ParseError.hpp"

#include <string_view>
#include <utility>

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
        StringView reason, ReadNumberStatus status, unit::CpIndex position = unit::CpIndex::noIndex()) noexcept :
        err::ParseError{std::move(reason), position}, _status{status} {}
    /// @overload
    explicit ParseNumberError(
        const std::string_view reason,
        ReadNumberStatus status,
        unit::CpIndex position = unit::CpIndex::noIndex()) noexcept :
        ParseNumberError{String{reason}, status, position} {}

    // defaults
    ~ParseNumberError() override = default;

public: // accessors
    /// Get the reader status that caused the error.
    [[nodiscard]] auto status() const noexcept -> ReadNumberStatus { return _status; }

private:
    ReadNumberStatus _status{ReadNumberStatus::Success}; ///< The reader status that caused the error.
};

}
