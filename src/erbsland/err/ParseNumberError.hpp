// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParseError.hpp"

#include "../text/ReadNumberStatus.hpp"

#include <string_view>
#include <utility>

namespace erbsland::err {

/// A number parse error exception.
/// This exception adds the reader status that caused the failure.
/// @tested{StringCharReaderTest}
class ParseNumberError : public ParseError {
public:
    /// Create a number parse error with a reason and status.
    /// @param reason The reason for the parse error.
    /// @param status The reader status that caused the error.
    /// @param position The optional code-point position of the parse error.
    explicit ParseNumberError(
        text::StringView reason,
        text::ReadNumberStatus status,
        unit::CpIndex position = unit::CpIndex::noIndex()) noexcept :
        ParseError{std::move(reason), position}, _status{status} {}
    /// @overload
    explicit ParseNumberError(
        const std::string_view reason,
        text::ReadNumberStatus status,
        unit::CpIndex position = unit::CpIndex::noIndex()) noexcept :
        ParseNumberError{text::String{reason}, status, position} {}

    // defaults
    ~ParseNumberError() override = default;

public: // accessors
    /// Get the reader status that caused the error.
    [[nodiscard]] auto status() const noexcept -> text::ReadNumberStatus { return _status; }

private:
    text::ReadNumberStatus _status{text::ReadNumberStatus::Success}; ///< The reader status that caused the error.
};

}
